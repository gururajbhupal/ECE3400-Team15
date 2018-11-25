#define FFT_N 256 // set to 256 point fft
#define LOG_OUT 1 // use the log output function

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

/*Current coordinates - robot starts at {0,0} and can go up to {m,m}*/
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
  bool explored;
  bool n_wall;
  bool e_wall;
  bool s_wall;
  bool w_wall;
};
/*Declare a type info as Info*/
typedef struct info Info;

/*All coordinates are only updated at intersections and based on surrounding walls*/

/*Coordinate to the left of the way the robot is moving (initially the left of {0,0})*/
Coordinate left = {0, 1};

/*Coordinate in front of the way the robot is moving (initially the front of {0,0})*/
Coordinate front = {1, 0};

/*Coordinate to the right of the way the robot is moving (no initial right coordinate until we leave x = 0 column)*/
Coordinate right;

/*This is the coordinate the robot is about to go to. Declared globally so both goTo() and maze_traversal_dfs() can access its information*/
Coordinate v;

/*m is the maximum index of the 2d maze array*/
int m = 8;

/*2d array which is the size of the maze to traverse. Each
  index of the maze contains the x,y coordinate
  (i.e for maze[0][1] x=0, y=1) as well as the wall information
  at that coordinate, as well as if that coordinate has been explored.
  Size of 2d array is 9x9 so indexes range from maze[0][0] to maze[8][8]*/
Info maze[9][9];

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
  delay(100); //delay to get off the line - used to be 300 tried reducing it
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
  delay(100); //delay to get off the line - used to be 300 tried reducing it
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


/* Updates the position of the robot assuming the robot starts at {0,0} facing towards {m,0}. 
   Also updates the coordinates surrounding the robot.
   FOLLOWING CONDITIONS MATTER FOR INDEXING THE ARRAY
   Note: Robot can't go more North then x = 0
         Robot can't go more South then x = m
         Robot can't go more West then y = 0
         Robot can't go more East then y = m

   Note: We don't know any information about the surrounding
         coordinates walls at the time of updating, but that
         will be updated as soon as the robot reaches that coordinate
*/
void update_position() {
  switch (heading) {
    case 0:
      x--;
      if (y != 0) {left = {x, y - 1};}
      if (x != 0) {front = {x - 1, y};}
      if (y != m) {right = {x, y + 1};}
      break;
    case 1:
      y--;
      if (x != 0) {left = {x - 1, y};}
      if (y != m) {front = {x, y + 1};}
      if (x != m) {right = {x + 1, y};}
      break;
    case 2:
      x++;
      if (y != m) {left = {x, y + 1};}
      if (x != m) {front = {x + 1, y};}
      if (y != 0) {right = {x, y - 1};}
      break;
    case 3:
      y++;
      if (x != m) {left = {x + 1, y};}
      if (y != 0) {front = {x, y - 1};}
      if (x != 0) {right = {x - 1, y};}
      break;
  }
}


/*Scans for surrounding walls and updates data accordingly.
  Also updates wall info in Maze for the current {x,y} coordinate*/
void scan_walls() {
  switch (heading) {
    case 0: // north
      if (check_left()) data = data | 0x0100; // west=true
      if (check_front()) data = data | 0x0200; // north=true
      if (check_right()) data = data | 0x0400; // east=true
      break;
    case 1: // west
      if (check_left()) data = data | 0x0800; // south=true
      if (check_front()) data = data | 0x0100; // west=true
      if (check_right()) data = data | 0x0200;// north=true
      break;
    case 2: // south
      if (check_left()) data = data | 0x0400;// east=true
      if (check_front()) data = data | 0x0800;// south=true
      if (check_right()) data = data | 0x0100;// west=true
      break;
    case 3: // east
      if (check_left()) data = data | 0x0200;// north=true
      if (check_front()) data = data | 0x0400;// east=true
      if (check_right()) data = data | 0x0800;// south=true
      break;
  }
  /*Update the wall information at current coordinate. The directions here are absolute relative to GUI*/
  maze[x][y].n_wall = (data >> 9) & 0x0001;
  maze[x][y].e_wall = (data >> 10) & 0x0001;
  maze[x][y].s_wall = (data >> 11) & 0x0001;
  maze[x][y].w_wall = (data >> 8) & 0x0001;
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

  Serial.println(data, HEX);

  /*Clear the data*/
  data = data & 0x0000;
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
  delay(45);
}


/*Sets mux_select to left wall sensor information, and returns true if there is a wall to the left*/
bool check_left() {
  mux_select(1, 0, 1);
  if (analogRead(A0) > wall_thresh) {
    return true;
  } else {
    return false;
  }
}

/*Sets mux_select to front wall sensor information, and returns true and turns on LED if there is a wall in front. */
bool check_front() {
  mux_select(1, 1, 1);
  if (analogRead(A0) > wall_thresh) {
    return true;
  } else {
    return false;
  }
}

/*Sets mux_select to right wall sensor information, and returns true and turns on LED if there is a wall to the right. */
bool check_right() {
  mux_select(0, 1, 1);
  if (analogRead(A0) > wall_thresh) {
    return true;
  } else {
    return false;
  }
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

/* Pushes the unvisited intersections w from current intersection v
   Pushes in reverse order of the way we visit!*/
void push_unvisited() {
  if (!check_right() && !maze[right.x][right.y].explored) {
    stack.push(right);
  }
  if (!check_left() && !maze[left.x][left.y].explored) {
    stack.push(left);
  }
  if (!check_front() && !maze[front.x][front.y].explored) {
    stack.push(front);
  }

}

/*Since the robot starts after the {0,0} intersection we manually take care of this special case.
  We must scan the walls at {0,0} and evaluate unvisited nodes from the get go.

  Note: left, front and right are initialized to be values assuming you start at the bottom right at {0,0}
        after robot_start() DFS begins*/
void robot_start() {
  /*At the beginning set the {x,y} = {0,0} coordinate to explored*/
  maze[0][0].explored = 1;
  /*scan the walls and update the GUI*/
  scan_walls();
  rf();
  /*pushes front and left coordinate from 0,0 initially (should NEVER push right because there
    is always a wall to the right initially)*/
  push_unvisited();
  /*If there is NO wall to the front of us begin going that way to stay consistent with DFS*/
  if (!check_front()) {
    /*need to mark the immediate front coordinate as explored otherwise it will mess up exploration later*/
    maze[front.x][front.y].explored = 1;
  }
  else if (!check_left()) {
    /*turn left if there is a wall in front and no wall to the left*/
    turn_left_linetracker();
    /*need to mark the immediate front coordinate as explored otherwise it will mess up exploration later*/
    maze[left.x][left.y].explored = 1;
  }
}

/*This function allows the robot to go to a location.
  WE ONLY GO TO A NODE VIA NODES WE HAVE VISITED

  heading = 1,3 robot traverses y-axis
  heading = 0,2 robot traverses x-axis

   At       To
  (5,4) -> (3,3)

  Coordinates below are immediate coordinates and boolean evaluation shown next to it
  westof => true
  eastof => false
  southof => false
  northof => true

  booleans are directions from GUI perspective AND ARE ABSOLUTE
  i.e not relative to how the robot is facing


  {0,0}        y        {0,5}
    ---------------------       N
    |st |   |   |   |   |     W   E
    ---------------------       S
    |   |   |   |   |   |
    ---------------------     northof
  x |   |   |to |   |   | leftof   rightof
    ---------------------     southof
    |   |   |   |   |   |
    ---------------------
    |   |   |   |   | at|
    ---------------------
  {5,0}                 {5,5}

  WALL INFORMATION IN THE INFO MAZE IS UPDATED ABSOLUTELY! So a north
  wall is always a wall in that coordinate facing North
*/
void goTo(int x, int y) {
  /*local variable current keeps track of current coordinate. {x,y} will
    be updated once we reach v*/
  Coordinate current = {x, y};
  /*v is the coordinate to go to which was set in maze_traversal_dfs()*/



  /*west of means that the robot needs heading = 3 to get there*/
  bool westof = ((v.x == current.x) && (v.y < current.y));
  /*east of means that the robot needs heading = 1 to get there*/
  bool eastof = ((v.x == current.x) && (v.y > current.y));
  /*south of means that the robot needs heading = 2 to get there*/
  bool southof = ((v.x > current.x) && (v.y == current.y));
  /*north of means that the robot needs heading = 0 to get there*/
  bool northof = ((v.x < current.x) && (v.y == current.y));

  /*self explanatory. true if at meets the recquirement from current*/
  bool northeastof ((v.x < current.x) && (v.y > current.y));
  bool northwestof ((v.x < current.x) && (v.y < current.y));
  bool southeastof ((v.x > current.x) && (v.y > current.y));
  bool southwestof ((v.x > current.x) && (v.y < current.y));

  /*at is true when we have reached our destination*/
  bool at = ((v.x == current.x) && (v.y == current.y));

  /*while we are NOT at the coordinate we need to traverse towards it

    This is gonna be tricky and a lot of conditionals but possible.
    The maze is of type Info so each {x,y} coordinate has the wall information
    and we will be likely wanna traceback through already explored coordinates

    I think we will just wanna update current.x and current.y and then set x,y to
    v.x and v.y once we reach it so we can begin updating again*/
  while (v.x != x && v.y != y) {
    //IMPLEMENT ME?
  }
}


/* Traverses a maze via depth first search while line following. Updates GUI via radio communication
        At each intersection the robot will scan the walls around it.
          It will always explore the front branch first,
          then go left and back to the straight branch,
          then go right and back to the straight branch

   THERE IS A HUGE CHANGE WITH THIS COMPARED TO REGULAR DFS. Since all of this code runs in a while loop in loop() we would
   have nested while loops. But we only ever push unvisited nodes from intersections. We either gotta fix that while loop from
   standard DFS to only push to the stack at an intersection or go with what I'm doing. What I'm doing should work if we fix
   robot_start() the right way to push to the stack in setup before maze_traversal_dfs() runs.

  Have to remember all of this is in a while loop

  NEED TO ADD IR CODE IN THE TRAVERSAL
*/
void maze_traversal_dfs() {
  /*If we are at an intersection*/
  if (atIntersection()) {
    /*stop so we have time to think*/
    halt();
    /*update the robots position*/
    update_position();
    /*push the surrounding unvisited nodes to the stack*/
    push_unvisited();
    /*if the stack is NOT empty*/
    if (!stack.isEmpty()) {
      /*Coordinate v is the coordinate the robot is about to visit*/
      v = stack.pop();
      /*If the robot has NOT BEEN TO v,*/
      if (!maze[v.x][v.y].explored) {
        /*If v is the front coordinate*/
        if (v.x == front.x && v.y == front.y) {
          /*send relevant information to GUI, turn left*/
          scan_walls();
          rf();
          adjust();
        }
        /*else if v is the left coordinate*/
        else if (v.x == left.x && v.y == left.y) {
          /*send relevant information to GUI, go straight*/
          scan_walls();
          rf();
          adjust();
          turn_left_linetracker();
        }
        /*else if v is the right coordinate*/
        else if (v.x == right.x && v.y == right.y) {
          /*send relevant information to GUI, turn right*/
          scan_walls();
          rf();
          adjust();
          turn_right_linetracker();
        }
        /*else if v is neither the front, left, or right coordinate
          of the robot we have explored a whole branch and need to go
          back to the coordinate where the robot branched from.
          This is hard, so we should just turn around and rerun DFS.
          Does something just to show behavior for now.*/
        else {
          turn_place_right();
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
  robot_start();
}

/*Run main code*/
void loop() {
  maze_traversal_dfs();
}
