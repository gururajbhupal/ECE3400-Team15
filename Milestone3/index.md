#Milestone 3 Report

## Introduction
For this milestone we modified our robots traversal algorithm. Now instead of exploring a maze via right hand wall following we explore it via a modified depth first search while updating the GUI.
Our algorithm is very similar to a depth first search, but instead of directly backtracking once we have no more unexplored nodes around us we plan a path to the nearest unexplored node (via nodes we have already visited) and then traverse that path. Often times this leads to directly backtracking, but there are a lot of scenarios where it actually makes maze exploration a lot faster!
Implementing Our Modified DFS

## Implementing Our Modified DFS
To implement DFS and path planning we realized that we needed a stack and a queue for each respectively. So we downloaded the StackArray and QueueList libraries from the Arduino playground. 
We debated for a while on what we were gonna push to the stack and queue, and decided on pushing  coordinates. 
Then we created the 2d array maze [9][9]. Initially the type of maze was boolean, but we realized that we wanted more information then just whether or not a coordinate was explored. At each coordinate we update the wall information and send it to the GUI, so we wanted a way to store that information at each {x,y} coordinate.
With the ideas above, we created two structs - coordinate and info- and type defined them as Coordinate and Info respectively. 
```
/*A coordinate contains an x,y location*/
struct coordinate {
  int x;
  int y;
};
/*Declare a type coordinate as Coordinate*/
typedef struct coordinate Coordinate;

/*Info contains information at an x,y coordinate. Information includes
  whether the coordinate has been explored as well as wall information*/
struct info {
  bool explored;
  /*all walls of each coordinate are set to high initially so that when path-planning back
     we don’t go through a node we haven’t visited!*/
  bool n_wall = 1;
  bool e_wall = 1;
  bool s_wall = 1;
  bool w_wall = 1;
};
/*Declare a type info as Info*/
typedef struct info Info;
```
With the above structs we now made our maze array of type Info. So each maze[x][y] has wall 
information as well as if that location has been explored!

Then we defined 3 Coordinates; left, front, and right. 
Left is initialized as the immediate left coordinate of the robot - {0,1}
Front is initialized as the immediate front coordinate of the robot - {1,0}
Right is not initialized to anything as we start the robot at the northwestern most spot of the maze facing south so there is always a wall to the right initially.

We then had to update our function update_position() so that at each {x,y} location we update the coordinate to the left, front, and right of us based on our current heading.

Likewise we also updated our scan_walls() function so that at each {x,y} location the maze[x][y] wall information was set.

###DFS Algorithm
With everything in place algorithm for our modified DFS was now pretty simple.

Some Notes:
Since we started at {0,0} which is at an intersection we have a global variable 
boolean first_run initialized to true which prevents the position from updating until we leave {0,0}
The DFS algorithm is in loop() so it is always rerunning itself
Every time we reach an intersection we update wall information and send it to the GUI via RF communication



MODIFIED DFS ALGORITHM

If at an intersection
	If this is the first time running the algorithm
		First_run = false
		Set {0,0} to explored
	Else
		Update position
	Push the surrounding unvisited valid coordinates to the stack
	If the stack is not empty
		V = popping the first element of the stack 
		If V has NOT been explored
			If V is the front coordinate
				Go forward
			Else if V is the left coordinate
				Turn left
			Else if V is the right coordinate
				Turn right
			Else V is a coordinate not immediately surrounding us
				Plan a path to V
				Traverse that path
			Set V to explored
Else if not at an intersection
	If every node has not been explored
		Line follow
	Else If every node has been explored
		Celebrate 

Most of the algorithm is pretty self explanatory except for planning our path to V. Essentially since we have all the information of coordinates already explored we traverse to V via the shortest route. So from where we are we push to the queue in the order we traverse back. Then we have another function which traverses a given queue.

We highly recommend you check out our code - Milestone_3_final. It is well commented and easy to follow!
Three videos of random maze traversals have been added below for TA enjoyment.
[video 1]
[video 2]
[video 3]

Conclusion
Everything seems to be wrapping up so quickly! Now we just need to clean up our robot, add the camera, and win the final competition. 




