#include <Servo.h>

Servo servo_left;
Servo servo_right;

int sensor_left = A3;
int sensor_middle = A4;
int sensor_right = A5;

int line_thresh = 400; //if BELOW this we detect a white line

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

void setup() {
  servo_setup();
  Serial.begin(9600);
}

void loop() {
  linefollow();
}
