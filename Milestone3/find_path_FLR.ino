StackArray <Coordinate> find_path(Coordinate v) {
  StackArray <Coordinate> path;
  Coordinate next = current;
  Coordinate prev;
  int h = heading;
  while (next != v) {
    /*west of means that the robot needs heading = 3 to get there*/
    bool westof = (v.y < next.y);
    /*east of means that the robot needs heading = 1 to get there*/
    bool eastof = (v.y > next.y);
    /*south of means that the robot needs heading = 2 to get there*/
    bool southof = (v.x > next.x);
    /*north of means that the robot needs heading = 0 to get there*/
    bool northof = (v.x < next.x);
    bool n = maze[next.x][next.y].n_wall;
    bool e = maze[next.x][next.y].e_wall;
    bool s = maze[next.x][next.y].s_wall;
    bool w = maze[next.x][next.y].w_wall;


    // choose next in order of FLR
    switch (h) {
      case 0: // reject south
        if (!n && northof) {
          next = {next.x - 1, next.y};
        } else if (!w && westof) {
          next = {next.x, next.y - 1};
        } else if (!e && eastof) {
          next = {next.x, next.y + 1};
        } else if (!n) {
          next = {next.x - 1, next.y};
        } else if (!w) {
          next = {next.x, next.y - 1};
        } else if (!e) {
          next = {next.x, next.y + 1};
        } else {
          next = {next.x + 1, next.y};
        }
        break;
      case 1: // reject west
        if (!e && eastof) {
          next = {next.x, next.y + 1};
        } else if (!n && northof) {
          next = {next.x - 1, next.y};
        } else if (!s && southof) {
          next = {next.x + 1, next.y};
        } else if (!e) {
          next = {next.x, next.y + 1};
        } else if (!n) {
          next = {next.x - 1, next.y};
        } else if (!s) {
          next = {next.x + 1, next.y};
        } else {
          next = {next.x, next.y - 1};
        }
        break;
      case 2: // reject north
        if (!s && southof) {
          next = {next.x + 1, next.y};
        } else if (!e && eastof) {
          next = {next.x, next.y + 1};
        } else if (!w && westof) {
          next = {next.x, next.y - 1};
        } else if (!s) {
          next = {next.x + 1, next.y};
        } else if (!e) {
          next = {next.x, next.y + 1};
        } else if (!w) {
          next = {next.x, next.y - 1};
        } else {
          next = {next.x - 1, next.y};
        }
        break;
      case 3: // reject east
        if (!w && westof) {
          next = {next.x, next.y - 1};
        } else if (!s && southof) {
          next = {next.x + 1, next.y};
        } else if (!n && northof) {
          next = {next.x - 1, next.y};
        } else if (!w) {
          next = {next.x, next.y - 1};
        } else if (!s) {
          next = {next.x + 1, next.y};
        } else if (!n) {
          next = {next.x - 1, next.y};
        } else {
          next = {next.x, next.y + 1};
        }
        break;
    }
    if (prev.x < next.x) {
      h = 2;
    } else if (prev.x > next.x) {
      h = 0;
    } else if (prev.y < next.y) {
      h = 1;
    } else if (prev.y > next.y) {
      h = 3;
    } else {
      digitalWrite(7, HIGH);
    }
    path.push(next);
  }
  return path;
}



// Maybe push_unvisited as you go?
void traverse_path(StackArray <Coordinate> path) {
  while (!path.isEmpty()) {
    if (atIntersection()) {
      update_position();
      p = path.pop();
      if (p.x == front.x && p.y == front.y) {
        /*send relevant information to GUI, go straight*/
        scan_walls();
        rf();
        adjust();
      }
      /*else if v is the left coordinate*/
      else if (p.x == left.x && p.y == left.y) {
        /*send relevant information to GUI, turn left*/
        scan_walls();
        rf();
        adjust();
        turn_left_linetracker();
      }
      /*else if v is the right coordinate*/
      else if (p.x == right.x && p.y == right.y) {
        /*send relevant information to GUI, turn right*/
        scan_walls();
        rf();
        adjust();
        turn_right_linetracker();
      }
      else {
        // turn around
        adjust();
        turn_right_linetracker();
        turn_right_linetracker();
      }
    }
    linefollow();
  }
}

