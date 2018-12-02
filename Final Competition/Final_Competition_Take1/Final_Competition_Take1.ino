#define FFT_N 128 // set to 256 point fft
#define LOG_OUT 1 // use the log output function

#include <Servo.h>
#include <FFT.h> // include the library
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
int line_thresh = 440; //if BELOW this we detect a white line
int wall_thresh = 150; //if ABOVE this we detect a wall
int IR_threshold = 80; //if ABOVE this we detect IR hat

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
  /*all coordinates of the maze are unexplored initially*/
  bool explored = 0;
  /*all walls of a maze of type info are initialized to be there.
    this insures that we cannot plan a path through unexplored coordinates
    when traversing*/
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

/*This is the coordinate the robot is about to go to. Declared globally so multiple functions can access its information*/
Coordinate v;

/*mx, my are the maximum indices of the 2d maze array*/
int mx = 8;
int my = 8;

/*2d array which is the size of the maze to traverse. Each
  index of the maze (maze[x][y]) contains the wall information
  at that coordinate, as well as if that coordinate has been explored.

  Size of 2d array is (mx+1)x(my+1) so indices range from maze[0][0] to maze[mx][my]
  If at location maze[x][y], depth = x*/
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
void turn_around() {
  turn_right_linetracker();
  turn_right_linetracker();
}


/*Periodically switches rotation to help debug*/
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
  Also updates wall info in Maze for the current {x,y} coordinate,
  and adjacent coordinate which we sometimes know the info of*/
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
    halt();
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

/*True if v is in the bounds of the maze
   FOLLOWING CONDITIONS MATTER FOR INDEXING THE ARRAY
   Note: Robot can't go more North then x = 0
         Robot can't go more South then x = mx
         Robot can't go more West then y = 0
         Robot can't go more East then y = my*/
bool is_in_bounds(Coordinate v) {
  if ((0 <= v.x && v.x <= mx) && (0 <= v.y && v.y <= my)) {
    return true;
  }
  return false;
}

/*Sets mux_select to audio information, and sets detects_audio to true if we detect a 660Hz signal. mux_select
  is then set to an empty signal on the mux to avoid noise from FFT interfering with servos.*/
void audio_detection() {
  mux_select(0, 0, 0); //select correct mux output

  /*Set temporary values for relevant registers*/
  int temp1 = TIMSK0;
  int temp2 = ADCSRA;
  int temp3 = ADMUX;
  int temp4 = DIDR0;


  /*Set register values to required values for IR detection*/
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0x40; // use adc0
  DIDR0 = 0x01; // turn off the digital input for adc0

  cli();  // UDRE interrupt slows this way down on arduino1.0
  for (int i = 0 ; i < 256 ; i += 2) { // save 128 samples
    while (!(ADCSRA & 0x10)); // wait for adc to be ready
    ADCSRA = 0xf5; // restart adc
    byte m = ADCL; // fetch adc data
    byte j = ADCH;
    int k = (j << 8) | m; // form into an int
    k -= 0x0200; // form into a signed int
    k <<= 6; // form into a 16b signed int
    fft_input[i] = k; // put real data into even bins
    fft_input[i + 1] = 0; // set odd bins to 0
  }
  fft_window(); // window the data for better frequency response
  fft_reorder(); // reorder the data before doing the fft
  fft_run(); // process the data in the fft
  fft_mag_log(); // take the output of the fft
  sei();

  /*When audio is detected, detects_audio is set to true. Once detected
    this value is never set back to false*/
  for (byte i = 0; i < FFT_N / 2; i++) {
    if (i == 5 && fft_log_out[i] > 135 / 2) {
      detects_audio = true;
    }
  }

  /*Restore the register values*/
  TIMSK0 = temp1;
  ADCSRA = temp2;
  ADMUX = temp3;
  DIDR0 =  temp4;

  mux_select(0, 1, 0); //SET TO BLANK OUTPUT TO AVOID FFT NOISE WITH SERVOS
}


/*Sets mux_select to IR information. Sets sees_Robot to true if there is a robot, else sees_robot = false
  mux_select is then set to an empty signal to avoid FFT noise interfering with servos.*/
void IR_detection() {
  mux_select(0, 0, 1); //select correct mux output

  /*Set temporary values for relevant registers*/
  int t1 = TIMSK0;
  int t2 = ADCSRA;
  int t3 = ADMUX;
  int t4 = DIDR0;

  /*Set register values to required values for IR detection*/
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0x40; // use adc0
  DIDR0 = 0x01; // turn off the digital input for adc0

  cli();  // UDRE interrupt slows this way down on arduino1.0
  for (int i = 0 ; i < 256 ; i += 2) { // save 128 samples
    while (!(ADCSRA & 0x10)); // wait for adc to be ready
    ADCSRA = 0xf5; // restart adc
    byte m = ADCL; // fetch adc data
    byte j = ADCH;
    int k = (j << 8) | m; // form into an int
    k -= 0x0200; // form into a signed int
    k <<= 6; // form into a 16b signed int
    fft_input[i] = k; // put real data into even bins
    fft_input[i + 1] = 0; // set odd bins to 0
  }
  fft_window(); // window the data for better frequency response
  fft_reorder(); // reorder the data before doing the fft
  fft_run(); // process the data in the fft
  fft_mag_log(); // take the output of the fft
  sei();

  for (byte i = 0 ; i < FFT_N / 2 ; i++) {
    /*If there is a robot*/
    if (i == 43 && fft_log_out[i] > IR_threshold) {
      sees_robot = true;
    }
    /*If there is no robot detected (care about not seeing IR case because in our implementation we need to exit our lock)*/
    if (i == 43 && fft_log_out[i] < IR_threshold) {
      sees_robot = false;
    }
  }

  /*Restore the register values*/
  TIMSK0 = t1;
  ADCSRA = t2;
  ADMUX = t3;
  DIDR0 =  t4;
  mux_select(0, 1, 0); //SET TO BLANK OUTPUT TO AVOID FFT NOISE WITH SERVOS
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


/*given a coordinate and heading, determine what the next coordinate
  will be based on heading*/
Coordinate calculate_coord(Coordinate a, int h) {
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


/*function which clears our queue for memory purposes since the QueueList library
  apparently doesn't do that when you pop*/
QueueList <Coordinate> clear_queue() {
  QueueList <Coordinate> empty;
  return empty;
}

/*path is the path to return*/
QueueList <Coordinate> path;
/*Searches for and builds a path to given coordinate, following FLR order and deprioritizing backtracking */
void find_path(Coordinate b) {
  /*set next and prev to current coordinate*/
  Coordinate next = {x, y};
  Coordinate prev = {x, y};

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

    /*calculate left, front, and right coordinates for each coordinate that gets added to our path*/
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

    /*Choose next coordinate to add based on heading and wall information*/
    switch (h) {
      case 0: //NORTH (reject South)
        /*if there is no wall to the north at the current coordinate and v coordinate is north of us*/
        if (!n && northof && is_in_bounds(f)) {
          next = calculate_coord(next, 0);
        }
        /*else if there is no wall to the west at the current coordinate and v is west of us*/
        else if (!w && westof && is_in_bounds(l)) {
          next = calculate_coord(next, 3);
        }
        /*else if there is no wall to the east at the current coordinate and v is to the east of us*/
        else if (!e && eastof && is_in_bounds(r)) {
          next = calculate_coord(next, 1);
        }
        /*else if there is no wall to the north at the current coordinate*/
        else if (!n && is_in_bounds(f)) {
          next = calculate_coord(next, 0);
        }
        /*else if there is no wall to the west at the current coordinate*/
        else if (!w && is_in_bounds(l)) {
          next = calculate_coord(next, 3);
        }
        /*else if there is no wall to the east at the current coordinate*/
        else if (!e && is_in_bounds(r)) {
          next = calculate_coord(next, 1);
        }
        /*else there is a wall at all surrounding coordinates and we need to go to south*/
        else {
          next = calculate_coord(next, 2);
        }
        break;
      case 1: //EAST (reject West)
        /*if there is no wall to the east at the current coordinate and v is east of us*/
        if (!e && eastof && is_in_bounds(f)) {
          next = calculate_coord(next, 1);
        }
        /*else if there is no wall to the north at the current coordinate and v is to the north of us*/
        else if (!n && northof && is_in_bounds(l)) {
          next = calculate_coord(next, 0);
        }
        /*else if there is no wall to the south of us at the current coordinate and v is south of us*/
        else if (!s && southof && is_in_bounds(r)) {
          next = calculate_coord(next, 2);
        }
        /*else if there is no wall to the east at the current coordinate*/
        else if (!e && is_in_bounds(f)) {
          next = calculate_coord(next, 1);
        }
        /*else if there is no wall to the north at the current coordinate*/
        else if (!n && is_in_bounds(l)) {
          next = calculate_coord(next, 0);
        }
        /*else if there is no wall to the south at the current coordinate*/
        else if (!s && is_in_bounds(r)) {
          next = calculate_coord(next, 2);
        }
        /*else there is a wall to the north, south, and east at the current coordinate and we need to go west*/
        else {
          next = calculate_coord(next, 3);
        }
        break;
      case 2: //SOUTH (reject North)
        if (!s && southof && is_in_bounds(f)) {
          next = calculate_coord(next, 2);
        } else if (!e && eastof && is_in_bounds(l)) {
          next = calculate_coord(next, 1);
        } else if (!w && westof && is_in_bounds(r)) {
          next = calculate_coord(next, 3);
        } else if (!s && is_in_bounds(f)) {
          next = calculate_coord(next, 2);
        } else if (!e && is_in_bounds(l)) {
          next = calculate_coord(next, 1);
        } else if (!w && is_in_bounds(r)) {
          next = calculate_coord(next, 3);
        } else {
          next = calculate_coord(next, 0);
        }
        break;
      case 3: //WEST (reject East)
        if (!w && westof && is_in_bounds(f)) {
          next = calculate_coord(next, 3);
        } else if (!s && southof && is_in_bounds(l)) {
          next = calculate_coord(next, 2);
        } else if (!n && northof && is_in_bounds(r)) {
          next = calculate_coord(next, 0);
        } else if (!w && is_in_bounds(f)) {
          next = calculate_coord(next, 3);
        } else if (!s && is_in_bounds(l)) {
          next = calculate_coord(next, 2);
        } else if (!n && is_in_bounds(r)) {
          next = calculate_coord(next, 0);
        } else {
          next = calculate_coord(next, 1);
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
    path.push(next);
  }
}


/*traverses the given route*/
void traverse_path(QueueList <Coordinate> route) {
  Coordinate p;
  /*local boolean variable which helps us not update position the first time since we stop on an
    intersection in maze_traversal_dfs()*/
  bool first_run2 = true;
  /*while the path to traverse is not empty*/
  while (!route.isEmpty()) {
    /*if we are at an intersection*/
    if (atIntersection_avg()) {
      /*halt*/
      halt();
      /*if this is the first run DO NOT UPDATE POSITION*/
      if (first_run2) {
        first_run2 = false;
      } else {
        update_position();
      }
      /*Coordinate p is what is popped off the queue*/
      p = route.pop();
      /*if p is the coordinate in front of us*/
      if (p.x == front.x && p.y == front.y) {
        /*send relevant information to GUI, go straight*/
        rf();
        adjust();
      }
      /*else if p is the left coordinate*/
      else if (p.x == left.x && p.y == left.y) {
        /*send relevant information to GUI, turn left*/
        rf();
        adjust();
        turn_left_linetracker();
      }
      /*else if p is the right coordinate*/
      else if (p.x == right.x && p.y == right.y) {
        /*send relevant information to GUI, turn right*/
        rf();
        adjust();
        turn_right_linetracker();
      }
      /*else the coordinate to go to is behind us*/
      else if ((p.x == back.x && p.y == back.y)) {
        /*send relevant information to GUI, turn around*/
        rf();
        adjust();
        turn_around();
      }
      /*else given bad path so freak out*/
      else {
        error();
      }
    }
    /*if we are not at an intersection we line follow*/
    linefollow();
  }
  /*MUST CLEAR THE PATH WHEN WE ARE DONE BECAUSE POP DOESN"T DO SO*/
  path = clear_queue();
}


/*Set the initial conditions for the robot assuming we start at the northwestern most corner facing
  north*/
void robot_start() {
  /*set {0,0} to explored*/
  maze[0][0].explored = 1;
  /*manually set west and south wall to true*/
  data = data | 0x0100;
  data = data | 0x0200;
  /*scan for other walls and update wall information for {0,0} in Maze*/
  scan_walls();
  /*if there is no wall in front of you, you will go forward so that is the starting intersection
    and we need to set that to explored*/
  if (!check_front()) {
    maze[front.x][front.y].explored = 1;
    /*if there was no wall in front of you and no wall to the left of you the left coordinate must
      be pushed to the stack*/
    if (!check_left()) {
      stack.push(left);
    }
  }
  /*else if there is a wall in front of you and no wall to the left of you*/
  else if (!check_left()) {
    /*left is the starting intersection so you must set it to explored*/
    maze[left.x][left.y].explored = 1;
    turn_left_linetracker();
  }
  /*update GUI with correct initial conditions*/
  rf();
}


/* Traverses a maze via depth first search while line following. Updates GUI via radio communication.
        At each intersection the robot will scan the walls around it.
          It will always explore the front branch first,
          then go left and back to the straight branch,
          then go right and back to the straight branch
        When we have reached a point where we are surrounded by walls OR all explored nodes, we traverse a
        path to the next unexplored node
  Have to remember all of this is in a while loop
*/
void maze_traversal() {
  /*If we are at an intersection*/
  if (atIntersection_avg()) {
    /*stop so we have time to think*/
    halt();
    /*update the robots position and the surrounding coordinates*/
    update_position();
    /*scan walls update GUI*/
    scan_walls();
    rf();
    /*push the surrounding unvisited nodes to the stack*/
    push_unvisited();
    /*if the stack is NOT empty*/
    if (!stack.isEmpty()) {
      /*if v is explored we don't care so get it off the stack*/
      while (maze[v.x][v.y].explored) {
        /*Coordinate v is the coordinate the robot is about to visit*/
        v = stack.pop();
      }
      /*find a path to v and traverse it*/
      find_path(v);
      traverse_path(path);
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

  /*Don't do anything unless you hear 660Hz OR we manually override via pressing the shiny red button*/
  while (!detects_audio || !button_pressed) {
    audio_detection();
  }
  /*Setup Maze information accordingly for GUI*/
  robot_start();
}


/*Run main code*/
void loop() {
  maze_traversal();
}


