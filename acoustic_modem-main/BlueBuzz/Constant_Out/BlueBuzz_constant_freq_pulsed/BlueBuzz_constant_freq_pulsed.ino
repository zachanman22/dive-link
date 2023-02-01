/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include <SPI.h>
#include <MCP49xx.h>

#define RELAY_SWITCH_PIN 14
#define SS_PIN 10 // system select pin for SPI communication utilizing MCP4901 DAC

////////////////////////////////////////////////////////////////////
// Modulation Variables
////////////////////////////////////////////////////////////////////
MCP49xx dac(MCP49xx::MCP4921, SS_PIN);

uint16_t sine_data_index = 0;
int ADCCENTER = 2048;

float volume = 0.5; // volume from 0 to 1

// https://www.wolframalpha.com/input?i=table+round%2825%2B103*%28sin%282*pi*t%2F20%29%2B1%29%29+from+0+to+19

// #define SINE_DATA_LENGTH 24                                                                                                                                // number of steps to take per sine wave
// uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 160, 189, 211, 226, 231, 226, 211, 189, 160, 128, 96, 67, 45, 30, 25, 30, 45, 67, 96}; // pre-computed sine wave
// int16_t sine_data[SINE_DATA_LENGTH];                                                                                                                       // sine-wave including volume
//
#define SINE_DATA_LENGTH 20                                                                                                                                  // number of steps to take per sine wave
uint16_t sine_data_raw[SINE_DATA_LENGTH] = {2048, 2673, 3237, 3685, 3972, 4071, 3972, 3685, 3237, 2673, 2048, 1423, 859, 411, 124, 25, 124, 411, 859, 1423}; //{128, 167, 201, 223, 231, 223, 201, 167, 128, 89, 55, 33, 25, 33, 55, 89}; // pre-computed sine wave
uint16_t sine_data[SINE_DATA_LENGTH];

//#define SINE_DATA_LENGTH 12                                                                                         // number of steps to take per sine wave
// uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 180, 217, 231, 217, 180, 128, 76, 39, 25, 39, 76}; // pre-computed sine wave
// int16_t sine_data[SINE_DATA_LENGTH];

//#define SINE_DATA_LENGTH 10                                                                                         // number of steps to take per sine wave
// uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 189, 226, 226, 189, 128, 67, 30, 30, 67}; // pre-computed sine wave
// int16_t sine_data[SINE_DATA_LENGTH];

float currentMicroseconds = 0;
float frequency = 25000;
float frequencyMicroSeconds = 1 / frequency * 1000000;
uint64_t previousCPURead;
uint64_t currentCPURead;
uint32_t freq_sample_counter = 1;
uint64_t offset_amount = 0;
const float micro_freq_period_us = float(F_CPU) / 1000000;
float dacUpdateFrequency = frequencyMicroSeconds / SINE_DATA_LENGTH;

bool isOn = true;

const float pulseFreq = 2;
const float pulseFreqMicroSeconds = 1 / pulseFreq * 1000000;
const float microSecondPerPulse = 10000;
bool pulse = false;

elapsedMicros pulserTimer = 0;

/////////////////////////Shared Functions/////////////////////////

void setup()
{
    pinMode(RELAY_SWITCH_PIN, OUTPUT);

    // begin the SPI to communicate with MCP4901 DAC
    SPI.begin();
    dac.output(0);

    Serial.begin(115200); // begin Serial communication
    delay(1000);
    Serial.printf("Frequency: %f\n", frequency);
    Serial.printf("Microseconds: %f\n", frequencyMicroSeconds);
    // turn of relay (on == transmit)
    updateRelay(false);
    delay(1000);
    updateRelay(true);
    delay(200);
}

void loop()
{
    currentMicroseconds = getMicroseconds();
    if (Serial.available())
    {
        int i = Serial.parseInt();
        if (i != 0 && i != 1 && i != 2)
        {
            updateFrequency2(i);
        }
        else
        {
            if (i == 1)
            {
                volume += 0.05;
                if (volume > 1.0)
                {
                    volume = 0.1;
                }
                Serial.print("Volume: ");
                Serial.println(volume);
            }
            if (i == 2)
            {
                isOn = !isOn;
                Serial.print("On: ");
                if (isOn)
                {
                    Serial.println("true");
                }
                else
                {
                    dac.output(0);
                    Serial.println("false");
                }
            }
        }
    }
    else
    {
        if (pulserTimer < microSecondPerPulse)
        {
            if (isOn)
            {
                updateModulation();
            }
        }
        else
        {
            // // turn on relay 3ms before activation
            // if (pulserTimer + 3000 >= pulseFreqMicroSeconds)
            // {
            //     updateRelay(true);
            // }
            // else
            // {
            //     updateRelay(false);
            // }
            if (pulserTimer >= pulseFreqMicroSeconds)
            {
                pulserTimer = 0;
            }
        }
    }
}

void updateFrequency2(int i)
{
    frequency = i;
    frequencyMicroSeconds = 1 / frequency * 1000000;
    dacUpdateFrequency = frequencyMicroSeconds / SINE_DATA_LENGTH;
    delay(1000);
    freq_sample_counter = 1;
    previousCPURead = 0;
    currentCPURead = 0;
    offset_amount = 0;
    Serial.printf("Frequency: %f\n", frequency);
    Serial.printf("Microseconds: %f\n", frequencyMicroSeconds);
}

/////////////////////////Modulation Functions///////////////////////

void updateRelay(bool relayOn)
{
    digitalWrite(RELAY_SWITCH_PIN, !relayOn);
}

void updateModulation()
{
    dac.output(uint16_t((sin(currentMicroseconds / 1000000 * frequency * 2 * PI)) * ADCCENTER * 0.95 * volume + ADCCENTER));
}

float getMicroseconds()
{
    previousCPURead = currentCPURead;
    currentCPURead = ARM_DWT_CYCCNT;
    if (previousCPURead >= currentCPURead)
    {
        // Serial.println(float((previousCPURead + offset_amount) / micro_freq_period_us));
        offset_amount = uint32_t((UINT32_MAX) - (previousCPURead - currentCPURead) + 1);
        // Serial.println(float((currentCPURead + offset_amount) / micro_freq_period_us));
        // delay(1000);
    }
    // Serial.println(float((currentCPURead + offset_amount) / micro_freq_period_us));
    return float((currentCPURead + offset_amount)) / micro_freq_period_us;
}
