int wall_front = A1;
int wall_right = A2;

void setup() {
  Serial.begin(115200);
  analogRead(wall_front);
  analogRead(wall_right);
}

void loop() {
  Serial.println("front");
  Serial.println(analogRead(wall_front));
  Serial.println("right");
  Serial.println(analogRead(wall_right));
  delay(500);

}

bool check_front() {
  if (analogRead(wall_front) > 225) { //then there is a wall
    return true;
  } else {
    return false;
  }
}

bool check_right() {
  if (analogRead(wall_right) > 225) { //then there is a wall
    return true;
  } else {
    return false;
  }
}

