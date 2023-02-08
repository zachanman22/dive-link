#define HWSERIAL Serial2
int incomingByte;
const int MAX_MESSAGE_LENGTH = 100;
int count;
static char message[MAX_MESSAGE_LENGTH];
void setup() {
  HWSERIAL.begin(9600);
  Serial.begin(9600);
}

void loop() {
  
  if (HWSERIAL.available() > 0) {
    incomingByte = HWSERIAL.read();
    //Serial.print("UART received: ");
    //Serial.println(int(incomingByte));
    //HWSERIAL.print("UART received:");
    //HWSERIAL.println(incomingByte);
    //Message coming in (check not terminating character) and guard for over message size
     if ( incomingByte != '\n' && (count < MAX_MESSAGE_LENGTH - 1) )
     {
       //Add the incoming byte to our message
       message[count] = incomingByte;
       count++;
     }
   //Full message received...
   else
   {
     //Print the message (or do other things)
     message[count] = '\0';
     Serial.println(message);

     //Reset for the next message
     count = 0;
   }
  }
}
