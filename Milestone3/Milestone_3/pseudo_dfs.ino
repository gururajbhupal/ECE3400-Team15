0 = north
1 = east
2 = south
3 = west
x_north = x-1;
y_west = y-1;
x_south = x+1;
y_east = y+1;
//dfs_check {
//    switch (heading) {
//        case 0:
//            if (!check_left && !grid[x][y-1]) {
//                stack.push([x,y-1]);
//            } else if (!check_front && !grid[x-1][y]) {
//                stack.push([x-1,y]);
//            } else if (!check_right && !grid[x][y+1]) {         stack.push([x,y+1]);
//            }
//            break;
//        case 1:
//            if (!check_left && !grid[x-1][y]) {
//                stack.push([x-1,y]);
//            } else if (!check_front && !grid[x][y+1]) {
//                stack.push([x][y+1]);
//            } else if (!check_right && !grid[x+1][y]) {
//                stack.push([x+1][y]);
//            }
//            break;
//        case 2:
//            if (!check_left && !grid[x][y+1]) {
//                stack.push([x,y+1]);
//            } else if (!check_front && !grid[x+1][y] {
//                stack.push([x+1,y]);
//            } else if (!check_right && !grid[x][y-1]) {
//                stack.push([x,y-1]);
//            }
//            break;
//        case 3:
//            if (!check_left && !grid[x+1,y]) {
//                stack.push([x+1,y]);
//            } else if (!check_front && !grid[x][y-1]) {
//                stack.push([x,y-1]);
//            } else if (!check_right && !grid[x-1][y]) {
//                stack.push([x-1,y]);
//            }
//            break;
//    }
//}


int left[];
int front[];
int right[];
void update_position() {
  switch (heading) {
    case 0:
      x--;
      left = [x, y-1];
      front = [x-1, y];
      right = [x, y+1];
      break;
    case 1:
      y--;
      left = [x-1, y];
      front = [x, y+1];
      right = [x+1, y];
      break;
    case 2:
      x++;
      left = [x, y+1];
      front = [x+1, y];
      right = [x, y-1];
      break;
    case 3:
      y++;
      left = [x+1, y];
      front = [x, y-1];
      right = [x-1, y];
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
        if (!maze[v]) {
            maze[x, y] = 1;
            push_unvisited();
        }
    }
}











