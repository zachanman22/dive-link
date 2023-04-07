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
// MD_AD9833  AD(DATA, CLK, FSYNC); // Arbitrary SPI pin
MD_AD9833   AD(FSYNC);  // Hardware SPI

int frequency_high = 104000;   //  Frequency when sending a 1
int frequency_low = 62000;    //  Frequency when sending a 0

void setup(void)
{

  // Initialize the button
  pinMode(0, INPUT);

  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);
  
  // Set up serial and function generator
  AD.begin();
  AD.setMode(MD_AD9833::MODE_SINE);
  AD.setActiveFrequency(MD_AD9833::CHAN_0);
  //AD.setFrequency(MD_AD9833::CHAN_0,120000);
  Serial.begin(115200);
  
  AD.setFrequency(MD_AD9833::CHAN_0,frequency_low);
  digitalWrite(8, HIGH);
  
}
// The main loop to do TDMA
void loop(void)
{
  
}
