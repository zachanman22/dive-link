/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include <SPI.h>
#include <MCP49xx.h>

#define RELAY_SWITCH_PIN 9
#define SS_PIN 10 // system select pin for SPI communication utilizing MCP4901 DAC

////////////////////////////////////////////////////////////////////
// Modulation Variables
////////////////////////////////////////////////////////////////////
MCP49xx dac(MCP49xx::MCP4901, SS_PIN);

uint16_t sine_data_index;

float volume = 0.4; // volume from 0 to 1

// modulation timers
IntervalTimer outgoingSineWaveDACTimer;
IntervalTimer signalSweep;

// http://www.wolframalpha.com/input/?i=table+round%28100%2B412*%28sin%282*pi*t%2F20%29%2B1%29%29+from+0+to+19

// #define SINE_DATA_LENGTH 24                                                                                                                                // number of steps to take per sine wave
// uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 156, 182, 203, 219, 229, 231, 225, 212, 193, 169, 142, 114, 87, 63, 44, 31, 25, 27, 37, 53, 74, 100, 128}; // pre-computed sine wave
// int16_t sine_data[SINE_DATA_LENGTH];                                                                                                                       // sine-wave including volume

#define SINE_DATA_LENGTH 16                                                                                          // number of steps to take per sine wave
uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 166, 197, 219, 226, 219, 197, 166, 128, 90, 59, 37, 30, 37, 59, 90}; // pre-computed sine wave
int16_t sine_data[SINE_DATA_LENGTH];

float frequency = 25000;
float frequencyMicroSeconds = 1 / frequency * 1000000;

/////////////////////////Shared Functions/////////////////////////

void setup()
{
    pinMode(RELAY_SWITCH_PIN, OUTPUT);

    // begin the SPI to communicate with MCP4901 DAC
    SPI.begin();
    dac.output(0);

    // set priority of timers to prevent interruption
    outgoingSineWaveDACTimer.priority(100);
    signalSweep.priority(99);

    updateVolume();
    Serial.begin(115200); // begin Serial communication
    Serial.printf("Frequency: %f\n", frequency);
    Serial.printf("Microseconds: %f\n", frequencyMicroSeconds);
    // turn of relay (on == transmit)
    updateRelay(true);
    delay(100);
    beginTimersForModulation();
}

void loop()
{
    if (Serial.available())
    {
        Serial.readString();
        updateFrequency();
    }
}

void updateFrequency()
{
    frequency += 100;
    if (frequency >= 33000)
    {
        frequency = 25000;
    }
    frequencyMicroSeconds = 1 / frequency * 1000000;
    Serial.printf("Frequency: %f\n", frequency);
    Serial.printf("Microseconds: %f\n", frequencyMicroSeconds);
    updateSineWaveDACTimer();
}

/////////////////////////Modulation Functions/////////////////////////

void updateVolume()
{
    for (int i = 0; i < SINE_DATA_LENGTH; i++)
    {
        sine_data[i] = int((sine_data_raw[i] - sine_data_raw[0]) * volume) + sine_data_raw[0];
    }
}

void updateRelay(bool relayOn)
{
    digitalWrite(RELAY_SWITCH_PIN, !relayOn);
}

void updateSineWaveDACTimer()
{
    outgoingSineWaveDACTimer.update(frequencyMicroSeconds / SINE_DATA_LENGTH);
}

void beginTimersForModulation()
{
    while (!outgoingSineWaveDACTimer.begin(outgoingSineWaveDACTimerCallback, frequencyMicroSeconds / SINE_DATA_LENGTH))
    {
        ;
    }
    while (!signalSweep.begin(updateFrequency, 100000))
    {
        ;
    }
}

void outgoingSineWaveDACTimerCallback()
{
    dac.output(byte(sine_data[sine_data_index % SINE_DATA_LENGTH]));
    sine_data_index++;
    if (sine_data_index >= SINE_DATA_LENGTH)
    {
        sine_data_index = 0;
    }
}
