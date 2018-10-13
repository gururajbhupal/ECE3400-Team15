#define LOG_OUT 1 // use the log output function
#define FFT_N 256 // set to 256 point fft

//#include <FFT.h> // include the library
#include <Servo.h> // include servo library

// A0 is being used for the IR sensor
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

void turn_left() {
  servo_left.write(93);
  servo_right.write(70);
}

void turn_right() {
  servo_left.write(110);
  servo_right.write(87);
}

void turn_place() {
  servo_left.write(80);
  servo_right.write(80);
}

void turn_left_90() {
  servo_left.write(95);
  servo_right.write(85);
  delay(700);
  while (analogRead(sensor_left) < line_thresh) {
    servo_left.write(90);
    servo_right.write(85);
    Serial.println("looking for black");
  }
  delay(700);
  while (analogRead(sensor_middle) > line_thresh) {
    servo_left.write(90);
    servo_right.write(85);
    Serial.println("looking for white");
  }
  servo_left.write(90);
  servo_right.write(85);
  delay(150);
}

void turn_right_90() {
  servo_left.write(95);
  servo_right.write(85);
  delay(700);
  while (analogRead(sensor_right) < line_thresh) {
    servo_left.write(95);
    servo_right.write(90);
    Serial.println("looking for black");
  }
  delay(700);
  while (analogRead(sensor_middle) > line_thresh) {
    servo_left.write(95);
    servo_right.write(90);
    Serial.println("looking for white");
  }
  servo_left.write(95);
  servo_right.write(90);
  delay(250);
}


void fft_setup() {
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADMUX = 0x40; // use adc0  (analog pin A0)
  DIDR0 = 0x01; // turn off the digital input for adc0
}
//
//void fft() {  
//  int temp = ADCSRA; // set temp the adc prev val
//  ADCSRA = 0xe5; // set the adc to free running mode
//  int fft_length = 1000;
//  while(fft_length > 0) { // reduces jitter
//    Serial.println("herererererere");
//    cli();  // UDRE interrupt slows this way down on arduino1.0
//    for (int i = 0 ; i < 512 ; i += 2) { // save 256 samples
//      while(!(ADCSRA & 0x10)); // wait for adc to be ready
//      ADCSRA = 0xf5; // restart adc
//      byte m = ADCL; // fetch adc data
//      byte j = ADCH;
//      int k = (j << 8) | m; // form into an int
//      k -= 0x0200; // form into a signed int
//      k <<= 6; // form into a 16b signed int
//      fft_input[i] = k; // put real data into even bins
//      fft_input[i+1] = 0; // set odd bins to 0
//    }
//    fft_window(); // window the data for better frequency response
//    fft_reorder(); // reorder the data before doing the fft
//    fft_run(); // process the data in the fft
//    fft_mag_log(); // take the output of the fft
//    sei();
//    
//    for (byte i = 0 ; i < FFT_N/2 ; i++) { 
//      //Serial.println("start");
//      //Serial.println(fft_log_out[i]); // send out the data
//      if (i == 43) Serial.println(fft_log_out[i]);
//      if (i == 43 && fft_log_out[i] > 125) {
//        Serial.println("starrrrrrrrrrrrrrrrrrrrrrrrrrrrrrt");
//        digitalWrite(7, HIGH);
//        halt();   
//      }
//      if (i == 43 && fft_log_out[i] < 125) {
//        //go();
//        //digitalWrite(7, LOW);
//      }
//    }
//    fft_length = fft_length-1;
//  }
//  ADCSRA = temp; // set the adc to NOT free running mode
//}


bool check_front() {
  if (analogRead(A1) > wall_thresh) { // then there is a wall
    return true;
  } else {
    return false;
  }
}

bool check_right() {
  if (analogRead(A2) > wall_thresh) { // then there is a wall
    return true;
  } else {
    return false;
  }
}

void drive() {
  if ((analogRead(sensor_right) < line_thresh) && (analogRead(sensor_left) < line_thresh) && (analogRead(sensor_middle) < line_thresh)) {
  // checks to see if at an intersection
      if (!check_right()) {
        turn_right_90();
        digitalWrite(3, LOW); // no wall to right
      } else {
        digitalWrite(3, HIGH); // wall to the right
        while (check_front) {
          digitalWrite(2, HIGH); // wall in front
          turn_left_90();
        }
        digitalWrite(2, HIGH); // no wall in front
      }
  } else { 
    if ((analogRead(sensor_right) < line_thresh) && (analogRead(sensor_left) < line_thresh) && (analogRead(sensor_middle) < line_thresh)) {
      turn_left_90();
    }
    if (analogRead(sensor_middle) < line_thresh) {
      Serial.println("go1");
      //Serial.println(analogRead(sensor_middle));
      go();
    }
    if (analogRead(sensor_left) < line_thresh) {
      Serial.println("left");
      turn_left();
    }
    if (analogRead(sensor_right) < line_thresh) {
      Serial.println("right");
      turn_right();
    }
    if (analogRead(sensor_right) > line_thresh && analogRead(sensor_left) > line_thresh && analogRead(sensor_middle) > line_thresh) {
      Serial.println("halt");
      halt();
    }
  }
}

void setup() {
  Serial.begin(115200); // use the serial port
  servo_setup();
  //fft_setup();
  pinMode(7, OUTPUT); // LED to indicate whether reading IR or not
  pinMode(2, OUTPUT); // LED to indicate whether a wall is to the front or not
  pinMode(3, OUTPUT); // LED to indicate whether a wall is to the right or not
}

void loop() {
  Serial.println(analogRead(sensor_left));
//  Serial.println(analogRead(sensor_middle));
//  Serial.println(analogRead(sensor_right));
  if (check_front()) digitalWrite(3, HIGH);
  else digitalWrite(3, LOW);
  if (check_right()) digitalWrite(2, HIGH);
  else digitalWrite(2, LOW);
  if ((analogRead(sensor_right) < line_thresh) && (analogRead(sensor_left) < line_thresh) && (analogRead(sensor_middle) < line_thresh)) {
    if (!check_right()) { 
      //fft();
      turn_right_90();
    } else if (!check_front()) {
      //fft();
      delay(500);
    } else {
      //fft();
      go();
      while (analogRead(sensor_left) < line_thresh);
      delay(600);
      //digitalWrite(7, HIGH);
      turn_place();
      delay(750);
      while (check_front()) {
        while (analogRead(sensor_right) < line_thresh);
        while (analogRead(sensor_right) > line_thresh);
      }
      //while (check_front() && analogRead(sensor_middle) > line_thresh);
      servo_left.write(90);
      servo_right.write(90);
    }
    //digitalWrite(7, LOW);
  }
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
  //fft();
  // drive();
}
