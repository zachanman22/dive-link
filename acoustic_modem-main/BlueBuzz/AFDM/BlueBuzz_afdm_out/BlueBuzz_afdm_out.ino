/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include "ACOMM3.h"
#include <ArduinoJson.h>
#include <SPI.h>
#include <MCP49xx.h>
//#include <Entropy.h>
#include "sdios.h"

#define MESSAGE_MAXIMUM_SYMBOL_SIZE 16384 * 4
#define RELAY_SWITCH_PIN 14
#define dac_cs_pin 10 // system select pin for SPI communication utilizing MCP4901 DAC

DynamicJsonDocument doc(2048);
DynamicJsonDocument responseDoc(256);
JsonObject root = doc.to<JsonObject>();

////////////////////////////////////////////////////////////////////
// Modulation Variables
////////////////////////////////////////////////////////////////////
MCP49xx dac(MCP49xx::MCP4901, dac_cs_pin);

uint16_t messageSymbolSize;                              // tracks the number of symbols to modulate
byte symbolValueOfMessage[MESSAGE_MAXIMUM_SYMBOL_SIZE];  // encoded symbols to modulate
byte symbolValueNotEncoded[MESSAGE_MAXIMUM_SYMBOL_SIZE]; // non-encoded symbols.

char outgoingMessagesBytes[MESSAGE_MAXIMUM_SYMBOL_SIZE / 8 + 1]; // the incoming chars/bytes are stored to be turned into non-encoded symbols
int outgoingMessagesBytesIndex;

int symbolIndex;
bool modulationActive;

uint16_t sine_data_index;

float volume = 0.3; // volume from 0 to 1

// https://www.wolframalpha.com/input?i=table+round%2825%2B103*%28sin%282*pi*t%2F20%29%2B1%29%29+from+0+to+19

// #define SINE_DATA_LENGTH 24                                                                                                                                // number of steps to take per sine wave
// uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 160, 189, 211, 226, 231, 226, 211, 189, 160, 128, 96, 67, 45, 30, 25, 30, 45, 67, 96}; // pre-computed sine wave
// int16_t sine_data[SINE_DATA_LENGTH];                                                                                                                       // sine-wave including volume
//
#define SINE_DATA_LENGTH 16                                                                                          // number of steps to take per sine wave
uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 167, 201, 223, 231, 223, 201, 167, 128, 89, 55, 33, 25, 33, 55, 89}; // pre-computed sine wave
int16_t sine_data[SINE_DATA_LENGTH];



//#define SINE_DATA_LENGTH 12                                                                                         // number of steps to take per sine wave
// uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 180, 217, 231, 217, 180, 128, 76, 39, 25, 39, 76}; // pre-computed sine wave
// int16_t sine_data[SINE_DATA_LENGTH];

//#define SINE_DATA_LENGTH 10                                                                                         // number of steps to take per sine wave
// uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 189, 226, 226, 189, 128, 67, 30, 30, 67}; // pre-computed sine wave
// int16_t sine_data[SINE_DATA_LENGTH];

// acomm(baud rate, update rate)
// update rate/baud rate must be an integer.
// { 47000, 0.95}
const int num_frequency_pair = 12;
int bit_index[num_frequency_pair];
float frequency_data[num_frequency_pair][2] = {{25000, 0.65},
                                               {33000, 0.45},
                                               {28000, 0.4},
                                               {36000, 0.45},
                                               {26000, 0.4},
                                               {34500, 0.5},
                                               {30000, 0.4},
                                               {36500, 0.6},
                                               {27000, 0.3},
                                               {35500, 0.55},
                                               {32500, 0.3},
                                               {42000, 0.8}};

ACOMM3 acomm(50);

float currentMicroseconds;
uint64_t previousCPURead;
uint64_t currentCPURead;
uint32_t freq_sample_counter = 0;
uint64_t offset_amount = 0;
uint32_t symbolUpdateTimerCounter = 1;
const float micro_freq_period_us = float(F_CPU) / 1000000;
float dac_freq_us = 10;

elapsedMicros symbolUpdateTimer;

/////////////////////////Shared Functions/////////////////////////

void convertFrequencyDataToPrecomputedWave()
{
    for (int i = 0; i < num_frequency_pair; i++)
    {
        frequency_data[i][0] = frequency_data[i][0] * PI / 1000000.0;
    }
}

void normalizeVolumeThresholds()
{
    float sum = 0;
    for (int i = 0; i < num_frequency_pair; i++)
    {
        sum += frequency_data[i][1];
    }
    for (int i = 0; i < num_frequency_pair; i++)
    {
        frequency_data[i][1] /= sum;
    }
}

void setup()
{
    pinMode(RELAY_SWITCH_PIN, OUTPUT);

    // begin the SPI to communicate with MCP4901 DAC
    SPI.begin();
    dac.output(0);

    // turn of relay (on == transmit)
    updateRelay(false);
    convertFrequencyDataToMicroSecondsPerDACUpdate();

    updateVolume();
    Serial.begin(115200); // begin Serial communication
    delay(2000);
    Serial.println("Modem Online");
    Serial.print("Baud: ");
    Serial.println(acomm.getBaud());
    Serial.print("Volume: ");
    Serial.println(volume);
    Serial.println("ACOMM3");
    delay(500);
    symbolUpdateTimerCounter = 1;
    symbolUpdateTimer = 0;
}

void loop()
{
    currentMicroseconds = getMicroseconds();
    if (modulationActive)
    {
        updateModulation();
    }
    else
    {
        readIncomingSerialDataForModulation();
    }
}

void updateModulation()
{
    // Serial.print(currentMicroseconds);
    // Serial.printf(" ");
    // Serial.println(freq_sample_counter * dac_freq_us);
    float dacValue = 0;
    for(int i = 0; i < num_frequency_pair; i++)
    {
        if(symbolValueOfMessage[symbolIndex*num_frequency_pair+i] == 1)
        {
            dacValue += frequency_data[i][1]*sin(frequency_data[i][0]*currentMicroseconds);
        }
    }
    dacValue = dacValue * 2047 + 2048;
    dac.output(int(dacValue));

    if (symbolUpdateTimer >= symbolUpdateTimerCounter * acomm.getUSecondsPerSymbol())
    {
        if (symbolUpdateTimer > 10000000)
        {
            symbolUpdateTimer = 0;
            symbolUpdateTimerCounter = 0;
        }
        symbolUpdateTimerCounter++;
        outgoingSymbolUpdateTimerCallback();
    }
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

void messageCompleteAndModulationReset()
{
    dac.output(byte(0));
    Serial.println("Message Complete");

    // ground output, then disconnect transducer from amplifier
    delay(50);
    updateRelay(false);
    delay(50);

    symbolIndex = 0;
    sine_data_index = 0;
    outgoingMessagesBytesIndex = 0;
    messageSymbolSize = 0;
    symbolUpdateTimerCounter = 1;
    modulationActive = false;
}

void startModulation()
{
    modulationActive = true;
    updateRelay(true);
    delay(50);
    symbolIndex = 0;
    sine_data_index = 0;
    symbolUpdateTimerCounter = 1;
    freq_sample_counter = int(currentMicroseconds / dac_freq_us) + 1;
    symbolUpdateTimer = 0;
}

float getMicroseconds()
{
    previousCPURead = currentCPURead;
    currentCPURead = ARM_DWT_CYCCNT;
    if (previousCPURead >= currentCPURead)
    {
        offset_amount = uint32_t((UINT32_MAX) - (previousCPURead - currentCPURead) + 1);
        freq_sample_counter = 0;
    }
    return float((currentCPURead + offset_amount) / micro_freq_period_us);
}

void outgoingSymbolUpdateTimerCallback()
{
    symbolIndex++;
    if (symbolIndex*num_frequency_pair >= messageSymbolSize)
    {
        messageCompleteAndModulationReset();
        return;
    }
}

void saveBitToSymbolOut(int index, int tempBit)
{
    symbolValueOfMessage[index] = tempBit;
}

bool convertBitsOutToSymbolsOut()
{
    symbolValueOfMessage[0] = acomm.getNumSymbols() - 1;
    Serial.print(symbolValueOfMessage[0]);
    for (int i = 1; i < outgoingMessagesBytesIndex; i += 1)
    {
        int symbol = int(outgoingMessagesBytes[i - 1] - '0');
        if (symbol < 0 || symbol > 9)
        {
            Serial.println("Message stopped");
            return false;
        }
        symbolValueOfMessage[i] = symbol;
        Serial.print(symbolValueOfMessage[i]);
    }
    Serial.println();
    // symbolValueOfMessage[outgoingMessagesBytesIndex] = 0;
    // Serial.print(symbolValueOfMessage[outgoingMessagesBytesIndex]);
    // symbolValueOfMessage[outgoingMessagesBytesIndex + 1] = 0;
    // Serial.println(symbolValueOfMessage[outgoingMessagesBytesIndex + 1]);
    messageSymbolSize = outgoingMessagesBytesIndex;
    return true;
}

void readIncomingSerialDataForModulation()
{
    if (Serial.available())
    {
        char newestChar = Serial.read();
        outgoingMessagesBytes[outgoingMessagesBytesIndex++] = newestChar;
        if (newestChar == '\n')
        {
            if (outgoingMessagesBytesIndex < 2)
            {
                volume -= 0.1;
                if (volume <= 0.005)
                {
                    volume = 1;
                }
                updateVolume();
                Serial.print("Volume: ");
                Serial.println(volume);
                messageCompleteAndModulationReset();
                return;
            }
            if (!convertBitsOutToSymbolsOut())
            {
                messageCompleteAndModulationReset();
                return;
            }
            startModulation();
        }
    }
}
