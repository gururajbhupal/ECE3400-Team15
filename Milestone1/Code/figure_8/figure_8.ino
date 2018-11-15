#include <Servo.h>

Servo servo_left;
Servo servo_right;
int sensor_middle = A3;
int sensor_left = A4;
int sensor_right = A5;
int count = 0;


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
  delay(600); //delay value to reach specification
}

void setup() {
  servo_setup();
  Serial.begin(9600);
}

/*Turns to the right until the middle sensor finds a line (allows for 90 degree turns)*/
void turn_right_linetracker() {
  turn_place_right();
  delay(300); //delay to get off the line
  /*Following while loops keep the robot turning until we find the line to the right of us*/
  while (analogRead(sensor_middle) < line_thresh);
  while (analogRead(sensor_middle) > line_thresh);
}


/*Turns to the left until a middle sensor finds a line (allows for 90 degree turns)*/
void turn_left_linetracker() {
  turn_place_left();
  delay(300); //delay to get off the line
  /*Following while loops keep the robot turning until we find the line to the left of us*/
  while (analogRead(sensor_middle) < line_thresh);
  while (analogRead(sensor_middle) > line_thresh);
}

void loop() {
  if ((analogRead(sensor_right) < 700) && (analogRead(sensor_left) < 500) && (analogRead(sensor_middle) < 850) && (count < 4)) {
    count++;
    turn_right_linetracker();
  } else if ((analogRead(sensor_right) < 700) && (analogRead(sensor_left) < 500) && (analogRead(sensor_middle) < 850) && (count > 3)) {
    if (count == 7) {
      count = -1;
    }
    count++;
    turn_left_linetracker();
  } else {
    servo_left.write(95);
    servo_right.write(85);
  }
}
