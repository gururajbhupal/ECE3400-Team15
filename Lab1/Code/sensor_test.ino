#include <Servo.h>

int sensor_middle = A3; 
int sensor_left = A4; 
int sensor_right = A5; 

void setup() {
  Serial.begin(9600);
  Serial.println(analogRead(sensor_left));
  Serial.println(analogRead(sensor_middle));
  Serial.println(analogRead( sensor_right));
}

void loop() {
  delay(1000000);
}
