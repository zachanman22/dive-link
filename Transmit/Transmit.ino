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

MD_AD9833	 AD(FSYNC);  // Hardware SPI
// MD_AD9833	AD(DATA, CLK, FSYNC); // Arbitrary SPI pins

int frequency_high = 140000;   //  Frequency when sending a 1
int frequency_low = 60000;    //  Frequency when sending a 0
int begin_transmit = 40000;   //  Frequency to sync the receiver
char message[] = "F";         //  Message to transmit

const int frame_size = 120;   //  Amount of Bytes in the frame
const int ecclen = 8;         //  Amount of redundancy
const int msglen = frame_size - ecclen; //  Amount of message bytes in the frame
char frame[frame_size];       //  Allocating space for the frame
char message_container[msglen]; //  Allocating space for the message

int bit_speed = 500; //  Amount of time to hold a bit frequency in microseconds
int frame_delay = bit_speed * frame_size * 8; // Amount of time to wait for a full frame to send
int guard_time = bit_speed * 8; // Guard time to prevent overlap
int number_of_transmitters = 2; // Number of transmitters in TDMA
int transmit_id = 0;            // ID of this transmitter


char encoded[msglen + ecclen];  // Buffer for the encoded message
char sending_encoded[msglen + ecclen];  //Buffer for the message to send over serial
int send_high;    // Varible used to send bits over the communication

void setup(void)
{

  // Set up serial and function generator
	AD.begin();
  AD.setMode(MD_AD9833::MODE_SINE);
  AD.setActiveFrequency(MD_AD9833::CHAN_0);
  Serial.begin(115200);

  // Initialize the reed solomon encoding
  RS::ReedSolomon<msglen, ecclen> rs;

  // Initialize the button
  pinMode(0, INPUT_PULLUP);

  // Fill the message container with a filler character
  // Then move the message into the container
  // Finally, set the last byte of the message to 0
  for(int i = 0; i < sizeof(message_container); i++){
    message_container[i] = 'F';
  }
  for(int i = 0; i < sizeof(message) - 1; i++){
    message_container[i] = message[i];
  }
  message_container[sizeof(message_container) - 1] = '\0';

  // Encode the message and put it into the encoded container
  rs.Encode(message_container, encoded);

  // Corrupting first 8 bytes of message (any 4 bytes can be repaired, no more)
  for(uint i = 0; i < ecclen / 2; i++) {
      encoded[i + 6] = 'E';
  }

  // Move the encoded info into sending_encoded
  for(int i = 0; i < sizeof(encoded); i++){
    sending_encoded[i] = encoded[i];
  }

  // Move the zero byte between the message and ecc to the end and shift the ecc up
  for(int i = msglen; i < sizeof(sending_encoded); i++){
    sending_encoded[i-1] = sending_encoded[i];
  }

  // Wait for button push
  sending_encoded[sizeof(sending_encoded)-1] = '\0';
  Serial.println("Waiting for button press...");
  while(digitalRead(0));
  Serial.println("Transmitting");
}

// The main loop to do TDMA
void loop(void)
{
  // For all transmitters
  for(int transmit_time = 0; transmit_time < number_of_transmitters; transmit_time++){

    //  Delay for the guard time
    delayMicroseconds(guard_time);

    // If it is this sender's turn in the TDMA
    if(transmit_time == transmit_id){

      // Send each byte of the sending_encoded
      for(int i = 0; i < sizeof(sending_encoded); i++){
        send_integer(sending_encoded[i], 7);
      }

      //Stop transmitting
      AD.setFrequency(MD_AD9833::CHAN_0,0);

    // If it is not this sender's turn
    } else {
      // Wait for this frame to send
      AD.setFrequency(MD_AD9833::CHAN_0,0);
      delayMicroseconds(frame_delay);
    }
  }
}

// Function to send an integer/char via changing frequencies
void send_integer(int character, int num_bits){
  AD.setFrequency(MD_AD9833::CHAN_0,begin_transmit);
  delayMicroseconds(bit_speed);
  //Serial.print("S");
  for(int i = (num_bits - 1); i >= 0; i--){
    send_high = (character >> i) & 0X01;
    if(send_high){
      AD.setFrequency(MD_AD9833::CHAN_0,frequency_high);
      //Serial.print("1");
    } else {
      AD.setFrequency(MD_AD9833::CHAN_0,frequency_low);
      //Serial.print("0");
    }
    delayMicroseconds(bit_speed);
  }
  //Serial.print("\n");
}
