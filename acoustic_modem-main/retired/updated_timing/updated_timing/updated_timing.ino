/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include "Goertzel.h"
#include "ACOMM4.h"
#include <SPI.h>

#define MESSAGE_MAXIMUM_SYMBOL_SIZE 16384 * 4
#define RELAY_SWITCH_PIN 9
#define demod_pin 14
#define SS_PIN 10 // system select pin for SPI communication utilizing MCP4901 DAC

////////////////////////////////////////////////////////////////////
// Demodulation Variables
////////////////////////////////////////////////////////////////////

// acomm(baud rate, update rate)
// update rate/baud rate must be an integer.
ACOMM4 acomm(250);

// look into:
// pool test, confirm transducers are proper for our purposes before ordering more
// flag for switching back to listening channel
// send on one channel
// receive on another
// add saving data and waveform option

#define SINE_DATA_LENGTH 12                                                                                         // number of steps to take per sine wave
uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 180, 217, 231, 217, 180, 128, 76, 39, 25, 39, 76}; // pre-computed sine wave
int16_t sine_data[SINE_DATA_LENGTH];
int sine_data_index = 0;

IntervalTimer demodTimer;
uint32_t previousCPURead;
uint32_t currentCPURead;
uint32_t counter = 0;
float frequencyOut = 300000;
int numberOfSteps = 16;
float freqMicrosecondsDAC = 1/frequencyOut*1000000.0/numberOfSteps;

/////////////////////////Shared Functions/////////////////////////
elapsedMicros checkTimer;
void setup()
{
    pinMode(RELAY_SWITCH_PIN, OUTPUT);
    pinMode(demod_pin, INPUT);

    // set analog read resolution and average so that Goertzel can be calculated in real time

    // currently analog read takes...
    // 8 bit - 3us
    // 10 bit - 5us
    analogReadResolution(8);
    analogReadAveraging(1);

    // begin the SPI to communicate with MCP4901 DAC
    SPI.begin();

    // turn of relay (on == transmit)
    updateRelay(false);

    Serial.begin(115200); // begin Serial communication
//    while (!Serial)
//    {
//        ;
//    }
    delay(2000);
    Serial.print("DAC update speed: ");
    Serial.println(freqMicrosecondsDAC,4);
    delay(1000);

//    beginDemodTimer();
    checkTimer = 0;
}

float getMicroseconds(){
  previousCPURead = currentCPURead;
  currentCPURead = ARM_DWT_CYCCNT;
  if(previousCPURead > currentCPURead){
    counter++;
  }
  return float(currentCPURead/(float(F_CPU)/1000000)) + float(counter*(UINT32_MAX/(float(F_CPU)/1000000)));
}

void loop()
{
  float startingMicroseconds = getMicroseconds();
  int x = 0;
  int numTests = 100000*12;
  float startTime = getMicroseconds();
  while(x < numTests){
    if(getMicroseconds() >= startingMicroseconds + (x+1)*freqMicrosecondsDAC){
      uint8_t newVal = sine_data_raw[(sine_data_index++)%12] ;
      x++;
    }
  }
  Serial.println((getMicroseconds()-startTime)/numTests,4);
  delay(2000);
//  
//  float temp;
//  
//  Serial.println("TEST:");
//  float startTime = getMicroseconds();
//  while(x < numTests){
//    temp = getMicroseconds();
//    x++;
//  }
//  Serial.println((temp-startTime)/numTests,4);
//  delay(2000);
}

void updateRelay(bool relayOn)
{
    digitalWrite(RELAY_SWITCH_PIN, !relayOn);
}

// /////////////////////////Demodulation Functions/////////////////////////

void beginDemodTimer()
{
//    Serial.println(acomm.getTimerUpdateSpeedMicros());
//    delay(3000);
    while (!demodTimer.begin(demodTimerCallback, acomm.getTimerUpdateSpeedMicros()))
    {
        ;
    }
}

void demodTimerCallback()
{
    byte analogSample = byte(analogRead(demod_pin));
    Serial.print(micros());
    Serial.printf(" ");
    Serial.print(analogSample);
    Serial.printf("$\n?");
    return;
//    acomm.addSample(analogSample);
//    if (acomm.isMessageReady())
//    {
//        demodTimer.end();
//        acomm.printMessages();
//        delay(50);
//        beginDemodTimer();
//    }
    // Serial.println(micros() - microsnow);
}
