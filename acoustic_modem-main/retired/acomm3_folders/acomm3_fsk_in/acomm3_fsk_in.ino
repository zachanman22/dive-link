/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include "Goertzel.h"
#include "ACOMM3.h"
#include <SPI.h>

#define MESSAGE_MAXIMUM_SYMBOL_SIZE 16384 * 4
#define RELAY_SWITCH_PIN 9
#define demod_pin 14
#define SS_PIN 10 // system select pin for SPI communication utilizing MCP4901 DAC

////////////////////////////////////////////////////////////////////
// Demodulation Variables
////////////////////////////////////////////////////////////////////

char incomingMessageBytes[MESSAGE_MAXIMUM_SYMBOL_SIZE / 8 + 1];

byte incomingSymbols[MESSAGE_MAXIMUM_SYMBOL_SIZE]; // encoded symbols received
int inSymbolIndex;

// acomm(baud rate, update rate)
// update rate/baud rate must be an integer.
ACOMM3 acomm(250);

// look into:
// pool test, confirm transducers are proper for our purposes before ordering more
// flag for switching back to listening channel
// send on one channel
// receive on another
// add saving data and waveform option

IntervalTimer demodTimer;

/////////////////////////Shared Functions/////////////////////////

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
    demodTimer.priority(20);

    Serial.begin(115200); // begin Serial communication
    while (!Serial)
    {
        ;
    }

    delay(100);

    beginDemodTimer();
}

void loop()
{
    ;
}

void updateRelay(bool relayOn)
{
    digitalWrite(RELAY_SWITCH_PIN, !relayOn);
}

// /////////////////////////Demodulation Functions/////////////////////////

void beginDemodTimer()
{
    while (!demodTimer.begin(demodTimerCallback, acomm.getTimerUpdateSpeedMicros()))
    {
        ;
    }
}

void endMessageReceive()
{
    demodTimer.end();
    acomm.setMessageInProgress(false);
    // for (int i = 0; i < inSymbolIndex; i++)
    //     Serial.print(incomingSymbols[i]);
    Serial.println();
    inSymbolIndex = 0;
    delay(50);
    interrupts();
}

void processActiveDemodulation()
{
    if (acomm.symbolReadyToProcess())
    {
        byte newSymbol = acomm.getSymbol();
        if (acomm.isSymbolEndSymbol(newSymbol))
        {
            endMessageReceive();
            beginDemodTimer();
        }
        else
        {
            incomingSymbols[inSymbolIndex++] = newSymbol;
            Serial.print(incomingSymbols[inSymbolIndex - 1]);
        }
    }
}

void detectStartOfDemodulation()
{
    if (acomm.hasMessageStarted())
    {
        Serial.println("Demod started");
        acomm.setMessageInProgress(true);
    }
}

void demodTimerCallback()
{
    byte analogSample = byte(analogRead(demod_pin));
    // Serial.print(micros());
    // Serial.print(" ");
    // Serial.println(analogSample);
    acomm.addSample(analogSample);

    if (acomm.isMessageInProgress())
    {
        processActiveDemodulation();
    }
    else
    {
        detectStartOfDemodulation();
    }
}
