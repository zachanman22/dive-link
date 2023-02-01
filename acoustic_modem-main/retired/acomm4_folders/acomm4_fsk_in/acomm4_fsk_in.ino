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

IntervalTimer demodTimer;

/////////////////////////Shared Functions/////////////////////////
elapsedMicros checkTimer;
unsigned long counter = 1;
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

    // set priority of timers to prevent interruption
//    demodTimer.priority(128);

    Serial.begin(115200); // begin Serial communication
//    while (!Serial)
//    {
//        ;
//    }

    delay(2000);

//    beginDemodTimer();
    checkTimer = 0;
}

void loop()
{
//    Serial.println(checkTimer);
    if(checkTimer/8 >= counter){
      counter++;
      demodTimerCallback();
      if(checkTimer > 10000000){
        checkTimer = 0;
        counter = 1;
      }
    }
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
