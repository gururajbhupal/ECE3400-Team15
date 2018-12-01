#include <Servo.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <StackArray.h>

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
int mx = 5;
int my = 5;
int area = (mx + 1) * (my + 1);

/*2d array which is the size of the maze to traverse. Each
  index of the maze (maze[x][y]) contains the wall information
  at that coordinate, as well as if that coordinate has been explored.

  Size of 2d array is (mx+1)x(my+1) so indexes range from maze[0][0] to maze[mx][my]
  If at location maze[x][y], depth = x*/
Info maze[6][6];

/*Initializes a stack of coordinates (type Coordinate)*/
StackArray <Coordinate> stack;
/*The stack of coordinates we were just at*/
StackArray <Coordinate> backstack;

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

/*Robot starts ahead of an intersection! So set {0,0} to explored. If we are going forward then {1,0} needs to 
  manually be set to explored as well. Likewise for left.*/
void robot_start(){
  maze[0][0].explored = 1;
  backstack.push({0,0});
  if(!check_front){
    maze[front.x][front.y].explored = 1;
  }
  else if(!check_left){
    maze[left.x][left.y].explored = 1;
    turn_left_linetracker();
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
  int n = 100;
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


/*backtracks to Coordinate v*/
void backtrack() {
  /*if this is the first run we don't wanna update position - since we halt at an intersection in 
    maze_traversal_dfs and update position there*/
  bool first_r = true;
  while (!backstack.isEmpty()) {
    Coordinate h;
    if (atIntersection_avg()) {
      if (first_r) {
        first_r = false;
      }
      else {
        update_position();
        rf();
      }
      
      /*if v is an adjacent coordinate go towards v*/
      if (v.x == front.x && v.y == front.y && !check_front()){
        adjust();
        break;
      }
      if(v.x == left.y && v.y == left.y && !check_left()){
        adjust();
        turn_left_linetracker();
        break;
      }
      if(v.x == right.x && v.y == right.y && !check_right()){
        adjust();
        turn_right_linetracker();
        break;
      }

      /*if v is not an adjacent coordinate we want to backtrack until it is*/
      h = backstack.pop();
      /*If h is the front coordinate*/
      if (is_in_bounds(front) && h.x == front.x && h.y == front.y) {
        /*go straight*/
        adjust();
      }
      /*else if h is the left coordinate*/
      else if (is_in_bounds(left) && h.x == left.x && h.y == left.y) {
        /*turn left*/
        adjust();
        turn_left_linetracker();
      }
      /*else if h is the right coordinate*/
      else if (is_in_bounds(right) && h.x == right.x && h.y == right.y) {
        /*turn right*/
        adjust();
        turn_right_linetracker();
      }
      /*else if h is the right coordinate*/
      else if (is_in_bounds(back) && h.x == back.x && h.y == back.y) {
        /*turn right*/
        adjust();
        turn_around();
      }
    }
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
void maze_traversal_dfs() {
  /*If we are at an intersection*/
  if (atIntersection_avg()) {
    /*stop so we have time to think*/
    halt();
    /*update the robots position and the surrounding coordinates*/
    update_position();
    /*push current coordinate to the back stack*/

    /*push the surrounding unvisited nodes to the stack*/
    push_unvisited();
    /*update wall information and update GUI*/
    scan_walls();
    rf();
    /*if the stack is NOT empty*/
    if (!stack.isEmpty()) {
      /*Coordinate v is the coordinate the robot is about to visit*/
      v = stack.pop();
      /*If the robot has NOT BEEN TO v,*/
      if (!maze[v.x][v.y].explored) {
        /*If v is the front coordinate*/
        if (is_in_bounds(front) && v.x == front.x && v.y == front.y) {
          backstack.push({x,y});          
          /*go straight*/
          adjust();
        }
        /*else if v is the left coordinate*/
        else if (is_in_bounds(left) && v.x == left.x && v.y == left.y) {
          backstack.push({x,y});   
          /*turn left*/
          adjust();
          turn_left_linetracker();
        }
        /*else if v is the right coordinate*/
        else if (is_in_bounds(right) && v.x == right.x && v.y == right.y) {
          backstack.push({x,y});   
          /*turn right*/
          adjust();
          turn_right_linetracker();
        }
        /*else if v is neither the front, left, or right coordinate
          of the robot we have explored a whole branch and need to go
          back to the coordinate where the robot branched from.*/
        else {
          backtrack();
        }
        /*Mark v as visited*/
        maze[v.x][v.y].explored = 1;
      }
    }
  }
  /*If we are NOT at an intersection we linefollow*/
  linefollow();
}


/*Set up necessary stuff for our code to work*/
void setup() {
  Serial.begin(115200); // use the serial port
  servo_setup(); //setup the servos

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

  /*INITIALIZER CODE*/
  robot_start();
}


/*Run main code*/
void loop() {
  maze_traversal_dfs();
}

