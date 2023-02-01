/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include <Hamming.h>
#include "Goertzel.h"
#include "ACOMM3.h"
#include <ArduinoJson.h>
#include <SPI.h>
#include <MCP49xx.h>
#include <Entropy.h>
#include "SdFat.h"
#include "sdios.h"

#define MESSAGE_MAXIMUM_SYMBOL_SIZE 16384 * 4
#define RELAY_SWITCH_PIN 9
#define demod_pin 14
#define SS_PIN 10 // system select pin for SPI communication utilizing MCP4901 DAC

DynamicJsonDocument doc(2048);
DynamicJsonDocument responseDoc(256);
JsonObject root = doc.to<JsonObject>();

////////////////////////////////////////////////////////////////////
// Modulation Variables
////////////////////////////////////////////////////////////////////
MCP49xx dac(MCP49xx::MCP4901, SS_PIN);

uint16_t messageSymbolSize;                              // tracks the number of symbols to modulate
byte symbolValueOfMessage[MESSAGE_MAXIMUM_SYMBOL_SIZE];  // encoded symbols to modulate
byte symbolValueNotEncoded[MESSAGE_MAXIMUM_SYMBOL_SIZE]; // non-encoded symbols.

char outgoingMessagesBytes[MESSAGE_MAXIMUM_SYMBOL_SIZE / 8 + 1]; // the incoming chars/bytes are stored to be turned into non-encoded symbols
int outgoingMessagesBytesIndex;

int symbolIndex;
bool modulationActive;

uint16_t sine_data_index;

float volume = 1; // volume from 0 to 1

// modulation timers
IntervalTimer outgoingSineWaveDACTimer;
IntervalTimer outgoingSymbolUpdateTimer;

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
////////////////////////////////////////////////////////////////////
// Demodulation Variables
////////////////////////////////////////////////////////////////////
volatile bool incomingSignalDetected = false;

char incomingMessageBytes[MESSAGE_MAXIMUM_SYMBOL_SIZE / 8 + 1];

byte incomingSymbols[MESSAGE_MAXIMUM_SYMBOL_SIZE];        // encoded symbols received
byte incomingSymbolsDecoded[MESSAGE_MAXIMUM_SYMBOL_SIZE]; // parity check and decoded symbols
byte *incomingSymbolsAdj;                                 // pointer to align incoming symbol

// acomm(baud rate, update rate)
// update rate/baud rate must be an integer.
ACOMM3 acomm(20);

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
    dac.output(0);

    // turn of relay (on == transmit)
    updateRelay(false);

    // set priority of timers to prevent interruption
    outgoingSineWaveDACTimer.priority(100);
    outgoingSymbolUpdateTimer.priority(20);

    updateVolume();
    Serial.begin(115200); // begin Serial communication
    // while (!Serial)
    // {
    //     ;
    // }
    delay(100);

    // set ACOMM, which handles the Comm channels which utilize Goertzel

    Serial.println("Modem Online");
    delay(200);
    Serial.print("Baud: ");
    Serial.println(acomm.getBaud());
    Serial.print("Volume: ");
    Serial.println(volume);
    Serial.println("ACOMM3");
}

void loop()
{
    if (!modulationActive)
    {
        readIncomingSerialDataForModulation();
    }
}

void endAllTimers()
{
    outgoingSineWaveDACTimer.end();
    outgoingSymbolUpdateTimer.end();
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
    endAllTimers();
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
    modulationActive = false;
    interrupts();
}

void startModulation()
{
    endAllTimers();
    modulationActive = true;
    updateRelay(true);
    delay(50);
    symbolIndex = 0;
    sine_data_index = 0;
    dac.output(byte(sine_data[0]));
    interrupts();
    beginTimersForModulation();
}

void endTimersForModulation()
{
    outgoingSineWaveDACTimer.end();
    outgoingSymbolUpdateTimer.end();
}

void beginTimersForModulation()
{
    long microsnow = micros();
    while (!outgoingSymbolUpdateTimer.begin(outgoingSymbolUpdateTimerCallback, acomm.getUSecondsPerSymbol()))
    {
        ;
    }
    while (!outgoingSineWaveDACTimer.begin(outgoingSineWaveDACTimerCallback, convertIncomingBitToMicrosecondsPeriodForDACUpdate(symbolValueOfMessage[symbolIndex])))
    {
        ;
    }
    Serial.println(micros() - microsnow);
}

void outgoingSymbolUpdateTimerCallback()
{
    symbolIndex++;
    if (symbolIndex >= messageSymbolSize)
    {
        messageCompleteAndModulationReset();
        return;
    }
    if (symbolValueOfMessage[symbolIndex - 1] != symbolValueOfMessage[symbolIndex])
    {
        outgoingSineWaveDACTimer.update(convertIncomingBitToMicrosecondsPeriodForDACUpdate(symbolValueOfMessage[symbolIndex]));
    }
    sine_data_index = sine_data_index % SINE_DATA_LENGTH;
}

void outgoingSineWaveDACTimerCallback()
{
    dac.output(byte(sine_data[sine_data_index % SINE_DATA_LENGTH]));
    sine_data_index++;
}

float convertIncomingBitToMicrosecondsPeriodForDACUpdate(byte symbol)
{
    return acomm.getFreqMicroSecondsFromSymbol(byte(symbol)) / SINE_DATA_LENGTH;
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
    symbolValueOfMessage[outgoingMessagesBytesIndex] = 0;
    Serial.print(symbolValueOfMessage[outgoingMessagesBytesIndex]);
    symbolValueOfMessage[outgoingMessagesBytesIndex + 1] = 0;
    Serial.println(symbolValueOfMessage[outgoingMessagesBytesIndex + 1]);
    messageSymbolSize = outgoingMessagesBytesIndex + 2;
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
            if (outgoingMessagesBytesIndex < 2){
              volume -= 0.1;
              if(volume < 0.2){
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
            interrupts();
            startModulation();
        }
    }
}
