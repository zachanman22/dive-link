#include <stdint.h>

#define MESSAGE_MAXIMUM_SYMBOL_SIZE 8192
#define BAUD_RATE 3000
#define mod_pin 7
#define demod_pin 12

//////Shared Variables///////
bool hammingActive = true; //hamming (12, 7)
const float maxFrequency = 29126.2136;
const float minFrequency = 27027.027;
const float frequencyMicroSeconds = 1.0 / maxFrequency * 1000000;
const float secondaryFrequencyMicroSeconds = 1.0 / minFrequency * 1000000;

//////Demodulation Variables////////

volatile bool incomingSignalDetected = false;

char incomingMessageBytes[MESSAGE_MAXIMUM_SYMBOL_SIZE / 8 + 1];

volatile int incomingSymbols[MESSAGE_MAXIMUM_SYMBOL_SIZE];
volatile int inSymbolIndex;

volatile int messageTimeOutCounter;
const int messageTimeOutThreshold = 8;

const int noiseCounterThreshold = 4;
volatile int incomingNoiseCounter = 0;
volatile long lastMessageIncomingTimer = 0;

float lowSmallAverage;
float highSmallAverage;
float midAverage;
float uSECONDS_PER_SYMBOL = 1 / (float(BAUD_RATE)) * 1000000;
volatile uint32_t pingsCPUCycle[1000];
volatile int pingsCPUCycleIndex;

IntervalTimer multipleWaveFrequencyTimer;
IntervalTimer pingSpeedTimer;

void setup()
{
  Serial.begin(115200); //begin Serial communication
  while (!Serial)
  {
    ;
  }
  //enabled CPU Cycle count. This is used to get accurate measurements of the incoming frequency
  if (ARM_DWT_CYCCNT == ARM_DWT_CYCCNT)
  {
    // Enable CPU Cycle Count
    ARM_DEMCR |= ARM_DEMCR_TRCENA;
    ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
  }
  uSECONDS_PER_SYMBOL = 1 / (float(BAUD_RATE)) * 1000000;
  Serial.println("Starting up");
  Serial.println("Demodulation");

  //attach intterupt to begin checking for start condition
  attachStartConditionInterrupt();
}

void pingSpeedTimerCallback()
{
  //record the CPU cycle count at the time of incoming rising edge of incoming frequency
  pingsCPUCycle[pingsCPUCycleIndex++] = ARM_DWT_CYCCNT;
}

void startConditionMetCallback()
{
  //if this function is called and the incoming signal has not been detected
  //start the demodulation
  if (!incomingSignalDetected)
  {
    //start the multiplWaveFrequencyTimer. This timer is called every uSeconds_PER_SYMBOL,
    //This timer computes the frequency of each incoming period within the uSeconds_Per_Symbol.
    multipleWaveFrequencyTimer.begin(multipleWaveFrequencyTimerCallback, uSECONDS_PER_SYMBOL);

    //detachInterrupts on demodpin and attachRunning intterupt
    detachDemodInterrupt();
    attachRunningTimerInterrupt();

    //set demodFlag to true
    incomingNoiseCounter = 0;
    incomingSignalDetected = true;
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
  if (abs(microsDifference - frequencyMicroSeconds) >= abs(secondaryFrequencyMicroSeconds - microsDifference))
    // if (abs(secondaryFrequencyMicroSeconds - microsDifference) < 0.5)
    return 0;
  else
    return 1;
}

bool checkForEndedMessageCondition(int sumOfPings)
{
  //Checks to see if enough 0s or 1s have been received in a row to trigger a message end
  if (incomingSymbols[inSymbolIndex] == incomingSymbols[inSymbolIndex - 1])
  {
    //increment message time out counter. Once it is equal to or greater than messageTimeOutThreshold,
    //the incoming message has ended
    messageTimeOutCounter++;

    //if the symbolIndex is small, double count messageTimeOutCounter. This is to account for any noise that
    //spuriosly started the start conditions
    if (inSymbolIndex < 12)
    {
      messageTimeOutCounter++;
    }
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

void endDemodulationWithoutMessage()
{
  //end frequency timer to stop looking for incoming signal
  multipleWaveFrequencyTimer.end();
  //reset all tracking variables
  pingsCPUCycleIndex = 0;
  messageTimeOutCounter = 0;
  highSmallAverage = 0;
  lowSmallAverage = 0;
  midAverage = 0;
  inSymbolIndex = 0;
  //attach intterupt timer to check for signal start conditions.
  attachStartConditionInterrupt();
  //reset demoulation flag
  incomingSignalDetected = false;
}

void endDemodulation()
{
  //end frequency timer to stop looking for incoming signal
  multipleWaveFrequencyTimer.end();
  //align incoming bits with lead char. This corrects for if the first
  //bit was missed or double counted
  correctForShiftedMessage();
  //take symbols read in, and convert into bytes
  convertSymbolsIntoMessageBytes();
  //publish the data to the Serial port
  publishIncomingData();

  //reset all tracking variables
  pingsCPUCycleIndex = 0;
  messageTimeOutCounter = 0;
  inSymbolIndex = 0;
  highSmallAverage = 0;
  lowSmallAverage = 0;
  midAverage = 0;
  //attach intterupt timer to check for signal start conditions.
  attachStartConditionInterrupt();
  //reset demoulation flag
  incomingSignalDetected = false;
}

void shiftTimerForwardSmall(int numOfIndexesForShift)
{
  // The incoming signal is slightly desynced from the expected uSecondsPerPeriod
  // The onboard frequency is slightly BEHIND the incoming wave frequency
  // This function resyncs the the timer to the incoming frequency

  //end current frequency timer (will be reset at end of function)
  multipleWaveFrequencyTimer.end();
  //delay for half the number of steps. Using half prevents overcorrection.
  Serial.print("FSmall");
  delayMicroseconds(int((float(numOfIndexesForShift / 2) / (pingsCPUCycleIndex - 1)) * uSECONDS_PER_SYMBOL));
  //restart frequency timer
  multipleWaveFrequencyTimer.begin(multipleWaveFrequencyTimerCallback, uSECONDS_PER_SYMBOL);
}

void shiftTimerForwardLarge(int numOfIndexesForShift)
{
  // The incoming signal is slightly desynced from the expected uSecondsPerPeriod
  // The onboard frequency is slightly AHEAD of the incoming wave frequency
  // This function takes the highSmallAverage as the next reading, and and resynces the timer to the incoming frequency

  //end current frequency timer (will be reset at end of function)
  multipleWaveFrequencyTimer.end();
  //save the high small average value as that is the next value.
  addSymbolToIncomingSymbols(convertMicrosDifferenceToNearestSymbol(highSmallAverage));
  inSymbolIndex++;
  //delay for a portion of the uSeconds_PER_Symbol
  Serial.print("FLarge");
  delayMicroseconds(int(uSECONDS_PER_SYMBOL * ((pingsCPUCycleIndex - 1) - (numOfIndexesForShift / 2)) / (float(pingsCPUCycleIndex - 1))));
  //restart frequency timer
  multipleWaveFrequencyTimer.begin(multipleWaveFrequencyTimerCallback, uSECONDS_PER_SYMBOL);
}

bool checkForTimerBeingTooSlow(int midAverageSymbol, int numOfIndexesForShift)
{
  //check if the middle of the measured signal has a different frequency average than the lower end
  //if it does, shift the timer forward a small amount to catch up with the incoming signal
  if (midAverageSymbol != convertMicrosDifferenceToNearestSymbol(lowSmallAverage))
  {
    shiftTimerForwardSmall(numOfIndexesForShift);
    return true;
  }
  return false;
}

bool checkForTimerBeingTooFast(int midAverageSymbol, int numOfIndexesForShift)
{
  //check if the middle of the measured signal has a different frequency average than the lower end
  //if it does, shift the timer forward a large amount while saving the high small average to slow down to the incoming signal
  if (midAverageSymbol != convertMicrosDifferenceToNearestSymbol(highSmallAverage))
  {
    shiftTimerForwardLarge(numOfIndexesForShift);
    return true;
  }
  return false;
}

void addSymbolToIncomingSymbols(int symbol)
{
  incomingSymbols[inSymbolIndex] = symbol;
  // if (inSymbolIndex % 12 == 0)
  // {
  //   Serial.print(" ");
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
  detachDemodInterrupt();
  //numOfIndexesForShift is a check to calculate how to break the incoming frequency into 3 categories: lowSmallAverage, midAverage, highSmallAverage
  //this is used to correct for timing desync
  int numOfIndexesForShift = int((pingsCPUCycleIndex - 1) / 2.5);
  float timeDiffMicro = 0;
  if (pingsCPUCycleIndex < int((uSECONDS_PER_SYMBOL / frequencyMicroSeconds)) / 1.5)
  {
    endDemodulation();
    return;
  }
  for (int i = 1; i < pingsCPUCycleIndex; i++)
  {
    //convert cpu cycle counts into micros
    timeDiffMicro = getTimeDifferenceMicrosFromSavedCPUPings(pingsCPUCycle[i], pingsCPUCycle[i - 1]);
    //add the value to lowSmallAverage if index is less than numOfIndexesForShift
    if (i - 1 < numOfIndexesForShift)
    {
      lowSmallAverage += timeDiffMicro;
    }
    else
    {
      //add the value to highSmallAverage if index is greater than totalPingsIndex minus numOfIndexesForShift
      if (i >= pingsCPUCycleIndex - numOfIndexesForShift)
      {
        highSmallAverage += timeDiffMicro;
      }
    }
    midAverage += timeDiffMicro;
  }
  //convert total micros into averages
  lowSmallAverage /= numOfIndexesForShift;
  highSmallAverage /= numOfIndexesForShift;
  // midAverage /= (pingsCPUCycleIndex - 1) - numOfIndexesForShift * 2;
  midAverage /= (pingsCPUCycleIndex - 1);
  //convert the midaverage micros into the nearest symbol
  addSymbolToIncomingSymbols(convertMicrosDifferenceToNearestSymbol(midAverage));
  //check if the proper conditions have been met to end the incoming demodulation
  if (checkForEndedMessageCondition(pingsCPUCycleIndex - 1))
  {
    return;
  }
  //increment symbolIndex
  inSymbolIndex++;
  //shift timing if needed
  // if (!checkForTimerBeingTooFast(incomingSymbols[inSymbolIndex - 1], numOfIndexesForShift))
  // {
  //   checkForTimerBeingTooSlow(incomingSymbols[inSymbolIndex - 1], numOfIndexesForShift);
  // }
  //reset averages and cpu index
  highSmallAverage = 0;
  lowSmallAverage = 0;
  midAverage = 0;
  pingsCPUCycleIndex = 0;
  //reattach timer interrupt
  attachRunningTimerInterrupt();
}

void correctForShiftedMessage()
{
  if (incomingSymbols[3] == 0)
  {
    return;
  }
  Serial.println("had to shift message");
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

////////Shared Functions/////////
void loop()
{
}