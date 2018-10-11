void setup() {
  Serial.begin(9600);
  analogRead(A1);
  analogRead(A2);
}

void loop() {
  Serial.println(analogRead(A1));
  Serial.println(analogRead(A2));
  delay(500);

}

bool check_front() {
  if (analogRead(A1) > 225) { //then there is a wall
    return true;
  } else {
    return false;
  }
}

bool check_right() {
  if (analogRead(A2) > 225) { //then there is a wall
    return true;
  } else {
    return false;
  }
}

