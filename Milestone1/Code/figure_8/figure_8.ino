#include <Servo.h>

Servo servo_left;
Servo servo_right;

int sensor_left = A3;
int sensor_middle = A4;  
int sensor_right = A5; 


int count = 0;
int left_threshold = 300;
int middle_threshold = 300;
int right_threshold = 300;

void setup() {
  servo_setup();
  Serial.begin(9600);
}

void loop() {
  Serial.println(analogRead(sensor_left));
  Serial.println(analogRead(sensor_middle));
  Serial.println(analogRead(sensor_right));
  Serial.println();

  if ((analogRead(sensor_right) < right_threshold) && (analogRead(sensor_left) < left_threshold) && (analogRead(sensor_middle) < middle_threshold) && (count < 4)) {
      count++;
      turn_right_90();
  } else if ((analogRead(sensor_right) < right_threshold) && (analogRead(sensor_left) < left_threshold) && (analogRead(sensor_middle) < middle_threshold) && (count > 3)) {
      if (count == 7) {
        count = -1;
      }
      count++;
      turn_left_90();
  } else { 
    if (analogRead(sensor_middle) < middle_threshold) {
      Serial.println("go1");
      Serial.println(analogRead(sensor_middle));
      go();
    }
      if (analogRead(sensor_right) < right_threshold && analogRead(sensor_left) < left_threshold && analogRead(sensor_middle) < middle_threshold) {
        Serial.println("go2");
        go();
      }
      if (analogRead(sensor_left) < left_threshold) {
        Serial.println("left");
        turn_left();
      }
      if (analogRead(sensor_right) < right_threshold) {
        Serial.println("right");
        turn_right();
      }
      if (analogRead(sensor_right) > right_threshold && analogRead(sensor_left) > left_threshold && analogRead(sensor_middle) > middle_threshold) {
        Serial.println("halt");
        halt();
    }
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

void turn_left_90() {
  servo_left.write(95);
  servo_right.write(85);
  delay(200);
  while (analogRead(sensor_left) < 500) {
    servo_left.write(90);
    servo_right.write(85);
    Serial.println("looking for black");
  }
  while (analogRead(sensor_left) > 500) {
    servo_left.write(90);
    servo_right.write(85);
    Serial.println("looking for white");
  }
  servo_left.write(90);
  servo_right.write(85);
  delay(150);
}

void turn_right() {
  servo_left.write(95);
  servo_right.write(90);
}

void turn_right_90() {
  servo_left.write(95);
  servo_right.write(85);
  delay(200);
  while (analogRead(sensor_right) < 700) {
    servo_left.write(95);
    servo_right.write(90);
    Serial.println("looking for black");
  }
  while (analogRead(sensor_right) > 700) {
    servo_left.write(95);
    servo_right.write(90);
    Serial.println("looking for white");
  }
  servo_left.write(95);
  servo_right.write(90);
  delay(150);
}
