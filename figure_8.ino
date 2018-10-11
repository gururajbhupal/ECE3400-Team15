#include <Servo.h>

Servo servo_left;
Servo servo_right;
int sensor_middle = A3; 
int sensor_left = A4; 
int sensor_right = A5; 
int count = 0;

void setup() {
  servo_setup();
  Serial.begin(9600);
  Serial.println(analogRead(sensor_left));
  Serial.println(analogRead(sensor_middle));
  Serial.println(analogRead(sensor_right));
  //delay(50000000);
}

void loop() {
  if ((analogRead(sensor_right) < 700) && (analogRead(sensor_left) < 500) && (analogRead(sensor_middle) < 850) && (count < 4)) {
      count++;
      turn_right_90();
  } else if ((analogRead(sensor_right) < 700) && (analogRead(sensor_left) < 500) && (analogRead(sensor_middle) < 850) && (count > 3)) {
      if (count == 7) {
        count = -1;
      }
      count++;
      turn_left_90();
  } else { 
    servo_left.write(95);
  servo_right.write(85);
    
//    if (analogRead(sensor_middle) < 850) {
//      Serial.println("go1");
//      Serial.println(analogRead(sensor_middle));
//      go();
//    }
//      if (analogRead(sensor_right) < 500 && analogRead(sensor_left) < 500 && analogRead(sensor_middle) < 850) {
//        Serial.println("go2");
//        go();
//      }
//      if (analogRead(sensor_left) < 500) {
//        Serial.println("left");
//        turn_left();
//      }
//      if (analogRead(sensor_right) < 600) {
//        Serial.println("right");
//        turn_right();
//      }
//      if (analogRead(sensor_right) > 500 && analogRead(sensor_left) > 500 && analogRead(sensor_middle) > 850) {
//        Serial.println("halt");
//        halt();
//    }
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

void turn_right_90() {
  delay(1500);
  servo_left.write(95);
  servo_right.write(95);
  delay(1100);
}

void turn_left_90() {
  delay(1500);
  servo_left.write(85);
  servo_right.write(85);
  delay(1100);
}

void turn_right() {
  servo_left.write(95);
  servo_right.write(90);
}

