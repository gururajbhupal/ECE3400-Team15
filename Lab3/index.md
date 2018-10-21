# Lab 3 Report

## Introduction
In this lab we integrated many of the components we had working in the previous labs (including IR sensor, audio sensor, line sensors, and wall sensors) to have our robot start tracking the maze after a 660 Hz tone. Additionally, our robot sent information wirelessly to a base station which displayed the information on the screen using a GUI.

## Implementation

The first thing we did was to start the robot once a 660 Hz tone is played. To do this, we used our code from lab 2 and we also added a variable detects_audio that indicates if we have heard the signal or not. Our code is below.

<img src="audio_detect.png" alt="audio_detect" width="250"/>

To implement this with out full code, we used a spin lock that would only allow the robot to start traversing the maze once the tone was played. In this code, we loop while we have not yet heard the audio signal. In the loop we check for the audio signal.

<img src="Media/spinlock.png" alt="spinlock" width="250"/>

One problen we had with this was that powering the wall sensors with the same power as the amplifier that pwer the microphone signal caused a lot of noise that prevented us from distinguishing the 660 Hz tone from noise. To fix this, we add a second power source just to power the audio signal amplifier.

The following video shows the robot starting on a 660Hz tone and exploring the entire maze and changing a path if it sees another robot.

<!-- video of robot goin and doin its thing -->

## Testing
