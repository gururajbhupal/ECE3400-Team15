void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
}

void loop() {
  // Run through a 3x3 maze
  Serial.println("reset");
  delay(1000);
  Serial.println("0,0,north=true,west=true,south=true");
  delay(1000);
  Serial.println("0,1,north=true,south=true");
  delay(1000);
  Serial.println("0,2,north=true,east=true");
  delay(1000);
  Serial.println("1,2,east=true,south=true");
  delay(1000);
  Serial.println("1,1,north=true,south=true");
  delay(1000);
  Serial.println("1,0,west=true,north=true");
  delay(1000);
  Serial.println("2,0,south=true,west=true");
  delay(1000);
  Serial.println("2,1,south=true,north=true");
  delay(1000);
  Serial.println("2,2,south=true,east=true,north=true");
  delay(5000);
}

