#define LOG_OUT 1 // use the log output function
#define FFT_N 256 // set to 256 point fft

#include <FFT.h> // include the library
#include <Servo.h> // include servo library

// A0 is being used for the IR sensor
Servo servo_left; // pin 9
Servo servo_right; // pin 10
int wall_front = A1;
int wall_right = A2;
int sensor_left = A4;
int sensor_middle = A3; 
int sensor_right = A5; 

void servo_setup() {
  servo_right.attach(10);
  servo_left.attach(9);
  servo_left.write(90);
  servo_right.write(90);
}

void go() {
  servo_left.write(110);
  servo_right.write(70);
}

void halt() {
  servo_left.write(90);
  servo_right.write(90);
}

void turn_left() {
  servo_left.write(90);
  servo_right.write(70);
}

void turn_right() {
  servo_left.write(100);
  servo_right.write(90);
}

void turn_right_90() {
  delay(1500); // goes forward until the wheels are in line with the intersetion
  servo_left.write(95);
  servo_right.write(95);
  delay(600); // should be half way through turn when it starts to looks for a white line
  while (analogRead(sensor_right) > 700) && (analogRead(sensor_left) > 500) && (analogRead(sensor_middle) > 850);
  delay(150); //straightens out a little bit
  // while all three sensors see black
  // may have to mess around with these thresholds
}

void turn_left_90() {
  delay(1500); // goes forward until the wheels are in line with the intersetion
  servo_left.write(85);
  servo_right.write(85);
  delay(600); // should be half way through turn when it starts to looks for a white line
  while (analogRead(sensor_right) > 700) && (analogRead(sensor_left) > 500) && (analogRead(sensor_middle) > 850);
  delay(150); //straightens out a little bit
  // while all three sensors see black
  // may have to mess around with these thresholds
}

void fft_setup() {
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADMUX = 0x40; // use adc0  (analog pin A0)
  DIDR0 = 0x01; // turn off the digital input for adc0
}

void fft() {  
  int temp = ADCSRA; // set temp the adc prev val
  ADCSRA = 0xe5; // set the adc to free running mode
  int fft_length = 10;
  while(fft_length > 0) { // reduces jitter
    cli();  // UDRE interrupt slows this way down on arduino1.0
    for (int i = 0 ; i < 512 ; i += 2) { // save 256 samples
      while(!(ADCSRA & 0x10)); // wait for adc to be ready
      ADCSRA = 0xf5; // restart adc
      byte m = ADCL; // fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m; // form into an int
      k -= 0x0200; // form into a signed int
      k <<= 6; // form into a 16b signed int
      fft_input[i] = k; // put real data into even bins
      fft_input[i+1] = 0; // set odd bins to 0
    }
    fft_window(); // window the data for better frequency response
    fft_reorder(); // reorder the data before doing the fft
    fft_run(); // process the data in the fft
    fft_mag_log(); // take the output of the fft
    sei();
    Serial.println("start");
    for (byte i = 0 ; i < FFT_N/2 ; i++) { 
      Serial.println(fft_log_out[i]); // send out the data
      if (i == 43 && fft_log_out[i] > 125) {
        digitalWrite(7, HIGH);
      }
      if (i == 43 && fft_log_out[i] < 125) {
        digitalWrite(7, LOW);
      }
    }
  }
  ADCSRA = temp; // set the adc to NOT free running mode
}


bool check_front() {
  if (analogRead(A1) > 100) { // then there is a wall
    return true;
  } else {
    return false;
  }
}

bool check_right() {
  if (analogRead(A2) > 100) { // then there is a wall
    return true;
  } else {
    return false;
  }
}

void drive() {
  if ((analogRead(sensor_right) < 700) && (analogRead(sensor_left) < 500) && (analogRead(sensor_middle) < 850)) {
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
    if (analogRead(sensor_middle) < 850) {
      Serial.println("go1");
      Serial.println(analogRead(sensor_middle));
      go();
    }
      if (analogRead(sensor_right) < 500 && analogRead(sensor_left) < 500 && analogRead(sensor_middle) < 850) {
        Serial.println("go2");
        go();
      }
      if (analogRead(sensor_left) < 500) {
        Serial.println("left");
        turn_left();
      }
      if (analogRead(sensor_right) < 600) {
        Serial.println("right");
        turn_right();
      }
      if (analogRead(sensor_right) > 500 && analogRead(sensor_left) > 500 && analogRead(sensor_middle) > 850) {
        Serial.println("halt");
        halt();
    }
  }
}

void setup() {
  Serial.begin(115200); // use the serial port
  servo_setup();
  fft_setup();
  pinMode(7, OUTPUT); // LED to indicate whether reading IR or not
  pinMode(2, OUTPUT); // LED to indicate whether a wall is to the front or not
  pinMode(3, OUTPUT); // LED to indicate whether a wall is to the right or not
}

void loop() {
  fft();
  drive();
}
