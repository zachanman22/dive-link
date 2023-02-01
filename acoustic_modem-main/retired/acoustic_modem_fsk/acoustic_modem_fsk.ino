/*Performing FSK modulation utilizing a DAC on a teensy 3.2
 * Author: Scott Mayberry
 */
#include <stdint.h>

#define MESSAGE_MAXIMUM_SYMBOL_SIZE 16384
#define BAUD_RATE 1000
#define RELAY_SWITCH_PIN 11
#define demod_pin 12

//////Shared Variables///////
bool hammingActive = true; //hamming (12, 7)
const float maxFrequency = 29126.2136;
const float minFrequency = 27027.027;
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

float volume = 0.5;

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
const int messageTimeOutThreshold = 8;

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
    Serial.begin(115200); //begin Serial communication
    while (!Serial)
    {
        ;
    }
    if (ARM_DWT_CYCCNT == ARM_DWT_CYCCNT)
    {
        // Enable CPU Cycle Count
        ARM_DEMCR |= ARM_DEMCR_TRCENA;
        ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
    }
    analogWriteResolution(10);
    pinMode(RELAY_SWITCH_PIN, OUTPUT);
    pinMode(A14, OUTPUT);
    analogWrite(A14, 0);
    updateRelay(false);
    outgoingSineWaveDACTimer.priority(100);
    outgoingSymbolUpdateTimer.priority(99);
    multipleWaveFrequencyTimer.priority(99);
    uSECONDS_PER_SYMBOL = 1 / (float(BAUD_RATE)) * 1000000;
    //outgoingSineWaveDACTimer.begin(timer0_callback, 1.25);
    updateVolume();
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
    Serial.println("Message Complete");
    endDemodulationWithoutMessage();
    modulationActive = false;
}

void startModulation()
{
    noInterrupts();
    modulationActive = true;
    detachDemodInterrupt();
    updateRelay(true);
    delay(100);
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

void addParityBitsToDACandPeriod(int tempBits[])
{
    int parityBits[5];
    parityBits[0] = tempBits[6] ^ tempBits[5] ^ tempBits[3] ^ tempBits[2] ^ tempBits[0];
    parityBits[1] = tempBits[6] ^ tempBits[4] ^ tempBits[3] ^ tempBits[1] ^ tempBits[0];
    parityBits[2] = tempBits[5] ^ tempBits[4] ^ tempBits[3];
    parityBits[3] = tempBits[2] ^ tempBits[1] ^ tempBits[0];
    parityBits[4] = tempBits[0] ^ tempBits[1] ^ tempBits[2] ^ tempBits[3] ^ tempBits[4] ^ tempBits[5] ^ tempBits[6] ^ parityBits[0] ^ parityBits[1] ^ parityBits[2] ^ parityBits[3];
    for (int i = 0; i < 5; i++)
    {
        Serial.print(parityBits[i]);
        saveBitToSymbolOut(outgoingMessagesBytesIndex * 12 + 7 + i, parityBits[i]);
    }
    Serial.print(" ");
}

void saveBitToSymbolOut(int index, int tempBit)
{
    symbolValueOfMessage[index] = tempBit;
}

void readIncomingSerialDataForModulation()
{
    if (Serial.available())
    {
        interruptDemodulationWithModulation();
        if (outgoingMessagesBytesIndex == 0)
        {
            outgoingMessagesBytes[outgoingMessagesBytesIndex] = 'w';
        }
        else
        {
            outgoingMessagesBytes[outgoingMessagesBytesIndex] = Serial.read();
        }

        //Serial.print(char(outgoingMessagesBytes[outgoingMessagesBytesIndex]));
        if (hammingActive)
        {
            int tempBits[7];
            for (int i = 6; i >= 0; i--) //convert new byte into bits and then convert bits into half frequency times for interval timer
            {
                tempBits[(6 - i)] = (outgoingMessagesBytes[outgoingMessagesBytesIndex] >> i) & 1;
                Serial.print(tempBits[(6 - i)]);
                saveBitToSymbolOut(outgoingMessagesBytesIndex * 12 + (6 - i), tempBits[(6 - i)]);
            }
            addParityBitsToDACandPeriod(tempBits);
            messageSymbolSize += 12;
        }
        else
        {
            for (int i = 7; i >= 0; i--) //convert new byte into bits and then convert bits into half frequency times for interval timer
            {
                int tempBit = (outgoingMessagesBytes[outgoingMessagesBytesIndex] >> i) & 1;
                Serial.print(tempBit);
                saveBitToSymbolOut(outgoingMessagesBytesIndex * 8 + (7 - i), tempBit);
            }
            Serial.print(" ");
            messageSymbolSize += 8;
        }
        if (outgoingMessagesBytes[outgoingMessagesBytesIndex] == '\n') //if character is a new line, end of message
        {
            Serial.println();
            Serial.println("Starting Message Modulation");
            startModulation();
        }
        outgoingMessagesBytesIndex++;
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
    // if (inSymbolIndex % 12 == 0)
    // {
    //     Serial.print(" ");
    // }
    // Serial.print(symbol);
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
    if (averageMeasuredTimeBetweenPings < maxFrequencyMicroSeconds - 5 || averageMeasuredTimeBetweenPings > minFrequencyMicroSeconds + 5)
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

void correctForShiftedMessage()
{
    if (incomingSymbols[3] == 0)
    {
        return;
    }
    //Serial.println("had to shift message");
    int pos = 0;
    while (incomingSymbols[pos] != 0)
    {
        pos++;
    }
    int correctAmount = abs(pos - 3);
    if (pos - 3 > 0)
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
        if (i % 12 == 0 && i != 0)
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
        pos = inSymbolIndex / 12;
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

void hammingCodeErrorCorrection()
{
    int parityBitChecks[5];
    for (int i = 0; i < inSymbolIndex; i += 12)
    {
        parityBitChecks[0] = (incomingSymbols[i + 6] ^ incomingSymbols[i + 5] ^ incomingSymbols[i + 3] ^ incomingSymbols[i + 2] ^ incomingSymbols[i + 0]) ^ incomingSymbols[i + 7];
        parityBitChecks[1] = (incomingSymbols[i + 6] ^ incomingSymbols[i + 4] ^ incomingSymbols[i + 3] ^ incomingSymbols[i + 1] ^ incomingSymbols[i + 0]) ^ incomingSymbols[i + 8];
        parityBitChecks[2] = (incomingSymbols[i + 5] ^ incomingSymbols[i + 4] ^ incomingSymbols[i + 3]) ^ incomingSymbols[i + 9];
        parityBitChecks[3] = (incomingSymbols[i + 2] ^ incomingSymbols[i + 1] ^ incomingSymbols[i + 0]) ^ incomingSymbols[i + 10];
        parityBitChecks[4] = (incomingSymbols[i + 0] ^ incomingSymbols[i + 1] ^ incomingSymbols[i + 2] ^ incomingSymbols[i + 3] ^ incomingSymbols[i + 4] ^ incomingSymbols[i + 5] ^ incomingSymbols[i + 6] ^ incomingSymbols[i + 7] ^ incomingSymbols[i + 8] ^ incomingSymbols[i + 9] ^ incomingSymbols[i + 10]) ^ incomingSymbols[i + 11];
        if (parityBitChecks[0] + parityBitChecks[1] + parityBitChecks[2] + parityBitChecks[3] > 0)
        { //if there is an error detected, then this if statement catches it
            if (parityBitChecks[4] == 0)
            {
                ; //Possible double bit error
            }
            else
            {
                int errorIndex = errorDetectionIndex(parityBitChecks[0] * 16 + parityBitChecks[1] * 8 + parityBitChecks[2] * 4 + parityBitChecks[3] * 2 + parityBitChecks[4]);
                if (incomingSymbols[i + errorIndex] == 0)
                {
                    incomingSymbols[i + errorIndex] = 1;
                }
                else
                {
                    incomingSymbols[i + errorIndex] = 0;
                }
            }
        }
    }
}

int errorDetectionIndex(int errorValue)
{
    switch (errorValue)
    {
    case 27:
        return 0;
    case 11:
        return 1;
    case 19:
        return 2;
    case 29:
        return 3;
    case 13:
        return 4;
    case 21:
        return 5;
    case 25:
        return 6;
    case 17:
        return 7;
    case 9:
        return 8;
    case 5:
        return 9;
    case 3:
        return 10;
    case 1:
        return 11;
    default:
        return -1;
    }
}

void convertSymbolsIntoMessageBytes()
{
    if (hammingActive)
    {
        for (int i = 0; i < inSymbolIndex; i += 12)
        {
            incomingMessageBytes[i / 12] = convertSymbolsToByteAtIndexWithHamming(i);
        }
        hammingCodeErrorCorrection();
        for (int i = 0; i < inSymbolIndex; i += 12)
        {
            incomingMessageBytes[i / 12] = convertSymbolsToByteAtIndexWithHamming(i);
        }
    }
    else
    {
        for (int i = 0; i < inSymbolIndex; i += 8)
        {
            incomingMessageBytes[i / 8] = convertSymbolsToByteAtIndex(i);
        }
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

char convertSymbolsToByteAtIndexWithHamming(int incomingSymbolsTempIndex)
{
    int byteAtIndex = (incomingSymbols[incomingSymbolsTempIndex + 6]) + (incomingSymbols[incomingSymbolsTempIndex + 5] * 2) +
                      (incomingSymbols[incomingSymbolsTempIndex + 4] * 4) + (incomingSymbols[incomingSymbolsTempIndex + 3] * 8) +
                      (incomingSymbols[incomingSymbolsTempIndex + 2] * 16) + (incomingSymbols[incomingSymbolsTempIndex + 1] * 32) +
                      (incomingSymbols[incomingSymbolsTempIndex + 0] * 64);
    return char(byteAtIndex);
}