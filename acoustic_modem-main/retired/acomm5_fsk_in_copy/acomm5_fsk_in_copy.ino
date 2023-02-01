/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include <SPI.h>

#define RELAY_SWITCH_PIN 9
#define demod_pin 14

uint32_t previousCPURead = 0;
uint32_t currentCPURead = 0;
uint32_t CPUoffset = 0;
uint32_t us_counter = 0;
uint64_t freq_sample_counter = 0;
const float sample_freq = 100000;
const float sample_freq_period_us = 1 / sample_freq * 1000000;

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
    delay(2000);
    Serial.println(UINT32_MAX);
    Serial.println(float(F_CPU));
    delay(2000);
    us_counter = 0;
    freq_sample_counter = (getMicroseconds() + 1000000) / sample_freq_period_us;
}

float getMicroseconds()
{
    previousCPURead = currentCPURead;
    currentCPURead = ARM_DWT_CYCCNT;
    if (previousCPURead > currentCPURead)
    {
        CPUoffset = UINT32_max
    }
    return float((currentCPURead) / (float(F_CPU) / 1000000)) + float(CPUoffset / (float(F_CPU) / 1000000));
}
float currentMicroseconds;
void loop()
{
    currentMicroseconds = getMicroseconds();
    if (currentMicroseconds / sample_freq_period_us >= freq_sample_counter)
    {
        freq_sample_counter++;
        byte analogSample = byte(analogRead(demod_pin));
        Serial.print(micros());
        Serial.printf(" ");
        Serial.print(analogSample);
        Serial.printf("$\n?");
    }
}

void updateRelay(bool relayOn)
{
    digitalWrite(RELAY_SWITCH_PIN, !relayOn);
}
