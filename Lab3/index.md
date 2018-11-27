# Lab 3 Report


## Introduction
In this lab, we integrated many of the components we had working in the previous labs (including IR sensor, audio sensor, line sensors, and wall sensors) to have our robot start tracking the maze after a 660 Hz tone. Additionally, our robot sent information wirelessly to a base station which displayed the information on the screen using a GUI.  

## Starting on a 660 Hz Tone 

The first thing we did was to start the robot once a 660 Hz tone is played. To do this, we used our code from lab 2 and we also added a variable detects_audio that indicates if we have heard the signal or not. Our code is below.

```
/*
Sets mux_select to audio information, and sets detects_audio to true if we detect a 660Hz signal. mux_select is then set to an empty signal on the mux to avoid noise from FFT interfering with servos.
*/
void audio_detection() {
  mux_select(0, 0, 0); //select correct mux output

  /*Set temporary values for relevant registers*/
  int temp1 = TIMSK0;
  int temp2 = ADCSRA;
  int temp3 = ADMUX;
  int temp4 = DIDR0;

  ... fft code same as IR

  /*When audio is detected, detects_audio is set to true. Once detected
    this value is never set back to false*/
  for (byte i = 0; i < FFT_N / 2; i++) {
    if (i == 5 && fft_log_out[i] > 135) {
      detects_audio = true;
    }
  }

  /*Restore the register values*/
  TIMSK0 = temp1;
  ADCSRA = temp2;
  ADMUX = temp3;
  DIDR0 =  temp4;

  mux_select(0, 1, 0); //SET TO BLANK OUTPUT TO AVOID FFT NOISE WITH SERVOS
}
```

## Starting on a 660 Hz Tone and exploring the entire maze  

To implement this with out full code, we used a spin lock that would only allow the robot to start traversing the maze once the tone was played. In this code, we loop while we have not yet heard the audio signal. 

```
/*Main code to run*/
void loop() {
  ...
  while (!detects_audio) {
    audio_detection();
  }
  ...
}
```

One problem we had with this was that powering the wall sensors with the same power as the amplifier that power the microphone signal caused a lot of noise that prevented us from distinguishing the 660 Hz tone from noise. To fix this, we add a second power source just to power the audio signal amplifier. To make sense of the way we mux select see the mux implementation below.

### Exploring the Maze, Avoiding Robots, Ignoring Decoys - All on 660Hz
 
<iframe width="560" height="315" src="https://www.youtube.com/embed/cb1B5bx-IMQ" frameborder="0" allow="autoplay; encrypted-media" allowfullscreen></iframe>

## Data Scheme for Storing Information
The data scheme involved defining a protocol for storage on the Robot and transfer of maze information from the robot to the base station. 
We tried to minimize the size of the data type in this protocol by defining most of the information on bit level. 
This helps in 2 aspects:   
* Memory requirement to store the data is less. 
* Data size is less, hence processing will be faster and hence helps to decrease latency during data transmission.

Our Data Protocol organization is as follows.

### Nibble 1 
[0:3] - Robot x co-ordinate (Range is 0-8 since its a 9x9 matrix) 

### Nibble 2 
[4:7] - Robot y co-ordinate (Range is 0-8 since its a 9x9 matrix) 

### Nibble 3
[8] - West Wall(0:no wall; 1:wall exists) 

[9] - North Wall(0:no wall; 1:wall exists) 

[10] - East Wall(0:no wall; 1:wall exists) 

[11] - South Wall(0:no wall; 1:wall exists)

### Nibble 4
[12:14] - Treasure Type (Predetermined treasure type codes as below) 

No Treasure = 0

Blue Triangle = 1 | Blue Square = 2 | Blue Diamond = 3

Red Triangle = 4 | Red Square = 5 | Red Diamond = 6 

[15] - Opponent Robot ( 0 - no Robot exists ; 1 - Robot obstructing path ) 


By this, we are able to send all the data using just 2 bytes of which can be easily stored as an integer. If we want to introduce a “data data_bar” scheme for more redundancy for data correction, we can still implement the whole protocol in an unsigned long. But the data was pretty clean over the channel, so we did not care to implement more redundancy.  

### Storing Visited Locations
Our current implementation for storing visited locations is to initialize a ixj boolean 2d array which sets the i,j index to true if it has been visited. This might not be the most efficient way to do this and we will look to optimize this in the future.

## Sending Information Wirelessly Between Arduinos
In order to get the Arduinos to communicate wirelessly using the RF chips, we studied and experimented with the given GettingStarted.ino file to find out which parts were necessary to transmit and receive information and which parts were extraneous to our objective for this lab. The configuration settings in our code were drawn heavily from the given configuration settings, with the only changes being an increased power and data rate and a decreased payload size for more reliability. Since the robot was always sending information and the base station was always receiving it, we found the role switching capabilities of the original code to be unnecessary. Both pipes for reading and writing were set from the beginning and we found no need for the base station to send information for the robot to receive beyond an acknowledgment of a successfully received message. As a result, the final RF code used for this lab was significantly cut down in length while still remaining functional.


## Updating the GUI from Virtual Robot connected to Base Station

Once the robot sends the base station a message with all of the information encoded in the format described above, the base station decodes the message with the use of masking and bit shifting in order to extract information from specific bits. The base station iterates over the bits of the message and prints (without newlines) the information contained within them. For example, if our robot detects another robot, bit 15 will be set to 1 and the base station will print “,robot=true”. Once the message has been fully parsed and interpreted, it prints a new line so that the GUI will receive the information and update accordingly. To ensure our communication protocol works as intended, we created a random 3x3 maze and queued up the messages a virtual robot would send if it traversed the maze.

 

The two videos below show data transmission from Arduino1 → Arduino 2 and Arduino2 using the data to display the information on the GUI.  
The first video is of us initializing the arduinos.  
The second video is a screen recording of the GUI from the base station.  
Note: This is a virtual demo to ensure we understood the GUI and RF implementation.

### Demo of robot-to-gui integration
<iframe width="560" height="315" src="https://www.youtube.com/embed/FKI-ZMED-DY" frameborder="0" allow="autoplay; encrypted-media" allowfullscreen></iframe>


### Screen recording of GUI
<iframe width="560" height="315" src="https://www.youtube.com/embed/kc94y1iIkes" frameborder="0" allow="autoplay; encrypted-media" allowfullscreen></iframe>  


## Systems Integration  
Now that we have a robot that can traverse the maze via right-hand wall following (while line following), starts on a 660Hz tone, and radio communications working it's time to integrate everything.

This called for a rework of our wiring. The first of which was to free up digital pins for the radio transmitter, and the second of which was to free up analog pins for sensors later down the line. 

### Adding a Mux
We decided to implement a mux to switch between our analog inputs. The mux currently has our audio and IR sensors as well as all three of our wall sensors (a left wall sensor was added to map the maze correctly at some point). To switch the mux output we have a function *mux_select(int s2, int s1, int s0)* we wrote to switch between inputs. Inputs that aren't used are grounded.

Each of the signals input into the mux has a corresponding function: *check_front()*, *check_left()*, *check_right()* *IR_detection()*, *audio_detection()*. Each of these functions calls our mux select function with the corresponding parameters.

AN IMPORTANT NOTE: For reasons we can't yet explain, our audio and IR detection code (which runs the FFT code) severely interferes with our servos operation. We got past this in lab 2 with temp variables but that doesn't seem to work anymore. We debugged it down to the fact that when we call either the IR or audio detection code the mux never resets and so just that signal is going into the analog pin and the noise from that signal is causing issues. This is just our hypotheses and may not actually be the case. To get around this both *audio_detection()* and *IR_detection()* set the mux to output an empty signal so that when the maze traversal code was running there would be no audio or IR signal going into the board. This solved a lot of our issues.

### Adding the Radios
Once the mux was setup we disconnected most of our LEDS to free up digital pin slots, then plugged the radio transmitter in. Our main code was now in need of an overhaul to implement radio communication during maze traversal. To do this we added three new functions:
* *rf()*
* *update_position()*
* *scan_walls()*

The global variables x, y, and heading were also added. 
* x and y are the current position of the robot
* heading is the direction the robot is facing (initally south to conform to GUI)


Small Side Note: There was a slight issue with how we initally mapped. The GUI is rows by columns which is y,x but we map based on x,y! This had to be changed in our initial implementation. Don't make the same mistake as us.

A snippet of each function is shown below

```
/*Turns the information into data and start listening*/
void rf() {
  /* format info into data*/
  data = data | y;
  data = data | x << 4;

  radio.stopListening();
  bool ok = radio.write( &data, sizeof(unsigned int) );

  /*Now, continue listening*/
  radio.startListening();

  /*Clear the data*/
  data = data & 0x0000; 
}
```

```
/*Updates the position of the robot assuming that the starting position is
  the bottom right facing north*/
void update_position() {
  switch (heading) {
    case 0:
      y++;
      break;
    case 1:
      x++;
      break;

  ... rest of cases
  }
}
```

```
/*Scans the walls and updates data accordingly*/
void scan_walls() {
  switch (heading) {
    case 0: // north
      if (check_left()) data = data | 0x0100; // west=true
      if (check_front()) data = data | 0x0200; // north=true
      if (check_right()) data = data | 0x0400; // east=true
      break;

  ... Rest of cases
  }
}
```

Now in our function *maze_traversal()* we just call *update_position()*, *scan_walls()*, and *rf()* accordingly!
A small snippet of *maze_traversal()* is shown below.

```
/*Traverses a maze via right hand wall following while line following. Updates GUI via radio communication*/
void maze_traversal() {
 ... rest of code

    if (atIntersection()) {
      /*Check if there is a wall to the right of us*/
      if (!check_right()) {
        Serial.println("turning right");
        adjust();
        update_position();
        scan_walls();
        rf();
        turn_right_linetracker();
      }

 ... rest of code
}
```

## Systems Integration Demo

With everything set up its time to show this baby off! 

The following video shows the information the base station received from the robot. It is in the format that is required for the maze GUI to update properly. Each line accurately represents the robot's observations on each tile of the maze, as well as the walls. The robot started in the top left corner of the maze (0, 0) while facing downwards. However, it could start from any corner of the maze and map it properly!

<iframe width="560" height="315" src="https://www.youtube.com/embed/xTH5_UQPLGo" frameborder="0" allow="autoplay; encrypted-media" allowfullscreen></iframe> 


<iframe width="560" height="315" src="https://www.youtube.com/embed/U0MhqqGQsVE" frameborder="0" allow="autoplay; encrypted-media" allowfullscreen></iframe> 

## Conclusion
We have a lot of stuff to do and a lot of things to refine and fix (like how do we map the maze if we run into a robot?!). Expect some exciting refinements at a lab near you.
