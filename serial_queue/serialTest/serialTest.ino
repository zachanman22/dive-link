
void setup() {
  Serial.begin(9600);
}
void loop() {
  Serial.println("hello world");
  delay(1000);
//  int incomingByte;
//  if (Serial.available() > 0) {
//    incomingByte = Serial.read();
//    Serial.print("USB received: ");
//    Serial.println(char(incomingByte));
//  }
}
