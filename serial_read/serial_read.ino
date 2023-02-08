#define HWSERIAL Serial2
int incomingByte;
void setup() {
  HWSERIAL.begin(9600);
  Serial.begin(9600);
}

void loop() {
  
  if (HWSERIAL.available() > 0) {
    incomingByte = HWSERIAL.read();
    Serial.print("UART received: ");
    Serial.println(char(incomingByte));
    //HWSERIAL.print("UART received:");
    //HWSERIAL.println(incomingByte);
  }
}
