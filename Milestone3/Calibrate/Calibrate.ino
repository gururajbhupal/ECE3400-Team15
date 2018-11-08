#include <Servo.h> // include servo library

/*Simple function so we can calibrate our wheels to sensors when we change distance*/

Servo servo_left; // pin 9
Servo servo_right; // pin 10
int wall_front = A1;
int wall_right = A2;
int sensor_left = A3;
int sensor_middle = A4;
int sensor_right = A5;

int line_thresh = 400;
int wall_thresh = 150;

void servo_setup() {
  servo_right.attach(10);
  servo_left.attach(9);
  servo_left.write(90);
  servo_right.write(90);
}

void go() {
  servo_left.write(100);
  servo_right.write(80);
}

void halt() {
  servo_left.write(90);
  servo_right.write(90);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // use the serial port
  servo_setup();
  
  go();
  delay(700); //THIS IS THE DELAY FOR THE WHEELS TO REACH THE SENSOR
  halt();
}

void loop() {
  // put your main code here, to run repeatedly:

}
