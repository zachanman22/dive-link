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

float volume = 0.7; // volume from 0 to 1

// modulation timers
IntervalTimer outgoingSineWaveDACTimer;
IntervalTimer signalSweep;

// https://www.wolframalpha.com/input?i=table+round%2825%2B103*%28sin%282*pi*t%2F20%29%2B1%29%29+from+0+to+19

// #define SINE_DATA_LENGTH 24                                                                                                                                // number of steps to take per sine wave
// uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 160, 189, 211, 226, 231, 226, 211, 189, 160, 128, 96, 67, 45, 30, 25, 30, 45, 67, 96}; // pre-computed sine wave
// int16_t sine_data[SINE_DATA_LENGTH];                                                                                                                       // sine-wave including volume
//
#define SINE_DATA_LENGTH 16                                                                                          // number of steps to take per sine wave
uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 167, 201, 223, 231, 223, 201, 167, 128, 89, 55, 33, 25, 33, 55, 89}; // pre-computed sine wave
int16_t sine_data[SINE_DATA_LENGTH];

//#define SINE_DATA_LENGTH 12                                                                                         // number of steps to take per sine wave
//uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 180, 217, 231, 217, 180, 128, 76, 39, 25, 39, 76}; // pre-computed sine wave
//int16_t sine_data[SINE_DATA_LENGTH];


//#define SINE_DATA_LENGTH 10                                                                                         // number of steps to take per sine wave
//uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 189, 226, 226, 189, 128, 67, 30, 30, 67}; // pre-computed sine wave
//int16_t sine_data[SINE_DATA_LENGTH];



float frequency = 32000;
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
        int i = Serial.parseInt();
        if (i != 0)
        {
            updateFrequency2(i);
        }
    }
}

void updateFrequency2(int i)
{
    frequency = i;
    frequencyMicroSeconds = 1 / frequency * 1000000;
    Serial.printf("Frequency: %f\n", frequency);
    Serial.printf("Microseconds: %f\n", frequencyMicroSeconds);
    updateSineWaveDACTimer();
}

void updateFrequency()
{
    frequency += 100;
    if (frequency >= 34000)
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
    // while (!signalSweep.begin(updateFrequency, 100000))
    // {
    //     ;
    // }
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
