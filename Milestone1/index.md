# Introduction

## Setting Up Sensors

To aid the robot in navigating the grid, we used 3 line sensors. Due to shadows and that not all 3 sensors are the exact same height of the ground, each of the sensors has a different threshold to determine if it is above white or black. Using the following code, we put the sensors over white and black surfaces and reset the Uno to determine the thresholds without being spammed by constant readings.


![SensorCalibrationCode](https://github.com/gururajbhupal/ECE3400-Team15/edit/master/Milestone1/Media/SensorCalibrationCode.png)

## Line Following

To have the robot follow the line, we looped over 5 different cases for the 3 line sensors. The three line sensors are all in a row at the front of the robot. The middle sensor is supposed to be over white and the two outside sensors should be to the left and right of the line and sensing black. 


![FollowLineCode](https://github.com/gururajbhupal/ECE3400-Team15/edit/master/Milestone1/Media/FollowLineCode.png)

The logic behind the implementation is that if the central sensor is on white and the two outside sensors are on black, the robot should go straight. If all three sensors are white, then the robot is traversing over a junction and should continue to go straight. If the left sensor senses white, the robot is too far to the right and so the robot adjusts by turning slightly left. If the right sensor senses white, the robot is too far to the left and so the robot adjusts by turning slightly right. The final case is that if all sensors sense black, then the robot stops. This indicates that the robot has reaches the end of the line or that it is completely off the line. The functions for turning left, turning right, moving forward, and stopping are below. 


![MoveCommandCode](https://github.com/gururajbhupal/ECE3400-Team15/edit/master/Milestone1/Media/MoveCommandCode.png)

[![FollowLine](http://img.youtube.com/vi/6EOPY7VUni4/0.jpg)](http://www.youtube.com/watch?v=6EOPY7VUni4)

## Traversing Grid in a Figure 8

To traverse the grid in a figure 8, we used our line following code in addition to new figure 8 logic. For the robot to go in a figure 8, it must make 4 right turns followed by 4 left turns. If all three sensors are over white, then the robot know it must turn left or right because the robot is at a junction. The robot keeps track of whether to make a left turn or a right turn using the count variable. If count is between 0 and 3, then it will turn right, if it is between 4 and 7 it will turn left. Once count gets to 7, count resets it to 0.


![FigureEightCode](https://github.com/gururajbhupal/ECE3400-Team15/edit/master/Milestone1/Media/FigureEightCode.png)

Once we determine which turn to make, we increment or reset count depending on the case, and then tell the robot to either turn left or right 90 degrees. The two implementations of turning right and left 90 degrees are shown below. As our robot turns, the sensor on the side it is turning will initially be over white when it enters the intersection, then black as it turns, and then white again once it has finished the turn. Our implementations tells the robot to move forward for 0.2 seconds and then to turn until it reaches white for the second time and then for 0.15 seconds more.  

![TurnRightCode](https://github.com/gururajbhupal/ECE3400-Team15/edit/master/Milestone1/Media/TurnRightCode.png)

![TurnLeftCode](https://github.com/gururajbhupal/ECE3400-Team15/edit/master/Milestone1/Media/TurnLeftCode.png)

Once the robot finishes executing a 90 degree turn, it resume running the line following code until it reaches another junction. Here is a video of our robot successfully traversing 2 figure 8s on the grid.

[![FigureEight](http://img.youtube.com/vi/swSXyXTXP5s/0.jpg)](http://www.youtube.com/watch?v=swSXyXTXP5s)
