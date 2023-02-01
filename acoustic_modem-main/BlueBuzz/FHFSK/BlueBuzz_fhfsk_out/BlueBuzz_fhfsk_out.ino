/*Performing FSK modulation utilizing a DAC on a teensy 4.1
   Author: Scott Mayberry
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
#define g0 8
#define g1 7
#define g2 6

DynamicJsonDocument doc(2048);
DynamicJsonDocument responseDoc(256);
JsonObject root = doc.to<JsonObject>();

////////////////////////////////////////////////////////////////////
// Modulation Variables
////////////////////////////////////////////////////////////////////
MCP49xx dac(MCP49xx::MCP4921, dac_cs_pin);

uint16_t messageSymbolSize;                              // tracks the number of symbols to modulate
byte symbolValueOfMessage[MESSAGE_MAXIMUM_SYMBOL_SIZE];  // encoded symbols to modulate
byte symbolValueNotEncoded[MESSAGE_MAXIMUM_SYMBOL_SIZE]; // non-encoded symbols.

char outgoingMessagesBytes[MESSAGE_MAXIMUM_SYMBOL_SIZE / 8 + 1]; // the incoming chars/bytes are stored to be turned into non-encoded symbols
int outgoingMessagesBytesIndex;

int symbolIndex;
bool modulationActive;

uint16_t sine_data_index;

float volume = 0.03; // volume from 0 to 1

// https://www.wolframalpha.com/input?i=table+round%2825%2B103*%28sin%282*pi*t%2F20%29%2B1%29%29+from+0+to+19
// sine-wave including volume
//
#define SINE_DATA_LENGTH 16                                                                                                             // number of steps to take per sine wave
uint16_t sine_data_raw[SINE_DATA_LENGTH] = {2048, 2793, 3425, 3848, 3996, 3848, 3425, 2793, 2048, 1303, 671, 248, 100, 248, 671, 1303}; //{128, 167, 201, 223, 231, 223, 201, 167, 128, 89, 55, 33, 25, 33, 55, 89}; // pre-computed sine wave
int16_t sine_data[SINE_DATA_LENGTH];

const int number_of_vol_levels = 20;
int16_t sine_data_by_vol[number_of_vol_levels][SINE_DATA_LENGTH];
int volume_index = 0;

// acomm(baud rate, update rate)
// update rate/baud rate must be an integer.
// { 47000, 0.95}
const int num_frequency_pair = 6;
// float frequency_data[num_frequency_pair][4] = {{25000, 0.3, 32500, 0.5}};
//  6-6-22 tests
float frequency_data[num_frequency_pair][4] = {{25000, 0.45, 33000, 0.2},
                                               {28000, 0.35, 36000, 0.45},
                                               {26000, 0.2, 34500, 0.5},
                                               {30000, 0.4, 36500, 0.7},
                                               {27000, 0.25, 35500, 0.35},
                                               {32500, 0.5, 42000, 0.85}};

ACOMM3 acomm(100);

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

void convertFrequencyDataToMicroSecondsPerDACUpdate()
{
  for (int i = 0; i < num_frequency_pair; i++)
  {
    frequency_data[i][0] = (1 / frequency_data[i][0]) * 1000000 / SINE_DATA_LENGTH;
    frequency_data[i][2] = (1 / frequency_data[i][2]) * 1000000 / SINE_DATA_LENGTH;
  }
}

void setVolumeSineData()
{
  for (int i = number_of_vol_levels - 1; i >= 0; i--)
  {
    for (int k = 0; k < SINE_DATA_LENGTH; k++)
    {
      sine_data_by_vol[i][k] = int16_t((sine_data_raw[k] - sine_data_raw[0]) * ((float(i + 1)) / number_of_vol_levels)) + sine_data_raw[0];
      // Serial.println(sine_data_by_vol[i][k]);
    }
  }
}

void setup()
{
  setupPins();
  // begin the SPI to communicate with MCP4901 DAC
  SPI.begin();
  dac.output(0);

  // turn of relay (on == transmit)
  updateRelay(false);
  convertFrequencyDataToMicroSecondsPerDACUpdate();

  updateVolume();
  Serial.begin(115200); // begin Serial communication
  delay(2000);
  setVolumeSineData();
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

void setupPins()
{
  pinMode(RELAY_SWITCH_PIN, OUTPUT);
  pinMode(g0, OUTPUT);
  pinMode(g1, OUTPUT);
  pinMode(g2, OUTPUT);
  digitalWrite(g0, HIGH);
  digitalWrite(g1, LOW);
  digitalWrite(g2, LOW);
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
  if (currentMicroseconds >= freq_sample_counter * dac_freq_us)
  {
    freq_sample_counter += 1;
    outgoingSineWaveDACTimerCallback();
  }
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
  digitalWrite(RELAY_SWITCH_PIN, false);
  // digitalWrite(RELAY_SWITCH_PIN, !relayOn);
}

void messageCompleteAndModulationReset()
{
  dac.output(0);
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

float getFreqMicroSecondsFromSymbol(int symbol)
{
  volume_index = max(0, int(frequency_data[symbolIndex % num_frequency_pair][symbol * 2 + 1] * number_of_vol_levels * volume) - 1);

  // volume_index = int(frequency_data[symbolIndex % num_frequency_pair][(symbol+1) * 2]*number_of_vol_levels);
  // volume_index = (volume_index+1)%num_frequency_pair;
  // Serial.println(int  (1/(frequency_data[symbolIndex % num_frequency_pair][symbol * 2]*SINE_DATA_LENGTH/1000000)));
  // Serial.println(volume_index);
  return frequency_data[symbolIndex % num_frequency_pair][symbol * 2];
}

void startModulation()
{
  //    delay(8000);
  modulationActive = true;
  updateRelay(true);
  delay(50);
  symbolIndex = 0;
  sine_data_index = 0;
  dac.output(uint16_t(sine_data[0]));
  dac_freq_us = getFreqMicroSecondsFromSymbol(symbolValueOfMessage[0]);
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
  dac_freq_us = getFreqMicroSecondsFromSymbol(symbolValueOfMessage[symbolIndex]);
  freq_sample_counter = int(currentMicroseconds / dac_freq_us) + 1;
  if (symbolIndex >= messageSymbolSize)
  {
    messageCompleteAndModulationReset();
    return;
  }
  sine_data_index = sine_data_index % SINE_DATA_LENGTH;
}

void outgoingSineWaveDACTimerCallback()
{
  dac.output(uint16_t(sine_data_by_vol[volume_index][sine_data_index]));
  sine_data_index++;
  if (sine_data_index >= SINE_DATA_LENGTH)
    sine_data_index = sine_data_index % SINE_DATA_LENGTH;
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
