/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include <SPI.h>

#define RELAY_SWITCH_PIN 9
#define demod_pin 14
/////////////////////////Shared Functions/////////////////////////
elapsedMicros checkTimer;
uint64_t counter;
const int sample_freq_period_us = 8;
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
    Serial.printf("?");
    counter = 1;
    checkTimer = 0;
}

void loop()
{
    //    Serial.println(checkTimer);
    if (checkTimer / sample_freq_period_us >= counter)
    {
        if (checkTimer > 10000000)
        {
            checkTimer = 0;
            counter = 0;
        }
        counter++;
        demodTimerCallback();
    }
}

void updateRelay(bool relayOn)
{
    digitalWrite(RELAY_SWITCH_PIN, !relayOn);
}

// /////////////////////////Demodulation Functions/////////////////////////
void demodTimerCallback()
{
    byte analogSample = byte(analogRead(demod_pin));
    Serial.print(micros());
    Serial.printf(" ");
    Serial.print(analogSample);
    Serial.printf("$\n?");
}
