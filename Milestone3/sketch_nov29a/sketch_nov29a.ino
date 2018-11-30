#define FFT_N 256 // set to 256 point fft
#define LOG_OUT 1 // use the log output function

#include <Servo.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <StackArray.h>
#include <QueueList.h>

/*Set up nRF24L01 radio on SPI bus plus pins 9 & 10*/
RF24 radio(9, 10);

/* Radio pipe addresses for the 2 nodes to communicate.*/
const uint64_t pipes[2] = { 0x000000002ALL, 0x000000002BLL};

Servo servo_left; // pin 6
Servo servo_right; // pin 5

/*boolean which is true if a robot has been detected, false otherwise*/
bool sees_robot = false;

/*boolean which is true if 660Hz has been detected, false otherwise*/
bool detects_audio = false;

/*boolean which is true if the override button has been pressed, false otherwise*/
bool button_pressed = false;

/* The rf message*/
unsigned int data;

/*Current coordinates - robot starts at {0,0} and can go up to {mx,my}*/
int x = 0;
int y = 0;

/* Orientation of robot with respect to where we start
   in the GUI. Directions are absolute relative to GUI.
   0 = north
   1 = east
   2 = south
   3 = west

  {0,0}        y        {0,m}
    ---------------------       N(0)
  start |   |   |   |   |    W(3)  E(1)
    ---------------------       S(2)
    |   |   |   |   |   |
    ---------------------
  x |   |   |   |   |   |
    ---------------------
    |   |   |   |   |   |
    ---------------------
    |   |   |   |   |   |
    ---------------------
  {m,0}                 {m,m}
*/
int heading = 2;
int counter = 0;

/*Line sensors*/
int sensor_left = A3;
int sensor_middle = A4;
int sensor_right = A5;

/*Initialize sensor threshold values*/
int line_thresh = 400; //if BELOW this we detect a white line
int wall_thresh = 150; //if ABOVE this we detect a wall
int IR_threshold = 160; //if ABOVE this we detect IR hat

/*A coordinate contains an x,y location*/
struct coordinate {
  int x;
  int y;
};
/*Declare a type coordinate as Coordinate*/
typedef struct coordinate Coordinate;

/*Info contains information at an x,y coordinate. Information includes
  whether the coordinate has been explored as well as wall information*/
struct info {
  bool explored = 0;
  bool n_wall = 1;
  bool e_wall = 1;
  bool s_wall = 1;
  bool w_wall = 1;
};
/*Declare a type info as Info*/
typedef struct info Info;

/*All coordinates are only updated at intersections and based on surrounding walls*/

/*Coordinate to the left of the way the robot is moving (initially the left of {0,0})*/
Coordinate left = {0, 1};

/*Coordinate in front of the way the robot is moving (initially the front of {0,0})*/
Coordinate front = {1, 0};

/*Coordinate to the right of the way the robot is moving (no initial right coordinate)*/
Coordinate right;

/*Coordinate behind the robot (no initial coordinate)*/
Coordinate back;

/*This is the coordinate the robot is about to go to. Declared globally so both goTo() and maze_traversal_dfs() can access its information*/
Coordinate v;

/*m is the maximum index of the 2d maze array*/
int mx = 3;
int my = 4;
int area = (mx + 1) * (my + 1);

/*2d array which is the size of the maze to traverse. Each
  index of the maze (maze[x][y]) contains the wall information
  at that coordinate, as well as if that coordinate has been explored.

  Size of 2d array is (mx+1)x(my+1) so indexes range from maze[0][0] to maze[mx][my]
  If at location maze[x][y], depth = x*/
Info maze[5][5];

/*Initializes a stack of coordinates (type Coordinate)*/
StackArray <Coordinate> stack;

/*Initializes the servo*/
void servo_setup() {
  servo_right.attach(5);
  servo_left.attach(6);
  servo_left.write(90);
  servo_right.write(90);
}


/*Go Straight*/
void go() {
  servo_left.write(100);
  servo_right.write(80);
}


/*Stop*/
void halt() {
  servo_left.write(90);
  servo_right.write(90);
}


/*Simple left turn adjust for linefollow*/
void turn_left() {
  servo_left.write(93);
  servo_right.write(70);
}

/*Simple right turn adjust for linefollow*/
void turn_right() {
  servo_left.write(110);
  servo_right.write(87);
}


/*Turns to the left in place*/
void turn_place_left() {
  servo_left.write(80);
  servo_right.write(80);
}


/*Turns to the right in place*/
void turn_place_right() {
  servo_left.write(100);
  servo_right.write(100);
}


/*Turns to the right until the middle sensor finds a line (allows for 90 degree turns)*/
void turn_right_linetracker() {
  turn_place_right();
  delay(300); //delay to get off the line - used to be 300 tried reducing it
  /*Following while loops keep the robot turning until we find the line to the right of us*/
  while (analogRead(sensor_middle) < line_thresh);
  while (analogRead(sensor_middle) > line_thresh);
  /*After we turn right our heading changes. N->E, E->S, S->W, W->N*/
  heading++;
  if (heading == 4) heading = 0;
}


/*Turns to the left until a middle sensor finds a line (allows for 90 degree turns)*/
void turn_left_linetracker() {
  turn_place_left();
  delay(300); //delay to get off the line - used to be 300 tried reducing it
  /*Following while loops keep the robot turning until we find the line to the left of us*/
  while (analogRead(sensor_middle) < line_thresh);
  while (analogRead(sensor_middle) > line_thresh);
  /*After we turn left our heading changes. N->W, E->N, S->E, W->S*/
  heading--;
  if (heading == -1) heading = 3;
}

/*Time it takes for wheels to reach intersection from the time the sensors detect the intersection*/
void adjust() {
  go();
  delay(600); //delay value to reach specification
}

/*Pulls a U-turn, updates heading accordinglyy*/
void turn_around() {
  turn_right_linetracker();
  turn_right_linetracker();
}

/* Periodically switches rotation to help debug */
void error() {
  int d = 500;
  while (1) {
    turn_place_left();
    delay(d);
    turn_place_right();
    delay(d);
  }
}


/*
  Select output of mux based on select bits.
  Order is s2 s1 s0 from 000 to 111
  000 is audio
  001 is IR
  010
  011 is right wall sensor
  100
  101 is left wall sensor
  110
  111 is middle wall sensor
*/
void mux_select(int s2, int s1, int s0) {
  digitalWrite(4, s2); //MSB s2
  digitalWrite(2, s1); //s1
  digitalWrite(3, s0); //LSB s0
  /*small delay allows mux enough time to select appropriate input before
    relevant code executes*/
  delay(55);
}


/*Sets mux_select to left wall sensor information, and returns true if there is a wall to the left*/
bool check_left() {
  //mux_select(1, 0, 1);
  if (analogRead(A2) > wall_thresh) {
    return true;
  } else {
    return false;
  }
}

/*Sets mux_select to front wall sensor information, and returns true and turns on LED if there is a wall in front. */
bool check_front() {
  // mux_select(1, 1, 1);
  if (analogRead(A1) > wall_thresh) {
    return true;
  } else {
    return false;
  }
}

/*Sets mux_select to right wall sensor information, and returns true and turns on LED if there is a wall to the right. */
bool check_right() {
  //mux_select(0, 1, 1);
  if (analogRead(A0) > wall_thresh) {
    return true;
  } else {
    return false;
  }
}

/* Updates the position of the robot assuming the robot starts at {0,0} facing towards {m,0}.
   Also updates the coordinates surrounding the robot.
   FOLLOWING CONDITIONS MATTER FOR INDEXING THE ARRAY
   Note: Robot can't go more North then x = 0
         Robot can't go more South then x = mx
         Robot can't go more West then y = 0
         Robot can't go more East then y = my

   Note: We don't know any information about the surrounding
         coordinates walls at the time of updating, but that
         will be updated as soon as the robot reaches that coordinate
*/
void update_position() {
  switch (heading) {
    case 0: //NORTH
      x--;
      left = {x, y - 1};
      front = {x - 1, y};
      right = {x, y + 1};
      back = {x + 1, y};
      break;
    case 1: //EAST
      y++;
      left = {x - 1, y};
      front = {x, y + 1};
      right = {x + 1, y};
      back = {x, y - 1};
      break;
    case 2: //SOUTH
      x++;
      left = {x, y + 1};
      front = {x + 1, y};
      right = {x, y - 1};
      back = {x - 1, y};
      break;
    case 3: //WEST
      y--;
      left = {x + 1, y};
      front = {x, y - 1};
      right = {x - 1, y};
      back = {x, y + 1};
      break;
  }
}


/*Scans for surrounding walls and updates data accordingly.
  Also updates wall info in Maze for the current {x,y} coordinate*/
void scan_walls() {
  switch (heading) {
    case 0: //NORTH
      if (check_left()) {
        data = data | 0x0100; // west=true
        maze[x][y].w_wall = 1;
      } else {
        maze[x][y].w_wall = 0;
        if (is_in_bounds(left)) maze[left.x][left.y].e_wall = 0;
      }
      if (check_front()) {
        data = data | 0x0200; // north=true
        maze[x][y].n_wall = 1;
      } else {
        maze[x][y].n_wall = 0;
        if (is_in_bounds(front)) maze[front.x][front.y].s_wall = 0;
      }
      if (check_right()) {
        data = data | 0x0400; // east=true
        maze[x][y].e_wall = 1;
      } else {
        maze[x][y].e_wall = 0;
        if (is_in_bounds(right)) maze[right.x][right.y].w_wall = 0;
      }
      maze[x][y].s_wall = 0;
      break;
    case 1: //EAST
      if (check_left()) {
        data = data | 0x0200;// north=true
        maze[x][y].n_wall = 1;
      } else {
        maze[x][y].n_wall = 0;
        if (is_in_bounds(left)) maze[left.x][left.y].s_wall = 0;
      }
      if (check_front()) {
        data = data | 0x0400;// east=true
        maze[x][y].e_wall = 1;
      } else {
        maze[x][y].e_wall = 0;
        if (is_in_bounds(front)) maze[front.x][front.y].w_wall = 0;
      }
      if (check_right()) {
        data = data | 0x0800;// south=true
        maze[x][y].s_wall = 1;
      } else {
        maze[x][y].s_wall = 0;
        if (is_in_bounds(right)) maze[right.x][right.y].n_wall = 0;
      }
      maze[x][y].w_wall = 0;
      break;
    case 2: //SOUTH
      if (check_left()) {
        data = data | 0x0400;// east=true
        maze[x][y].e_wall = 1;
      } else {
        maze[x][y].e_wall = 0;
        if (is_in_bounds(left)) maze[left.x][left.y].w_wall = 0;
      }
      if (check_front()) {
        data = data | 0x0800;// south=true
        maze[x][y].s_wall = 1;
      } else {
        maze[x][y].s_wall = 0;
        if (is_in_bounds(front)) maze[front.x][front.y].n_wall = 0;
      }
      if (check_right()) {
        data = data | 0x0100;// west=true
        maze[x][y].w_wall = 1;
      } else {
        maze[x][y].w_wall = 0;
        if (is_in_bounds(right)) maze[right.x][right.y].e_wall = 0;
      }
      maze[x][y].n_wall = 0;
      break;
    case 3: //WEST
      if (check_left()) {
        data = data | 0x0800; // south=true
        maze[x][y].s_wall = 1;
      } else {
        maze[x][y].s_wall = 0;
        if (is_in_bounds(left)) maze[left.x][left.y].n_wall = 0;
      }
      if (check_front()) {
        data = data | 0x0100; // west=true
        maze[x][y].w_wall = 1;
      } else {
        maze[x][y].w_wall = 0;
        if (is_in_bounds(front)) maze[front.x][front.y].e_wall = 0;
      }
      if (check_right()) {
        data = data | 0x0200;// north=true
        maze[x][y].n_wall = 1;
      } else {
        maze[x][y].n_wall = 0;
        if (is_in_bounds(right)) maze[right.x][right.y].s_wall = 0;
      }
      maze[x][y].e_wall = 0;
      break;
  }
}


/*Turns the information into data and starts listening*/
void rf() {
  /* format info into data*/
  data = data | y;
  data = data | x << 4;

  radio.stopListening();
  bool ok = radio.write( &data, sizeof(unsigned int) );

  if (ok)
    printf("ok...\n");
  else
    printf("failed.\n\r");

  /*Now, continue listening*/
  radio.startListening();

  //  Serial.println(data, HEX);

  /*Clear the data*/
  data = data & 0x0000;
}




/*Follows the line if a line sensor is on one. Halts movement if all three sensors are not on a line*/
void linefollow() {
  if (analogRead(sensor_middle) < line_thresh) {
    go();
  }
  if (analogRead(sensor_left) < line_thresh) {
    turn_left();
  }
  if (analogRead(sensor_right) < line_thresh) {
    turn_right();
  }
  if (analogRead(sensor_right) > line_thresh && analogRead(sensor_left) > line_thresh && analogRead(sensor_middle) > line_thresh) {
    halt(); //MIGHT NEED TO CHANGE THIS. IF WE DETECT ONLY BLACK WE NEED A FIND_WHITE FUNCTION
  }
}


/*Returns true if the robot is at an intersection, else false*/
bool atIntersection() {
  if ((analogRead(sensor_right) < line_thresh) && (analogRead(sensor_left) < line_thresh) && (analogRead(sensor_middle) < line_thresh)) {
    return true;
  }
  else {
    return false;
  }
}

/*Returns true if the robot is at an intersection for n readings, else false*/
bool atIntersection_avg() {
  bool flag = false;
  int n = 250;
  if (atIntersection()) {
    go();
    flag = true;
    for (int i = 0; i < n; i++) {
      if (!atIntersection()) {
        flag = false;
      }
    }
  }
  return flag;
}

/*Used to calibrate n in atIntersection_avg()*/
bool atIntersection_test() {
  bool flag = false;
  int c = 0;
  while (atIntersection()) {
    go();
    flag = true;
    c++;
  }
  if (c != 0) Serial.println(c);
  //  if (c > 250) Serial.println();
  return flag;
}

/*True if v is in the bounds of the maze*/
bool is_in_bounds(Coordinate v) {
  if ((0 <= v.x && v.x <= mx) && (0 <= v.y && v.y <= my)) {
    return true;
  }
  return false;
}

/* Pushes the unvisited intersections w from current intersection v
   Pushes in reverse order of the way we visit!*/
void push_unvisited() {
  if (is_in_bounds(right)) {
    /*If the coordinate to the right has not been explored and there is no wall to the right, push right coordinate to stack*/
    if (!check_right() && !maze[right.x][right.y].explored) {
      stack.push(right);
    }
  }
  if (is_in_bounds(left)) {
    /*If the coordinate to the left has not been explored and there is no wall to the right, push left coordinate to stack*/
    if (!check_left() && !maze[left.x][left.y].explored) {
      stack.push(left);
    }
  }
  if (is_in_bounds(front)) {
    /*If the coordinate in front has not been explored and there is no wall in front, push front coordinate to stack*/
    if (!check_front() && !maze[front.x][front.y].explored) {
      stack.push(front);
    }
  }
}


Coordinate move_coord(Coordinate a, int h) {
  switch (h) {
    case 0:
      a.x = a.x - 1;
      break;
    case 1:
      a.y = a.y + 1;
      break;
    case 2:
      a.x = a.x + 1;
      break;
    case 3:
      a.y = a.y - 1;
      break;
  }
  return a;
}


/*path is the path to return*/
QueueList <Coordinate> path;
/* Searches for and builds a path to given coordinate, following FLR order and deprioritizing backtracking */
void find_path(Coordinate b) {

  /*set next and prev to current coordinate*/
  Coordinate next = {x, y};
  Coordinate prev = {x, y};

  Serial.print("current.x: ");
    Serial.println(next.x);
    Serial.print("current.y: ");
    Serial.println(next.y);

  Coordinate f = front;
  Coordinate l = left;
  Coordinate r = right;

  /*h is the heading the robot needs to traverse a point*/
  int h = heading;

  /*while the next coordinate to add to the queue is not v*/
  while (next.x != b.x || next.y != b.y) {
    /*west of means that the robot needs heading = 3 to get there*/
    bool westof = (b.y < next.y);
    /*east of means that the robot needs heading = 1 to get there*/
    bool eastof = (b.y > next.y);
    /*south of means that the robot needs heading = 2 to get there*/
    bool southof = (b.x > next.x);
    /*north of means that the robot needs heading = 0 to get there*/
    bool northof = (b.x < next.x);

    /*following booleans are true if there is a wall at the {x,y} coordinate in that direction.
      DIRECTION HERE IS ABSOLUTE*/
    bool n = maze[next.x][next.y].n_wall;
    bool e = maze[next.x][next.y].e_wall;
    bool s = maze[next.x][next.y].s_wall;
    bool w = maze[next.x][next.y].w_wall;

    /*GOING NORTH -> X--
      GOING SOUTH -> X++
      GOING WEST  -> Y--
      GOING EAST  -> Y++*/


    switch (h) {
      case 0: //NORTH
        l = {next.x, next.y - 1};
        f = {next.x - 1, next.y};
        r = {next.x, next.y + 1};
        break;
      case 1: //EAST
        l = {next.x - 1, next.y};
        f = {next.x, next.y + 1};
        r = {next.x + 1, next.y};
        break;
      case 2: //SOUTH
        l = {next.x, next.y + 1};
        f = {next.x + 1, next.y};
        r = {next.x, next.y - 1};
        break;
      case 3: //WEST
        l = {next.x + 1, next.y};
        f = {next.x, next.y - 1};
        r = {next.x - 1, next.y};
        break;
    }

    // add is_in_bounds to ifs
    /*Choose next coordinate*/
    switch (h) {
      case 0: //NORTH (reject South)
        /*if there is no wall to the north at the current coordinate and v coordinate is north of us*/
        if (!n && northof && is_in_bounds(f)) {
          next = move_coord(next, 0);
        }
        /*else if there is no wall to the west at the current coordinate and v is west of us*/
        else if (!w && westof && is_in_bounds(l)) {
          next = move_coord(next, 3);
        }
        /*else if there is no wall to the east at the current coordinate and v is to the east of us*/
        else if (!e && eastof && is_in_bounds(r)) {
          next = move_coord(next, 1);
        }
        /*else if there is no wall to the north at the current coordinate*/
        else if (!n && is_in_bounds(f)) {
          next = move_coord(next, 0);
        }
        /*else if there is no wall to the west at the current coordinate*/
        else if (!w && is_in_bounds(l)) {
          next = move_coord(next, 3);
        }
        /*else if there is no wall to the east at the current coordinate*/
        else if (!e && is_in_bounds(r)) {
          next = move_coord(next, 1);
        }
        /*else there is a wall at all surrounding coordinates and we need to go to south*/
        else {
          next = move_coord(next, 2);
        }
        break;
      case 1: //EAST (reject West)
        /*if there is no wall to the east at the current coordinate and v is east of us*/
        if (!e && eastof && is_in_bounds(f)) {
          next = move_coord(next, 1);
        }
        /*else if there is no wall to the north at the current coordinate and v is to the north of us*/
        else if (!n && northof && is_in_bounds(l)) {
          next = move_coord(next, 0);
        }
        /*else if there is no wall to the south of us at the current coordinate and v is south of us*/
        else if (!s && southof && is_in_bounds(r)) {
          next = move_coord(next, 2);
        }
        /*else if there is no wall to the east at the current coordinate*/
        else if (!e && is_in_bounds(f)) {
          next = move_coord(next, 1);
        }
        /*else if there is no wall to the north at the current coordinate*/
        else if (!n && is_in_bounds(l)) {
          next = move_coord(next, 0);
        }
        /*else if there is no wall to the south at the current coordinate*/
        else if (!s && is_in_bounds(r)) {
          next = move_coord(next, 2);
        }
        /*else there is a wall to the north, south, and east at the current coordinate and we need to go west*/
        else {
          next = move_coord(next, 3);
        }
        break;
      case 2: //SOUTH (reject North)
        if (!s && southof && is_in_bounds(f)) {
          next = move_coord(next, 2);
        } else if (!e && eastof && is_in_bounds(l)) {
          next = move_coord(next, 1);
        } else if (!w && westof && is_in_bounds(r)) {
          next = move_coord(next, 3);
        } else if (!s && is_in_bounds(f)) {
          next = move_coord(next, 2);
        } else if (!e && is_in_bounds(l)) {
          next = move_coord(next, 1);
        } else if (!w && is_in_bounds(r)) {
          next = move_coord(next, 3);
        } else {
          next = move_coord(next, 0);
        }
        break;
      case 3: //WEST (reject East)
        if (!w && westof && is_in_bounds(f)) {
          next = move_coord(next, 3);
        } else if (!s && southof && is_in_bounds(l)) {
          next = move_coord(next, 2);
        } else if (!n && northof && is_in_bounds(r)) {
          next = move_coord(next, 0);
        } else if (!w && is_in_bounds(f)) {
          next = move_coord(next, 3);
        } else if (!s && is_in_bounds(l)) {
          next = move_coord(next, 2);
        } else if (!n && is_in_bounds(r)) {
          next = move_coord(next, 0);
        } else {
          next = move_coord(next, 1);
        }
        break;
    }

    /*Set the heading we approach the next coordinate at*/
    if (prev.x < next.x) {
      h = 2;
    } else if (prev.x > next.x) {
      h = 0;
    } else if (prev.y < next.y) {
      h = 1;
    } else if (prev.y > next.y) {
      h = 3;
    }
    /*Set the previous coordinate to the coordinate the robot was just at*/
    prev = next;
    /*push next coordinate to the queue*/
    Serial.print("x: ");
    Serial.println(next.x);
    Serial.print("y: ");
    Serial.println(next.y);
    path.push(next);
  }
}

/*traverses the given path*/
void traverse_path(QueueList <Coordinate> route) {
  
    Serial.print("back.x: ");
    Serial.println(back.x);
    Serial.print("back.y: ");
    Serial.println(back.y);
  //  /*don't wanna update position the first time so we set a flag variable*/
  bool first_run2 = true;
  /*while the path to traverse is not empty*/
  while (!route.isEmpty()) {
    /*if we are at an intersection*/
    if (atIntersection_avg()) {
      //      adjust();
      halt();
      if (first_run2) {
        first_run2 = false;
      } else {

        // since adjust has been added to dfs else, always need to update position
        update_position();
      }

      /*Coordinate p is what is popped off the queue*/
      Coordinate p = route.pop();

    Serial.print("p.x: ");
    Serial.println(p.x);
    Serial.print("p.y: ");
    Serial.println(p.y);
      /*if p is the coordinate in front of us*/
      if (p.x == front.x && p.y == front.y) {
        /*send relevant information to GUI, go straight*/
        //        scan_walls();
        rf();
        adjust();
      }
      /*else if p is the left coordinate*/
      else if (p.x == left.x && p.y == left.y) {
        /*send relevant information to GUI, turn left*/
        //        scan_walls();
        rf();
        adjust();
        turn_left_linetracker();
      }
      /*else if p is the right coordinate*/
      else if (p.x == right.x && p.y == right.y) {
        /*send relevant information to GUI, turn right*/

        //        scan_walls();
        rf();
        adjust();
        turn_right_linetracker();
      }
      /*else the coordinate to go to is behind us*/
      else if ((p.x == back.x && p.y == back.y)) {
        /*send relevant information to GUI, turn around*/
        //        scan_walls();
        rf();
        adjust();
        turn_around();
      }
      /*else given bad path*/
      else {
        error();
      }
    }
    /*if we are not at an intersection we line follow*/
    linefollow();
  }
}




/* Traverses a maze via depth first search while line following. Updates GUI via radio communication.
        At each intersection the robot will scan the walls around it.
          It will always explore the front branch first,
          then go left and back to the straight branch,
          then go right and back to the straight branch
  Have to remember all of this is in a while loop
  NEED TO ADD IR CODE IN THE TRAVERSAL
*/
bool first_run = true;
void maze_traversal_dfs() {
  /*If we are at an intersection*/
  if (atIntersection_avg()) {
    //adjust();
    /*stop so we have time to think*/
    halt();
    /*update the robots position and the surrounding coordinates*/
    if (first_run) {
      maze[0][0].explored = 1;
      first_run = false;
    }
    else {
      update_position();
    }

    scan_walls();
    rf();

    /*push the surrounding unvisited nodes to the stack*/
    push_unvisited();

    /*if the stack is NOT empty*/
    if (!stack.isEmpty()) {
      /*Coordinate v is the coordinate the robot is about to visit*/
      v = stack.pop();
      /*If the robot has NOT BEEN TO v,*/
      if (!maze[v.x][v.y].explored) {
        //        Serial.print("Right: ");
        //        Serial.print(right.x);
        //        Serial.print(" ");
        //        Serial.println(right.y);
        /*If v is the front coordinate*/
        if (is_in_bounds(front) && v.x == front.x && v.y == front.y) {
          /*send relevant information to GUI, go straight*/
//          scan_walls();
//          rf();
          adjust();
        }
        /*else if v is the left coordinate*/
        else if (is_in_bounds(left) && v.x == left.x && v.y == left.y) {
          /*send relevant information to GUI, turn left*/
//          scan_walls();
//          rf();
          adjust();
          turn_left_linetracker();
        }
        /*else if v is the right coordinate*/
        else if (is_in_bounds(right) && v.x == right.x && v.y == right.y) {
          /*send relevant information to GUI, turn right*/
//          scan_walls();
//          rf();
          adjust();
          turn_right_linetracker();
        }
        /*else if v is the coordinate behind*/
        else if (is_in_bounds(back) && v.x == back.x && v.y == back.y) {
          /*send relevant information to GUI, turn around*/
//          scan_walls();
//          rf();
          adjust();
          turn_around();
        }
        /*else if v is neither the front, left, or right coordinate
          of the robot we have explored a whole branch and need to go
          back to the coordinate where the robot branched from.*/
        else {
//          scan_walls();
//          rf();
          find_path(v);
//          error(); // did not get here
          traverse_path(path);
        }
        /*Mark v as visited*/
        maze[v.x][v.y].explored = 1;
        counter++;
        //         Serial.print("Count: ");
        // Serial.println(counter);
      }
    }
    // else stack is empty
    // halt when all nodes are explored
    // can easily be removed
    // else {
    //      while (1) halt();
    //    }
  }
  /*If we are NOT at an intersection we linefollow*/
  linefollow();
}


/*Set up necessary stuff for our code to work*/
void setup() {
  Serial.begin(115200); // use the serial port
  servo_setup(); //setup the servos

  /*Setup LEDS for testing*/
  pinMode(7, OUTPUT);

  /*Setup MUX Select Pins*/
  pinMode(4, OUTPUT); //S2 - MSB
  pinMode(2, OUTPUT); //S1
  pinMode(3, OUTPUT); //S0 - LSB

  /* Setup and configure rf radio*/
  radio.begin();

  /*Optionally, increase the delay between retries & # of retries*/
  radio.setRetries(15, 15);
  radio.setAutoAck(true);

  /*Set the channel*/
  radio.setChannel(0x50);

  /*Set the power
    RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_MED=-6dBM, and RF24_PA_HIGH=0dBm.*/
  radio.setPALevel(RF24_PA_MAX);

  /*RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps*/
  radio.setDataRate(RF24_2MBPS);

  /*Optionally, reduce the payload size. Seems to improve reliability*/
  radio.setPayloadSize(8);

  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);

  /*ADD AUDIO CODE HERE*/

  /*This function adjusts robot to traverse the maze from the get go under the
    assumption the robot must start at the relative bottom right of the maze facing
    relative up. Also updates GUI accordingly and begins DFS accordingly.*/
  //  robot_start();
}


/*Run main code*/
void loop() {
  maze_traversal_dfs();
}

//  Serial.print("Left: ");
//  Serial.println(analogRead(A2));
//  Serial.print("Middle: ");
//  Serial.println(analogRead(A1));
//  Serial.print("Right: ");
//  Serial.println(analogRead(A0));
//  delay(1000);

//int c_int = 0;
//  if (atIntersection_test()) {
//
//    adjust();
//    c_int++;
//    if (c_int == 2) {
//      c_int = 0;
//      Serial.println();
//      turn_around();
//    }
//
//  } else {
//    linefollow();
//  }


