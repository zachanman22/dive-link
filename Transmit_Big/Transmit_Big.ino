// Basic MD_AD9833 test file
//
// Initialises the device to default conditions
// 
// Connect a pot to A0 to change the frequency by turning the pot
//
#include <MD_AD9833.h>
#include <SPI.h>
#include "rs.hpp"

// Pins for SPI comm with the AD9833 IC
#define DATA  11	///< SPI Data pin number
#define CLK   13	///< SPI Clock pin number
#define FSYNC 10	///< SPI Load pin number (FSYNC in AD9833 usage)
// MD_AD9833	AD(DATA, CLK, FSYNC); // Arbitrary SPI pin
MD_AD9833   AD(FSYNC);  // Hardware SPI

int frequency_high = 80000;   //  Frequency when sending a 1
int frequency_low = 60000;    //  Frequency when sending a 0
char message[] = "9";         //  Message to transmit
char filler = 65;

const int frame_size = 8;   //  Amount of Bytes in the frame
const int ecclen = 0;         //  Amount of redundancy
const int msglen = frame_size - ecclen; //  Amount of message bytes in the frame
char frame[frame_size];       //  Allocating space for the frame
char message_container[msglen]; //  Allocating space for the message

int baud_rate = 800;
int bit_period = int(1000000/float(baud_rate)); //  Amount of time to hold a bit frequency in microseconds
int frame_delay = bit_period * frame_size * 8; // Amount of time to wait for a full frame to send
int guard_time = frame_delay
; // Guard time to prevent overlap
int number_of_transmitters = 1; // Number of transmitters in TDMA
int transmit_id = 0;            // ID of this transmitter
int transmit_time = 0;


char encoded[msglen + ecclen];  // Buffer for the encoded message
int send_high;    // Varible used to send bits over the communication
int send_high_prev;

elapsedMicros sendBit;  // Timer used for sending one bit
elapsedMicros guardBand;  // Timer used for waiting for guard band
elapsedMicros sendBlankFrame; // Timer used for sending a blank frame

RS::ReedSolomon<msglen, ecclen> rs;

void setup(void)
{

  // Initialize the button
  pinMode(0, INPUT);

  pinMode(8, OUTPUT);
  
  // Set up serial and function generator
	AD.begin();
  AD.setMode(MD_AD9833::MODE_SINE);
  AD.setActiveFrequency(MD_AD9833::CHAN_0);
  //AD.setFrequency(MD_AD9833::CHAN_0,120000);
  Serial.begin(115200);

  // Initialize the reed solomon encoding

  // Fill the message container with a filler character
  // Set the first character to the transmit_id
  // Then move the message into the container
  // Finally, set the last byte of the message to 0
//  for(int i = 0; i < sizeof(message_container); i++){
//    message_container[i] = filler;
//  }
////  message_container[0] = transmit_id + 1;
////  message_container[1] = transmit_time + 1;
////  for(int i = 2; i < sizeof(message) - 1; i++){
////    message_container[i] = message[i];
////  }
//  message_container[sizeof(message_container) - 1] = '\0';
////
//  // Encode the message and put it into the encoded container
//  rs.Encode(message_container, encoded);
////
////  // Move the zero byte between the message and ecc to the end and shift the ecc up
//  for(int i = msglen; i < sizeof(encoded); i++){
//    encoded[i-1] = encoded[i];
//  }
//  encoded[sizeof(encoded)-1] = '\0';
//  encoded[0] = encoded[0]+128;

  for(int i = 0; i < sizeof(encoded); i++){
    encoded[i] = filler; //<
  }
//  for(int i = 0; i < 8; i++){
//    encoded[i] = 0;
//  }
//  for(int i = 17; i < 25; i++){
//    encoded[i] = 0;
//  }
  send_high_prev = 2;


  // Wait for button push
  Serial.println("Waiting for button press...");
  digitalWrite(8, LOW);
  while(!digitalRead(0));
  Serial.println("Transmitting");
}

// The main loop to do TDMA
void loop(void)
{

  //  Delay for the guard time
  guardBand = 0;
  for(int i = 0; i < sizeof(encoded); i++){
    encoded[i] = filler+(transmit_time%26);
  }
  while(guardBand < guard_time);

  // If it is this transmitters turn in the TDMA
  if(transmit_time % number_of_transmitters == transmit_id){

    // Send each byte of the sending_encoded
    // Serial.println("Sending");
    digitalWrite(8, HIGH);
    //delay(1);
    sendBit = 0;
    for(int i = 0; i < sizeof(encoded); i++){
      send_integer(encoded[i], 8);
    }

      //Stop transmitting
      //AD.setFrequency(MD_AD9833::CHAN_0,1000);
      digitalWrite(8, LOW);
      //Serial.print("\n");
      //Serial.println("Stopping\n");
      send_high_prev = 2;

    // If it is not this sender's turn
    } else {
      //Set timer for waiting for the frame
      sendBlankFrame = 0;

      // This block of code is identical to the block of code that generated the first message
      // The purpose of this block is to generate a message for the sender to send next turn
      // in the TDMA if it is available. The message now has a byte for sender, a byte for time
      // and eight bytes for error correction. This added byte for time changes every send, thus
      // the error correction must run again in order for it to work properly again.

      //Fills the message container with filler character
//      for(int i = 0; i < sizeof(message_container); i++){
//        message_container[i] = filler;
//      }
//      // Fills the ID and Time fields
//      message_container[0] = transmit_id + 1;
//      message_container[1] = (transmit_time % 255) + 1;
//
//      // Fills in the message
//      for(int i = 2; i < sizeof(message) - 1; i++){
//        message_container[i] = message[i];
//      }
//      message_container[15] = 255;
//      message_container[16] = 255;
//      message_container[sizeof(message_container) - 1] = '\0';
//
//      // Encode the message and put it into the encoded container
//      rs.Encode(message_container, encoded);
//
//      // Move the zero byte between the message and ecc to the end and shift the ecc up
//      for(int i = msglen; i < sizeof(encoded); i++){
//        encoded[i-1] = encoded[i];
//      }
//      encoded[sizeof(encoded)-1] = '\0';
//      encoded[0] = encoded[0]+128;

      for(int i = 0; i < sizeof(encoded); i++){
        encoded[i] = filler+(transmit_time%26);
      }
//      encoded[0] = 128;
//      encoded[15] = 255;
//      encoded[16] = 255;
      
//      for(int i = 0; i < sizeof(encoded); i++){
//        print_message(encoded[i]);
//      }
//      Serial.println(" ");

      while(sendBlankFrame < frame_delay);
    }

    // Increment the time counter
    transmit_time++;
}

// Function to send an integer/char via changing frequencies
void send_integer(int character, int num_bits){
  for(int i = (num_bits - 1); i >= 0; i--){
    send_high = (character >> i) & 0X01;
    //if(send_high != send_high_prev){
      if(send_high){
        AD.setFrequency(MD_AD9833::CHAN_0,frequency_high);
        //Serial.print("1");
        //digitalWrite(8, HIGH);
      } else {
        AD.setFrequency(MD_AD9833::CHAN_0,frequency_low);
        //Serial.print("0");
        //digitalWrite(8, LOW);
      }
    //}
    send_high_prev = send_high;
    while(sendBit < bit_period);
    sendBit = sendBit - bit_period;
  }
  //Serial.print("\n");
}

void print_message(int in){
  for(int i = 7; i >= 0; i--){
    send_high = (in >> i) & 0X01;
    if(send_high && send_high_prev != send_high){
      Serial.print('1');
    } else {
      Serial.print('0');
    }
  }
}
