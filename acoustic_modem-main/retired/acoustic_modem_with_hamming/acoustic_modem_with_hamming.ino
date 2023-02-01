/*Performing FSK modulation utilizing a DAC on a teensy 3.2
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include <Hamming.h>
#include "Goertzel.h"
#include "ACOMM.h"

#define MESSAGE_MAXIMUM_SYMBOL_SIZE 16384
#define BAUD_RATE 2000
#define RELAY_SWITCH_PIN 11
#define demod_pin 12

//////Shared Variables///////
bool hammingActive = true; //hamming (12, 7)
Hamming hamming;
const float maxFrequency = 23000;//30612.2449; //96mhz/(30000*16)
const float minFrequency = 22000;//27027.027;
const float maxFrequencyMicroSeconds = 1.0 / maxFrequency * 1000000;
const float minFrequencyMicroSeconds = 1.0 / minFrequency * 1000000;
float uSECONDS_PER_SYMBOL;

//////Modulation Variables///////
int messageSymbolSize;
byte symbolValueOfMessage[MESSAGE_MAXIMUM_SYMBOL_SIZE];

char outgoingMessagesBytes[MESSAGE_MAXIMUM_SYMBOL_SIZE / 8 + 1];
int outgoingMessagesBytesIndex;

volatile uint16_t symbolIndex;
volatile uint16_t sine_data_index;
bool modulationActive;

float volume = 0.75;

IntervalTimer outgoingSineWaveDACTimer;
IntervalTimer outgoingSymbolUpdateTimer;

//http://www.wolframalpha.com/input/?i=table+round%28100%2B412*%28sin%282*pi*t%2F20%29%2B1%29%29+from+0+to+19
// #define SINE_DATA_LENGTH 20
// int16_t sine_data[SINE_DATA_LENGTH] = {512, 639, 754, 845, 904, 924, 904, 845, 754, 639, 512, 385, 270, 179, 120, 100, 120, 179, 270, 385};

#define SINE_DATA_LENGTH 16
int16_t sine_data_raw[SINE_DATA_LENGTH] = {512, 670, 803, 893, 924, 893, 803, 670, 512, 354, 221, 131, 100, 131, 221, 354};
int16_t sine_data[SINE_DATA_LENGTH];
//////Demodulation Variables///////
volatile bool incomingSignalDetected = false;

char incomingMessageBytes[MESSAGE_MAXIMUM_SYMBOL_SIZE / 8 + 1];

volatile byte incomingSymbols[MESSAGE_MAXIMUM_SYMBOL_SIZE];
volatile int inSymbolIndex;

volatile int messageTimeOutCounter;
int messageTimeOutThreshold = 8;

volatile int messageStartCounter;
const int messageStartThreshold = 2;

float averageMeasuredTimeBetweenPings;

volatile uint32_t cpuCycleAtRecentPing;
volatile uint32_t previousCPUCycleAtPing;
volatile int pingsCPUCycleIndex;

IntervalTimer multipleWaveFrequencyTimer;

/////////////////////////Shared Functions/////////////////////////

void setup()
{
    if (ARM_DWT_CYCCNT == ARM_DWT_CYCCNT)
    {
        // Enable CPU Cycle Count
        ARM_DEMCR |= ARM_DEMCR_TRCENA;
        ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
    }
    analogWriteResolution(10);
    pinMode(RELAY_SWITCH_PIN, OUTPUT);
    pinMode(13, OUTPUT);
    pinMode(A14, OUTPUT);
    analogWrite(A14, 0);
    digitalWrite(13, HIGH);
    updateRelay(false);
    outgoingSineWaveDACTimer.priority(100);
    outgoingSymbolUpdateTimer.priority(99);
    multipleWaveFrequencyTimer.priority(99);
    uSECONDS_PER_SYMBOL = 1 / (float(BAUD_RATE)) * 1000000;
    //outgoingSineWaveDACTimer.begin(timer0_callback, 1.25);
    updateVolume();
    Serial.begin(115200); //begin Serial communication
    while (!Serial)
    {
        ;
    }
    hamming.Init(8);
    Serial.println(hamming.getEncodedMessageLength());
    messageTimeOutThreshold = hamming.getEncodedMessageLength();
    attachStartConditionInterrupt();
    Serial.println("Modem Online"); //Send a note saying boot up
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
    multipleWaveFrequencyTimer.end();
}

/////////////////////////Modulation Functions/////////////////////////

void updateVolume()
{
    for (int i = 0; i < SINE_DATA_LENGTH; i++)
    {
        sine_data[i] = int((sine_data_raw[i] - 512) * volume) + 512;
    }
}

void updateRelay(bool relayOn)
{
    if (relayOn)
    {
        digitalWrite(RELAY_SWITCH_PIN, LOW);
    }
    else
    {
        digitalWrite(RELAY_SWITCH_PIN, HIGH);
    }
}

void messageCompleteAndModulationReset()
{
    endAllTimers();
    analogWrite(A14, 0);
    delay(50);
    updateRelay(false);
    delay(50);
    symbolIndex = 0;
    sine_data_index = 0;
    outgoingMessagesBytesIndex = 0;
    messageSymbolSize = 0;
    //Serial.println("Message Complete");
    endDemodulationWithoutMessage();
    modulationActive = false;
}

void startModulation()
{
    modulationActive = true;
    detachDemodInterrupt();
    updateRelay(true);
    delay(50);
    symbolIndex = 0;
    sine_data_index = 0;
    analogWrite(A14, sine_data[0]);
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
    while (!outgoingSineWaveDACTimer.begin(outgoingSineWaveDACTimerCallback, convertIncomingBitToMicrosecondsPeriodForDACUpdate(symbolValueOfMessage[0])))
    {
        ;
    }
    while (!outgoingSymbolUpdateTimer.begin(outgoingSymbolUpdateTimerCallback, uSECONDS_PER_SYMBOL))
    {
        ;
    }
}

void outgoingSymbolUpdateTimerCallback()
{
    symbolIndex++;
    if (symbolValueOfMessage[symbolIndex - 1] != symbolValueOfMessage[symbolIndex])
    {
        outgoingSineWaveDACTimer.update(convertIncomingBitToMicrosecondsPeriodForDACUpdate(symbolValueOfMessage[symbolIndex]));
    }
    sine_data_index = sine_data_index % SINE_DATA_LENGTH;
    if (symbolIndex >= messageSymbolSize)
    {
        messageCompleteAndModulationReset();
    }
}

void outgoingSineWaveDACTimerCallback()
{
    analogWrite(A14, sine_data[sine_data_index % SINE_DATA_LENGTH]);
    sine_data_index++;
}

float convertIncomingBitToMicrosecondsPeriodForDACUpdate(int bit)
{
    if (bit == 0)
    {
        return minFrequencyMicroSeconds / SINE_DATA_LENGTH;
    }
    else
    {
        return maxFrequencyMicroSeconds / SINE_DATA_LENGTH;
    }
}

void saveBitToSymbolOut(int index, int tempBit)
{
    symbolValueOfMessage[index] = tempBit;
}

void convertMessageCharsToSymbolsOut()
{
    byte tempBits[8];
    byte tempEncoded[hamming.getEncodedMessageLength()];
    if (hammingActive)
    {
        messageSymbolSize = outgoingMessagesBytesIndex * hamming.getEncodedMessageLength();
    }
    else
    {
        messageSymbolSize = 8 * outgoingMessagesBytesIndex;
    }
    for (int k = 0; k < outgoingMessagesBytesIndex; k++)
    {
        for (int i = 7; i >= 0; i--)
        {
            tempBits[(7 - i)] = (outgoingMessagesBytes[k] >> i) & 1;
        }
        if (hammingActive)
        {
            hamming.encode(tempBits, tempEncoded);
            for (int j = 0; j < hamming.getEncodedMessageLength(); j++)
            {
                saveBitToSymbolOut(k * hamming.getEncodedMessageLength() + j, tempEncoded[j]);
            }
        }
        else
        {
            for (int j = 0; j < 8; j++)
            {
                saveBitToSymbolOut(k * 8 + j, tempEncoded[j]);
            }
        }
    }
}

void readIncomingSerialDataForModulation()
{
    if (Serial.available())
    {
        if (outgoingMessagesBytesIndex == 0)
        {
            interruptDemodulationWithModulation();
            outgoingMessagesBytes[outgoingMessagesBytesIndex++] = char(247);
            return;
        }

        outgoingMessagesBytes[outgoingMessagesBytesIndex++] = Serial.read();
        if (outgoingMessagesBytes[outgoingMessagesBytesIndex - 1] == '\n') //if character is a new line, end of message
        {
            convertMessageCharsToSymbolsOut();
            Serial.println();
            Serial.print("[");
            for (int i = 0; i < messageSymbolSize; i++)
            {
                Serial.print(symbolValueOfMessage[i]);
                Serial.print(" ");
            }
            Serial.println("]");
            Serial.println(messageSymbolSize);
            //Serial.println("Starting Message Modulation");
            interrupts();
            startModulation();
        }
    }
}

/////////////////////////Demodulation Functions/////////////////////////

FASTRUN void pingSpeedTimerCallback()
{
    //record the CPU cycle count at the time of incoming rising edge of incoming frequency
    if (cpuCycleAtRecentPing == 0)
    {
        previousCPUCycleAtPing = ARM_DWT_CYCCNT;
    }
    cpuCycleAtRecentPing = ARM_DWT_CYCCNT;
    pingsCPUCycleIndex++;
}

FASTRUN void startConditionMetCallback()
{
    //if this function is called and the incoming signal has not been detected
    //start the demodulation
    noInterrupts();
    if (incomingSignalDetected || modulationActive)
    {
        interrupts();
        return;
    }
    if (messageStartCounter == 0)
    {
        messageStartCounter += 1;
        previousCPUCycleAtPing = ARM_DWT_CYCCNT;
        interrupts();
        return;
    }
    messageStartCounter += 1;
    cpuCycleAtRecentPing = ARM_DWT_CYCCNT;
    if (messageStartCounter != messageStartThreshold)
    {
        interrupts();
        return;
    }
    float averageMeasuredTimeBetweenPings = getTimeDifferenceMicrosFromSavedCPUPings(cpuCycleAtRecentPing, previousCPUCycleAtPing) / (messageStartCounter - 1);
    if (averageMeasuredTimeBetweenPings < maxFrequencyMicroSeconds - 5 || averageMeasuredTimeBetweenPings > minFrequencyMicroSeconds + 5)
    {
        messageStartCounter = 0;
        interrupts();
        return;
    }
    messageStartCounter = 0;
    cpuCycleAtRecentPing = 0;
    previousCPUCycleAtPing = 0;
    incomingSignalDetected = true;
    endAllTimers();
    //detachInterrupts on demodpin and attachRunning intterupt
    detachDemodInterrupt();
    attachRunningTimerInterrupt();
    //start the multiplWaveFrequencyTimer. This timer is called every uSeconds_PER_SYMBOL,
    //This timer computes the frequency of each incoming period within the uSeconds_Per_Symbol.
    while (!multipleWaveFrequencyTimer.begin(multipleWaveFrequencyTimerCallback, uSECONDS_PER_SYMBOL))
    {
        ;
    }
}

void detachDemodInterrupt()
{
    //detaches intterupts that either check for start condition or running timer interrupt
    detachInterrupt(digitalPinToInterrupt(demod_pin));
}
void attachRunningTimerInterrupt()
{
    //attach the interrupt the read the incoming micros
    attachInterrupt(digitalPinToInterrupt(demod_pin), &pingSpeedTimerCallback, RISING);
}
void attachStartConditionInterrupt()
{
    //attach the interrupt that checks for a rising edge on incoming signal.
    attachInterrupt(digitalPinToInterrupt(demod_pin), &startConditionMetCallback, RISING);
}

int convertMicrosDifferenceToNearestSymbol(float microsDifference)
{
    //check which value the read micros difference between waves is closer to, the center frequency of secondary frequency
    if (abs(microsDifference - maxFrequencyMicroSeconds) >= abs(minFrequencyMicroSeconds - microsDifference))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

bool checkForEndedMessageCondition()
{
    //Checks to see if enough 0s or 1s have been received in a row to trigger a message end
    if (incomingSymbols[inSymbolIndex] == incomingSymbols[inSymbolIndex - 1])
    {
        //increment message time out counter. Once it is equal to or greater than messageTimeOutThreshold,
        //the incoming message has ended
        messageTimeOutCounter++;
        //end the demodulation if the counter is greater than or equal to the threshold
        if (messageTimeOutCounter >= messageTimeOutThreshold)
        {
            endDemodulation();
            //return true since the message has ended
            return true;
        }
    }
    else
    {
        //reset the message time out counter since the value has changed
        messageTimeOutCounter = 0;
    }
    //return false since the message did not end
    return false;
}

void interruptDemodulationWithModulation()
{
    endAllTimers();
    //reset all tracking variables
    pingsCPUCycleIndex = 0;
    cpuCycleAtRecentPing = 0;
    messageTimeOutCounter = 0;
    inSymbolIndex = 0;
    averageMeasuredTimeBetweenPings = 0;
    detachDemodInterrupt();
    incomingSignalDetected = false;
}

void endDemodulationWithoutMessage()
{
    endAllTimers();
    //reset all tracking variables
    pingsCPUCycleIndex = 0;
    cpuCycleAtRecentPing = 0;
    messageTimeOutCounter = 0;
    inSymbolIndex = 0;
    averageMeasuredTimeBetweenPings = 0;
    detachDemodInterrupt();
    interrupts();
    //attach intterupt timer to check for signal start conditions.
    attachStartConditionInterrupt();
    //reset demoulation flag
    incomingSignalDetected = false;
}

void endDemodulation()
{
    //end frequency timer to stop looking for incoming signal
    endAllTimers();
    //align incoming bits with lead char. This corrects for if the first
    //bit was missed or double counted
    correctForShiftedMessage();
    //take symbols read in, and convert into bytes
    convertSymbolsIntoMessageBytes();
    //publish the data to the Serial port
    //publishBinaryMessage();
    publishIncomingData();

    //reset all tracking variables
    pingsCPUCycleIndex = 0;
    cpuCycleAtRecentPing = 0;
    messageTimeOutCounter = 0;
    inSymbolIndex = 0;
    averageMeasuredTimeBetweenPings = 0;
    detachDemodInterrupt();
    interrupts();
    //attach intterupt timer to check for signal start conditions.
    attachStartConditionInterrupt();
    //reset demoulation flag
    incomingSignalDetected = false;
}

void addSymbolToIncomingSymbols(int symbol)
{
    incomingSymbols[inSymbolIndex] = symbol;
}

float getTimeDifferenceMicrosFromSavedCPUPings(uint32_t currentVal, uint32_t previousVal)
{
    //take the CPUCylceCount at two rising edges, current and previous, and calculate the microseconds between
    if (previousVal < currentVal)
    {
        return (float(currentVal - previousVal)) / F_CPU * 1000000;
    }
    else
    {
        //accounts for overflow in the CPU cycle counter. For the teensy 3.2 at 96MHz, this happens every ~44 seconds
        return float(currentVal + (UINT32_MAX - previousVal)) / F_CPU * 1000000;
    }
}

FASTRUN void multipleWaveFrequencyTimerCallback()
{
    //remove demod interrupt to prevent interrupt of this timer (reattached at end of function)
    noInterrupts();
    if (pingsCPUCycleIndex - 1 <= 0)
    {
        endDemodulation();
        return;
    }
    averageMeasuredTimeBetweenPings = getTimeDifferenceMicrosFromSavedCPUPings(cpuCycleAtRecentPing, previousCPUCycleAtPing) / (pingsCPUCycleIndex - 1);
    if (averageMeasuredTimeBetweenPings < maxFrequencyMicroSeconds / 2 || averageMeasuredTimeBetweenPings > minFrequencyMicroSeconds * 2)
    {
        endDemodulation();
        return;
    }
    //convert the averageMeasuredTimeBetweenPings micros into the nearest symbol
    //Serial.println(averageMeasuredTimeBetweenPings);
    addSymbolToIncomingSymbols(convertMicrosDifferenceToNearestSymbol(averageMeasuredTimeBetweenPings));
    //check if the proper conditions have been met to end the incoming demodulation
    if (checkForEndedMessageCondition())
    {
        return;
    }
    //increment symbolIndex
    cpuCycleAtRecentPing = 0;
    inSymbolIndex++;
    averageMeasuredTimeBetweenPings = 0;
    pingsCPUCycleIndex = 0;
    //reattach timer interrupt
    interrupts();
}

// void correctForShiftedMessage()
// {
//     if (inSymbolIndex < 4)
//     {
//         return;
//     }
//     int shiftedValueLower = 0;
//     int shiftedValueUpper = 0;
//     int tempIndex = 0;
//     for (int i = 0; i < 4; i++)
//     {
//         shiftedValueLower += incomingSymbols[i];
//         shiftedValueUpper += incomingSymbols[i + 4];
//     }
//     int i = 0;
//     while (i + 1 < 20 && i + 1 < inSymbolIndex)
//     {
//         if (shiftedValueLower == 4 && shiftedValueUpper == 3)
//         {
//             break;
//         }
//         i += 1;
//         shiftedValueLower -= incomingSymbols[i - 1];
//         shiftedValueLower += incomingSymbols[i + 3];
//         shiftedValueUpper -= incomingSymbols[i - 1 + 4];
//         shiftedValueUpper += incomingSymbols[i + 7];
//     }
//     if (i == 0)
//     {
//         return;
//     }
//     for (int k = i; k < inSymbolIndex; k++)
//     {
//         incomingSymbols[k - i] = incomingSymbols[k];
//     }
//     Serial.println("shifted");
// }

void correctForShiftedMessage()
{
    if (incomingSymbols[4] == 0)
    {
        return;
    }
    //Serial.println("had to shift message");
    int pos = 0;
    while (incomingSymbols[pos] != 0)
    {
        pos++;
    }
    int correctAmount = abs(pos - 4);
    if (pos - 4 > 0)
    {
        for (int i = 0; i < inSymbolIndex; i++)
        {
            incomingSymbols[i] = incomingSymbols[i + correctAmount];
        }
        inSymbolIndex -= correctAmount;
    }
    else
    {
        for (int i = inSymbolIndex - 1; i >= 0; i--)
        {
            incomingSymbols[i + correctAmount] = incomingSymbols[i];
        }
        for (int i = 0; i < correctAmount; i++)
        {
            incomingSymbols[i] = 1;
        }
        inSymbolIndex += correctAmount;
    }
}

void publishBinaryMessage()
{
    for (int i = 0; i < inSymbolIndex; i++)
    {
        if (i % hamming.getEncodedMessageLength() == 0 && i != 0)
        {
            Serial.print(" ");
        }
        Serial.print(incomingSymbols[i]);
    }
    Serial.println();
}

void publishIncomingData() //publishes the available data in bytesIn to the Serial stream
{
    int pos = 0;
    if (hammingActive)
    {
        pos = inSymbolIndex / hamming.getEncodedMessageLength();
    }
    else
    {
        pos = inSymbolIndex / 8;
    }
    while (pos > 1 && incomingMessageBytes[pos] != '\n')
    {
        pos--;
    }
    if (pos <= 1)
    {
        return;
    }
    incomingMessageBytes[pos] = 0;
    Serial.println(incomingMessageBytes + 1);
}

void convertSymbolsIntoMessageBytes()
{
    int hammingAdjuster = 8;
    if (hammingActive)
    {
        hammingAdjuster += hamming.getParityBitsLength();
        hamming.parityCheckAndErrorCorrection(incomingSymbols);
    }
    for (int i = 0; i < inSymbolIndex; i += hammingAdjuster)
    {
        incomingMessageBytes[i / hammingAdjuster] = convertSymbolsToByteAtIndex(i);
    }
}

char convertSymbolsToByteAtIndex(int incomingSymbolsTempIndex)
{
    int byteAtIndex = (incomingSymbols[incomingSymbolsTempIndex + 7]) + (incomingSymbols[incomingSymbolsTempIndex + 6] * 2) +
                      (incomingSymbols[incomingSymbolsTempIndex + 5] * 4) + (incomingSymbols[incomingSymbolsTempIndex + 4] * 8) +
                      (incomingSymbols[incomingSymbolsTempIndex + 3] * 16) + (incomingSymbols[incomingSymbolsTempIndex + 2] * 32) +
                      (incomingSymbols[incomingSymbolsTempIndex + 1] * 64) + (incomingSymbols[incomingSymbolsTempIndex + 0] * 128);
    return char(byteAtIndex);
}
