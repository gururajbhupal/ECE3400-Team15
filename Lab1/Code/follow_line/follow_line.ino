#include <Servo.h>

Servo servo_left;
Servo servo_right;
int sensor_left = A3;
int sensor_middle = A4;
int sensor_right = A5;

int left_threshold = 100;
int middle_threshold = 100;
int right_threshold = 100;

void setup() {
  servo_setup();
  Serial.begin(9600);
}

void loop() {
  Serial.println(analogRead(sensor_left));
  Serial.println(analogRead(sensor_middle));
  Serial.println(analogRead(sensor_right));
  Serial.println();
  if (analogRead(sensor_middle) < middle_threshold) {
    go();
  }
  if (analogRead(sensor_right) < right_threshold && analogRead(sensor_left) < left_threshold && analogRead(sensor_middle) < middle_threshold) {
    go();
  }
  else if (analogRead(sensor_left) < left_threshold) {
    turn_left();
  }
  else if (analogRead(sensor_right) < right_threshold) {
    turn_right();
  }
  if (analogRead(sensor_right) > right_threshold && analogRead(sensor_left) > left_threshold && analogRead(sensor_middle) > middle_threshold) {
    halt();
  }
}

void servo_setup() {
  servo_right.attach(10);
  servo_left.attach(9);
  servo_left.write(90);
  servo_right.write(90);
}

void go() {
  servo_left.write(95);
  servo_right.write(85);
}

void halt() {
  servo_left.write(90);
  servo_right.write(90);
}

void turn_left() {
  servo_left.write(90);
  servo_right.write(85);
}

void turn_right() {
  servo_left.write(95);
  servo_right.write(90);
}
