/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include <SPI.h>
#include <MCP49xx.h>

#define RELAY_SWITCH_PIN 14
#define SS_PIN 10 // system select pin for SPI communication utilizing MCP4901 DAC
#define TRIGGER_PIN 15

////////////////////////////////////////////////////////////////////
// Modulation Variables
////////////////////////////////////////////////////////////////////
MCP49xx dac(MCP49xx::MCP4921, SS_PIN);

uint16_t sine_data_index = 0;

float volume = 0.5; // volume from 0 to 1

// https://www.wolframalpha.com/input?i=table+round%2825%2B103*%28sin%282*pi*t%2F20%29%2B1%29%29+from+0+to+19

// #define SINE_DATA_LENGTH 24                                                                                                                                // number of steps to take per sine wave
// uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 160, 189, 211, 226, 231, 226, 211, 189, 160, 128, 96, 67, 45, 30, 25, 30, 45, 67, 96}; // pre-computed sine wave
// int16_t sine_data[SINE_DATA_LENGTH];                                                                                                                       // sine-wave including volume
//
#define SINE_DATA_LENGTH 20                                                                                          // number of steps to take per sine wave
uint16_t sine_data_raw[SINE_DATA_LENGTH] = {2048, 2673, 3237, 3685, 3972, 4071, 3972, 3685, 3237, 2673, 2048, 1423, 859, 411, 124, 25, 124, 411, 859, 1423};//{128, 167, 201, 223, 231, 223, 201, 167, 128, 89, 55, 33, 25, 33, 55, 89}; // pre-computed sine wave
uint16_t sine_data[SINE_DATA_LENGTH];

//#define SINE_DATA_LENGTH 12                                                                                         // number of steps to take per sine wave
// uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 180, 217, 231, 217, 180, 128, 76, 39, 25, 39, 76}; // pre-computed sine wave
// int16_t sine_data[SINE_DATA_LENGTH];

//#define SINE_DATA_LENGTH 10                                                                                         // number of steps to take per sine wave
// uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 189, 226, 226, 189, 128, 67, 30, 30, 67}; // pre-computed sine wave
// int16_t sine_data[SINE_DATA_LENGTH];

float currentMicroseconds = 0;
float frequency = 32000;
float frequencyMicroSeconds = 1 / frequency * 1000000;
uint64_t previousCPURead;
uint64_t currentCPURead;
uint32_t freq_sample_counter = 1;
uint64_t offset_amount = 0;
const float micro_freq_period_us = float(F_CPU) / 1000000;
float dacUpdateFrequency = frequencyMicroSeconds / SINE_DATA_LENGTH;

bool isOn = true;

long freqStepCounter = 0;
const float triggerFrequency = 250000.0;
const float triggerPeriodSeconds = 1/triggerFrequency;
int nextDacValue = 0;
float omega = 2*PI*frequency;

/////////////////////////Shared Functions/////////////////////////

void setup()
{
    pinMode(RELAY_SWITCH_PIN, OUTPUT);
    pinMode(TRIGGER_PIN, INPUT_PULLUP);
    

    // begin the SPI to communicate with MCP4901 DAC
    SPI.begin();
    dac.output(0);

    updateVolume();
    Serial.begin(115200); // begin Serial communication
    delay(1000);
    Serial.printf("Frequency: %f\n", frequency);
    Serial.printf("Microseconds: %f\n", frequencyMicroSeconds);
    // turn of relay (on == transmit)
    updateRelay(false);
    delay(1000);
    updateRelay(true);
    delay(200);
    nextDacValue = sine_data_raw[0];
    attachInterrupt(digitalPinToInterrupt(TRIGGER_PIN),output_freq,RISING);
}

void output_freq(){
    dac.output(nextDacValue);
    nextDacValue = int((sine_data_raw[0]-sine_data_raw[0]*0.05)*sin(omega*(++freqStepCounter)*triggerPeriodSeconds) + sine_data_raw[0]);
}

void loop()
{
    // currentMicroseconds = getMicroseconds();
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
                updateVolume();
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
    // else
    // {
    //     if (isOn)
    //     {
    //         updateModulation();
    //     }
    // }
}

void updateFrequency2(int i)
{
    detachInterrupt(digitalPinToInterrupt(TRIGGER_PIN));
    freqStepCounter = 0;
    frequency = i;
    omega = frequency*2*PI;
    frequencyMicroSeconds = 1 / frequency * 1000000;
    delay(1000);
    Serial.printf("Frequency: %f\n", frequency);
    Serial.printf("Microseconds: %f\n", frequencyMicroSeconds);
    delay(100);
    attachInterrupt(digitalPinToInterrupt(TRIGGER_PIN),output_freq,RISING);
    
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

void outgoingSineWaveDACTimerCallback()
{
    dac.output(sine_data[sine_data_index]);
    sine_data_index++;
    if (sine_data_index >= SINE_DATA_LENGTH)
    {
        sine_data_index = 0;
    }
}

void updateModulation()
{
    if (currentMicroseconds >= freq_sample_counter * dacUpdateFrequency)
    {
        freq_sample_counter += 1;
        outgoingSineWaveDACTimerCallback();
    }
}

float getMicroseconds()
{
    previousCPURead = currentCPURead;
    currentCPURead = ARM_DWT_CYCCNT;
    if (previousCPURead >= currentCPURead)
    {
        // Serial.println(float((previousCPURead + offset_amount) / micro_freq_period_us));
        offset_amount = uint32_t((UINT32_MAX) - (previousCPURead - currentCPURead) + 1);
        freq_sample_counter = 0;
        // Serial.println(float((currentCPURead + offset_amount) / micro_freq_period_us));
        // delay(1000);
    }
    // Serial.println(float((currentCPURead + offset_amount) / micro_freq_period_us));
    return float((currentCPURead + offset_amount) / micro_freq_period_us);
}
