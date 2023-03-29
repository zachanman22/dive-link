// Basic MD_AD9833 test file
//
// Initialises the device to default conditions
// 
// Connect a pot to A0 to change the frequency by turning the pot
//
#include <MD_AD9833.h>
#include <SPI.h>

// Pins for SPI comm with the AD9833 IC
#define DATA  11  ///< SPI Data pin number
#define CLK   13  ///< SPI Clock pin number
#define FSYNC 10  ///< SPI Load pin number (FSYNC in AD9833 usage)

MD_AD9833  AD(FSYNC);  // Hardware SPI
// MD_AD9833  AD(DATA, CLK, FSYNC); // Arbitrary SPI pins

int frequency_high = 120000;
int frequency_low = 70000;

int num_transmit = 2;
int id_transmit = 1;

int bit_speed = 1000;
int guard_band = bit_speed * 4;

char character = 'G';
int send_high;
int message_index;

void setup(void)
{
  AD.begin();
  Serial.begin(115200);
  AD.setMode(MD_AD9833::MODE_SINE);
  AD.setActiveFrequency(MD_AD9833::CHAN_0);

  pinMode(0, INPUT);
  Serial.println("Waiting for button press...");
  while(!digitalRead(0));
  Serial.println("Transmitting");
}

void loop(void)
{
  for(int i = 0; i < num_transmit; i++){
    if(i == id_transmit){
      send_integer(character, 8);
      AD.setFrequency(MD_AD9833::CHAN_0,1000);
    } else {
      delayMicroseconds(bit_speed * 8);
    }
    delayMicroseconds(guard_band);
  }
}

void send_integer(int character, int num_bits){
  for(int i = (num_bits - 1); i >= 0; i--){
    send_high = (character >> i) & 0X01;
    if(send_high){
      AD.setFrequency(MD_AD9833::CHAN_0,frequency_high);
      //Serial.print("1");
      //AD.setFrequency(MD_AD9833::CHAN_0,frequency_low);
    } else {
      AD.setFrequency(MD_AD9833::CHAN_0,frequency_low);
      //Serial.print("0");
      //AD.setFrequency(MD_AD9833::CHAN_0,frequency_high);
    }
    delayMicroseconds(bit_speed);
  }
  //Serial.println(" ");
}
