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

int frequency_high = 60000;
int frequency_low = 45000;
char message[] = "Hello World, this is a very important message that should not be corrupted.";
const int msglen = sizeof(message);
const int ecclen = 8;
char repaired[msglen];
char encoded[msglen + ecclen];
char sending_encoded[msglen + ecclen];
int character;
int send_high;

void setup(void)
{
	AD.begin();
  Serial.begin(115200);
  Serial2.begin(9600);
  AD.setMode(MD_AD9833::MODE_SINE);
  AD.setActiveFrequency(MD_AD9833::CHAN_0);
  character = 0;
  RS::ReedSolomon<msglen, ecclen> rs;

  rs.Encode(message, encoded);

  // Corrupting first 8 bytes of message (any 4 bytes can be repaired, no more)
  for(uint i = 0; i < ecclen / 2; i++) {
      encoded[i + 6] = 'E';
  }

  rs.Decode(encoded, repaired);

  Serial.print("Original:  "); Serial.println(message);
  Serial.print("Corrupted: "); Serial.println(encoded);
  Serial.print("Repaired:  "); Serial.println(repaired);
  for(int i = 0; i < sizeof(encoded); i++){
    Serial.println((int)encoded[i]);
    sending_encoded[i] = encoded[i];
  }
  Serial.print("\n\n\n");
  for(int i = 76; i < sizeof(sending_encoded); i++){
    sending_encoded[i-1] = sending_encoded[i];
  }
  sending_encoded[sizeof(sending_encoded)-1] = '\0';
  for(int i = 0; i < sizeof(sending_encoded); i++){
    Serial.println((int)sending_encoded[i]);
  }

  Serial.println((memcmp(message, repaired, msglen) == 0) ? "SUCCESS" : "FAILURE");
}

void loop(void)
{
  Serial2.println(sending_encoded);
  Serial.println(sending_encoded);
  delay(500);
  //AD.setFrequency(MD_AD9833::CHAN_0,frequency_low);
  //character = message[message_index];
  //Serial.println(character);
  //send_integer(character, 8);
  //message_index = (message_index + 1)%sizeof(message);
  //delay(500);
  
}

void send_integer(int character, int num_bits){
  Serial.println(sizeof(character));
  for(int i = (num_bits - 1); i >= 0; i--){
    send_high = (character >> i) & 0X01;
    if(send_high){
      Serial.print("1");
    } else {
      Serial.print("0");
    }
    delay(50);
  }
  Serial.println(" ");
}
