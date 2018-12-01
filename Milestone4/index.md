#Milestone 4 Report

## Introduction
For this milestone, we added shape detection to our camera. There are  6 types of treasures that have to be detected in the competition.


## Implementation 
The 6 treasure types that need to be detected are as below. 
Blue Triangle 
Blue Square 
Blue Diamond 
Red Triangle 
Red Square 

Based on this we have to compute several parameters . 

Presence of the  treasure 
Color of the treasure 
Shape of the treasure

Shape detection is one of the toughest challenges we had to face in among all the milestones. 
Majorly because of below reasons.
Irregularities in camera - due to excessive shaking or lose connections. 
	We realized that shape detection is more sensitive than color detection, so holding the camera in the hand causued  a lot of variation due to small disturbances.This made us realize that we need to do all experimentation on a stable platform, so we fixed a position for the camera.  
Irregularity in treasure - due to variations in position, color, and folds on paper
 All teams used different cut pieces of paper for shape detection. We faced a lot of issues due to these irregular pieces of paper because of which the relative positions used to vary. Based on the algorithm we used, we need the shapes to be relatively almost in the same position. So we used thicker and standardized shapes as below. 

The above standardizations helped a lot in obtaining the output. 

To identify the treasure we hooked up 5 LEDs 
2 LEDs - Color of Treasure 
3 LEDs - Shape of the Treasure

We maintained the same color detection scheme. 
For shape detection, on observation of the treasures, we saw that the treasures had differential lengths on a fixed line in the frame. We used this property and sampled 3 fixed lines on the frame and compared the lengths of this points. 


As shown in the figure above, we compared lengths of La, Lb and Lc. 
Based on the below relations we come to a conclusion of the shape. 
***pics ***

A huge amount of time is required to calibrate to choose exact La, Lb, and Lc. 
We have a frame which is 147px in height , so we initially started with positions with La = 36, Lb = 72 and Lc = 108, but we realised this is suitable for an ideal situation, but for a more practical case( both shown below ), after a lot of iterations of fine tuning, we obtained the,
 actual La =   actual Lb = actual Lc = 




**pics **

After this, we pass 3 outputs to arduino and sample it for 3.5s. Based on the average collected Arduino, we decide the shape and the color which are output from the arduino using 5 LEDs.
For stability we hold this output till the next output, this gives stable signals too. 

Below is the video of shape and color detection.

<iframe width="560" height="315" src="https://youtu.be/HeEwGeix-AQ" frameborder="0" allow="accelerometer; autoplay; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>

Left LEDs - Red LED - Red treasure 
	        Green LED - Blue treasure 

Right LEDs - Green - Triangle 
	           Yellow - Diamond 
		Red - Square
So below combinations give the output. 



Conclusion
To maintain the same static scanning, we need to be at an exact fixed distance from the treasure. So for the final competition, we have decided to scan it at the intersection where the treasures are almost at the same distance, since walls are almost same distance to intersection. 
Further tuning needs to be done to find the new parameters La, Lb and Lc for the competition. 




