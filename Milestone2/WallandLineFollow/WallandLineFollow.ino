#include <Servo.h> // include servo library

// A0 is being used for the IR sensor

Servo servo_left; // pin 9
Servo servo_right; // pin 10

//NEED TO CHANGE THE LOCATION OF THE RIGHT SENSOR TO THE MIDDLE OF THE ROBOT

/*Initialize the parameters to corresponding pins*/
int wall_front = A1;
int wall_right = A2;


int sensor_left = A3;
int sensor_middle = A4;
int sensor_right = A5;

/*Initialize threshold values*/
int line_thresh = 400; //if below this we detect a white line
int wall_thresh = 150; //if above this we detect a wall


/*Initializes the servo*/
void servo_setup() {
  servo_right.attach(10);
  servo_left.attach(9);
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

/*Simple left turn*/
void turn_left() {
  servo_left.write(93);
  servo_right.write(70);
}

/*Simple right turn*/
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

/*Time it takes for wheels to reach intersection from the time the sensors detect the intersection*/
void adjust() {
  go();
  delay(700); //delay value to reach specification
}

/*Turns 90 degrees to the right until the middle sensor finds a line*/
void turn_right_linetracker() {
  turn_place_right();
  delay(300); //delay to get off the line
  /*Following while loops keep the robot turning until we find the line to the right of us*/
  while (analogRead(sensor_middle) < line_thresh);
  while (analogRead(sensor_middle) > line_thresh);
}

/*Turns to the left until a middle sensor finds a line*/
void turn_left_linetracker() {
  turn_place_left();
  delay(300); //delay to get off the line
  /*Following while loops keep the robot turning until we find the line to the left of us*/
  while (analogRead(sensor_middle) < line_thresh);
  while (analogRead(sensor_middle) > line_thresh);
}

/*Returns true and turns on LED if there is a wall in front. */
bool check_front() {
  if (analogRead(A1) > wall_thresh) {
    digitalWrite(2, HIGH);
    return true;
  } else {
    digitalWrite(2, LOW);
    return false;
  }
}

/*Returns true and turns on LED if there is a wall to the right. */
bool check_right() {
  if (analogRead(A2) > wall_thresh) {
    digitalWrite(3, HIGH);
    return true;
  } else {
    digitalWrite(3, LOW);
    return false;
  }
}

/*follows the line the robot is on (robot must be on a line to work)*/
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

/*Traverses a maze via right hand wall following while line following*/
void maze_traversal() {

  /*Checks if there is a wall and turns on LED if so - allows us to see what robot is thinking*/
  check_front();
  check_right();


  /*If we are at an intersection*/
  if ((analogRead(sensor_right) < line_thresh) && (analogRead(sensor_left) < line_thresh) && (analogRead(sensor_middle) < line_thresh)) {

    /*Check if there is a wall to the right of us*/
    if (!check_right()) {
      adjust();
      turn_right_linetracker();
    }
    
    /*If there is no wall to the right of us and no wall in front of us */
    else if (!check_front()) {
      adjust(); //adjust here takes us off the intersection allowing us to linefollow
    }

    /*There IS A WALL to the right of us AND in front of us*/
    else {
      adjust();
      turn_left_linetracker();
      /*Following if statement allows for 180 degree pivots*/
      if(check_front()){
        turn_left_linetracker();
      }
    }
  }
  /*If we are not at an intersection then line follow*/
  linefollow();
}
void setup() {
  Serial.begin(115200); // use the serial port
  servo_setup();
  pinMode(2, OUTPUT); // LED to indicate whether a wall is to the front or not
  pinMode(3, OUTPUT); // LED to indicate whether a wall is to the right or not
  pinMode(7, OUTPUT); // LED to test stuff while implementing (NOT used in final version as of now)
}

void loop() {
  maze_traversal();
}
