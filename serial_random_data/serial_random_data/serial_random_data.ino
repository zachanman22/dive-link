int data;
void setup() {
  Serial.begin(115200);

}

void loop() {
  data = random(1000,4000);
  Serial.println(data);
  delayMicroseconds(2);
}
