#define FFT_N 256 // set to 256 point fft
#define LOG_OUT 1 // use the log output function

#include <Servo.h> // include servo library
#include <FFT.h> // include the library

// A0 is being used for the IR sensor

Servo servo_left; // pin 9
Servo servo_right; // pin 10

/*boolean which is true if a robot has been detected, false otherwise*/
bool sees_robot = false;

/*Initialize the parameters to corresponding pins*/
int wall_front = A1;
int wall_right = A2;


int sensor_left = A3;
int sensor_middle = A4;
int sensor_right = A5;

/*Initialize threshold values*/
int line_thresh = 400; //if below this we detect a white line
int wall_thresh = 150; //if above this we detect a wall
int IR_threshold = 160; //if above this we detect IR hat


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

/*follows the line the robot is on, else halts if all three sensors are not on a line*/
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

/*Sets sees_Robot to true if there is a robot, else sees_robot = false*/
void IR_detection() {

  /*Set temporary values for relevant registers*/
  int t1 = TIMSK0; 
  int t2 = ADCSRA; 
  int t3 = ADMUX; 
  int t4 = DIDR0; 

  /*Set register values to required values for IR detection*/
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0x40; // use adc0
  DIDR0 = 0x01; // turn off the digital input for adc0

  cli();  // UDRE interrupt slows this way down on arduino1.0
  for (int i = 0 ; i < 512 ; i += 2) { // save 256 samples
    while (!(ADCSRA & 0x10)); // wait for adc to be ready
    ADCSRA = 0xf5; // restart adc
    byte m = ADCL; // fetch adc data
    byte j = ADCH;
    int k = (j << 8) | m; // form into an int
    k -= 0x0200; // form into a signed int
    k <<= 6; // form into a 16b signed int
    fft_input[i] = k; // put real data into even bins
    fft_input[i + 1] = 0; // set odd bins to 0
  }
  fft_window(); // window the data for better frequency response
  fft_reorder(); // reorder the data before doing the fft
  fft_run(); // process the data in the fft
  fft_mag_log(); // take the output of the fft
  sei();
  Serial.println("start");
  for (byte i = 0 ; i < FFT_N / 2 ; i++) {
    Serial.println(fft_log_out[i]); // send out the data
    if (i == 43 && fft_log_out[i] > IR_threshold) {
      digitalWrite(7, HIGH);
      sees_robot = true;
    }
    if (i == 43 && fft_log_out[i] < IR_threshold) {
      digitalWrite(7, LOW);
      sees_robot = false;
    }
  }

  /*Restore the register values*/
  TIMSK0 = t1;
  ADCSRA = t2;
  ADMUX = t3;
  DIDR0 =  t4;
}

/*Returns true if the robot is at an intersection, else false*/
void atIntersection(){
  if((analogRead(sensor_right) < line_thresh) && (analogRead(sensor_left) < line_thresh) && (analogRead(sensor_middle) < line_thresh)){
    return true;
  }
  else{
    return false;
  }
}

/*Traverses a maze via right hand wall following while line following*/
void maze_traversal() {

  /*Checks if there is a wall and turns on LED if so - allows us to see what robot is thinking*/
  check_front();
  check_right();

  /*If there is a robot avoid it!!*/
  if (sees_robot) {
    /*Turn right until we see a line*/
    turn_right_linetracker();
    /*Ensure that the line we pick up isn't gonna run us into a wall*/
    while (check_front()) {
      turn_right_linetracker();
    }
  }
  
  /*If there is NO ROBOT then traverse the maze via right hand wall following*/
  else {
    /*If we are at an intersection*/
    if (atIntersection()) {

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
        /*Following if statement allows for robot to turn around at dead end*/
        if (check_front()) {
          turn_left_linetracker();
        }
      }
    }
    /*If we are not at an intersection then line follow*/
    linefollow();
  }
}

/*Set up necessary stuff*/
void setup() {
  Serial.begin(115200); // use the serial port
  servo_setup(); //setup the servo
  pinMode(2, OUTPUT); // LED to indicate whether a wall is to the front or not
  pinMode(3, OUTPUT); // LED to indicate whether a wall is to the right or not
  pinMode(7, OUTPUT); // LED to indicate whether there is a robot in front of us
}

/*Main code to run*/
void loop() {
  IR_detection(); //update sees_robot
  maze_traversal(); //traverse the maze
}
