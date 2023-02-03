// Basic MD_AD9833 test file
//
// Initialises the device to default conditions
// 
// Connect a pot to A0 to change the frequency by turning the pot
//
#include <MD_AD9833.h>
#include <SPI.h>

// Pins for SPI comm with the AD9833 IC
#define DATA  11	///< SPI Data pin number
#define CLK   13	///< SPI Clock pin number
#define FSYNC 10	///< SPI Load pin number (FSYNC in AD9833 usage)

MD_AD9833	AD(FSYNC);  // Hardware SPI
// MD_AD9833	AD(DATA, CLK, FSYNC); // Arbitrary SPI pins

void setup(void)
{
	AD.begin();
  pinMode(3,OUTPUT);
  Serial1.begin(9600);
}
int out=100000;
void loop(void)
{
	AD.setFrequency(MD_AD9833::CHAN_0, out);
//  out+=100;
//  if(out>=1000000)out=10000;
  digitalWrite(3,!digitalRead(3));
//  Serial1.println("hello");
  delay(10);
}
