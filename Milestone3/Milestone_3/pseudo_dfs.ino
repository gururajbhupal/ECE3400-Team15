//0 = north
// 1 = east
//        2 = south
//            3 = west
//                x_north = x - 1;
y_west = y - 1;
x_south = x + 1;
y_east = y + 1;


struct surrounding_coordinate {
  int x;
  int y;
}

surrounding_coordinate left;
surrounding_coordinate front;
surrounding_coordinate right;

void update_position() {
  switch (heading) {
    case 0:
      x--;
      if (y != 0) {
        left.x = x;
        left.y = y - 1;
      }
      if (x != 0) {
        front.x = x - 1;
        front.y = y;
      }
      if (y != m) {
        right.x = x
        right.y = y + 1;
      }
      break;
    case 1:
      y--;
      if (x != 0) {
        left.x = x - 1;
        left.y = y;
      }
      if (y != m) {
        front.x  = x;
        front. y = y + 1;
      }
      if (x != m) {
        right.x = x + 1;
        right.y = y;
      }

      break;
    case 2:
      x++;
      left = [x, y + 1];
      front = [x + 1, y];
      right = [x, y - 1];
      break;
    case 3:
      y++;
      left = [x + 1, y];
      front = [x, y - 1];
      right = [x - 1, y];
      break;
  }
}


push_unvisited {
  if (!check_right && !maze[right]) stack.push(right);
  if (!check_front && !maze[front]) stack.push(front);
  if (!check_left && !maze[left]) stack.push(left);
}



dfs {
  push_unvisited();
  while (stack is not empty) {
    v = stack.pop;
    if (!maze[v][]) {
      maze[x, y] = 1;
      push_unvisited();
    }
  }
}











