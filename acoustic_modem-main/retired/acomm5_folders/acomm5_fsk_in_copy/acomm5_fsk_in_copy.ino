/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include <SPI.h>

#define RELAY_SWITCH_PIN 9
#define demod_pin 14

uint64_t previousCPURead;
uint64_t currentCPURead;
uint32_t freq_sample_counter = 0;
uint64_t offset_amount = 0;
const float sample_freq = 100000;
const float sample_freq_period_us = 1 / sample_freq * 1000000;
const float micro_freq_period_us = float(F_CPU) / 1000000;

void setup()
{
    pinMode(RELAY_SWITCH_PIN, OUTPUT);
    pinMode(demod_pin, INPUT);

    analogReadResolution(8);
    analogReadAveraging(1);

    // begin the SPI to communicate with MCP4901 DAC
    SPI.begin();

    // turn of relay (on == transmit)
    updateRelay(false);

    // set priority of timers to prevent interruption
    //    demodTimer.priority(128);

    Serial.begin(115200); // begin Serial communication
                          //    delay(1000);
    delay(100);
    freq_sample_counter = (getMicroseconds() + 1000000) / sample_freq_period_us;
}

float getMicroseconds()
{
    previousCPURead = currentCPURead;
    currentCPURead = ARM_DWT_CYCCNT;
    if (previousCPURead >= currentCPURead)
    {
        offset_amount = uint32_t((UINT32_MAX - previousCPURead) + offset_amount + 1);
        freq_sample_counter = 1; // int((currentCPURead + offset_amount) / sample_freq_period_us) +
    }
    return float((currentCPURead + offset_amount) / micro_freq_period_us);
}
float currentMicroseconds;
void loop()
{
    currentMicroseconds = getMicroseconds();
    if (currentMicroseconds >= freq_sample_counter * sample_freq_period_us)
    {
        freq_sample_counter += 1;
        //        byte analogSample = byte(analogRead(demod_pin));
        //        if(freq_sample_counter > 4738330)
        Serial.println(micros());
        //        Serial.printf(" ");
        //        Serial.println(freq_sample_counter);
        //        Serial.printf("$\n?");
    }
}

void updateRelay(bool relayOn)
{
    digitalWrite(RELAY_SWITCH_PIN, !relayOn);
}
