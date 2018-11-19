void maze_traversal_dfs() {
  push_unvisited()
  while (!stack.isEmpty()) {
    if (atIntersection() {
      halt();
      update_position();
      Coordinate v = stack.pop();
      
      /*If the robot has NOT BEEN TO v,*/
      if (!maze[v.x][v.y].explored) {
        /*Go to v*/
        if (v.x == left.x && v.y == left.y) {
          scan_walls();
          rf();
          adjust();
          turn_left_linetracker();
        }
        else if (v.x == front.x && v.y == front.y) {
          scan_walls();
          rf();
          adjust();
        }
        else if (v.x == right.x && v.y == right.y) {
          scan_walls();
          rf();
          adjust();
          turn_right_linetracker();
        }
        /*Mark v as visited*/
        maze[v.x][v.y].explored = 1;
      }
      push_unvisited();
    }
    linefollow();
  }
}
