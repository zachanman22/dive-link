// include core Wiring API
#include "Arduino.h"

// include this library's description file
#include "ACOMM.h"

ACOMM::ACOMM(int in_symbolrate, float in_SAMPLING_FREQUENCY, int ADC_CENTER)
{
  initialize(in_symbolrate, in_SAMPLING_FREQUENCY, ADC_CENTER);
}

ACOMM::ACOMM(int in_symbolrate, float in_SAMPLING_FREQUENCY)
{
  initialize(in_symbolrate, in_SAMPLING_FREQUENCY, 128);
}

void ACOMM::initialize(int in_symbolrate, float in_SAMPLING_FREQUENCY, int ADC_CENTER)
{
  sampling_frequency = in_SAMPLING_FREQUENCY;
  timerUpdateSpeedMicros = 1 / in_SAMPLING_FREQUENCY * 1000000;

  // set variables from inputs
  setBaud(in_symbolrate);

  // calc variables from inputs
  numBits = 0;

  // setup defaults
  startMessageThreshold = defaultStartMessageThreshold;
  endMessageThreshold = defaultEndMessageThreshold;
  bufferStartIndex = 0;

  // set up freq
  for (int i = 0; i < numFreq; i++)
  {
    freq[i] = 25000 + frequencySteps * i;
  }

  // set up freqMicroSeconds
  for (int i = 0; i < numFreq; i++)
  {
    freqMicroSeconds[i] = 1 / freq[i] * 1000000;
  }

  // set up Goertzel
  for (int i = 0; i < numFreq; i++)
  {
    g[i].reinit(freq[i], sampling_frequency, ADC_CENTER);
  }

  // default set mary to BFSK
  setMary(2);

  // default set channel to 0
  setChannel(0);

  clearStartBuffer();
}

float ACOMM::getTimerUpdateSpeedMicros()
{
  return timerUpdateSpeedMicros;
}

void ACOMM::clearStartBuffer()
{
  for (int i = 0; i < bufferMaxSize; i++)
  {
    bufferStartValues[i] = 0;
  }
}

float ACOMM::getStartBufferAverage()
{
  float average = 0;
  for (int i = 0; i < bufferMaxSize; i++)
  {
    average += bufferStartValues[i];
  }
  return average / bufferMaxSize;
}

void ACOMM::addToStartBuffer(float in)
{
  if (bufferStartIndex >= bufferMaxSize)
  {
    bufferStartIndex = 0;
  }
  bufferStartValues[bufferStartIndex++] = in;
}

void ACOMM::setStartMessageThreshold(float newThreshold)
{
  startMessageThreshold = newThreshold;
}

void ACOMM::setEndMessageThreshold(float newThreshold)
{
  endMessageThreshold = newThreshold;
}

int ACOMM::getNumChannels()
{
  return numFreq / mary;
}

int ACOMM::getCurrentChannel()
{
  return freqIndex / mary;
}

int ACOMM::setMary(int m)
{
  numBits = 0;
  while (m != 1)
  {
    m /= 2;
    numBits += 1;
  }
  mary = int(pow(2, numBits));
  // Serial.print("Mary: ");
  // Serial.println(mary);
  return mary;
}

int ACOMM::getMary()
{
  return mary;
}

int ACOMM::getNumBits()
{
  return numBits;
}

bool ACOMM::setChannel(int newChannel)
{
  if (newChannel >= 0 && newChannel < numFreq / mary)
  {
    tx = -1;
    freqIndex = mary * newChannel;
    return true;
  }
  return false;
}

void ACOMM::addSample(int sample)
{
  for (int i = 0; i < mary; i++)
  {
    g[i + freqIndex].addSample(sample);
  }
}

void ACOMM::calcPurity()
{
  for (int i = 0; i < mary; i++)
  {
    purity[i + freqIndex] = g[i + freqIndex].detect();
    // Serial.println(purity[i + freqIndex]);
  }
  // Serial.println(";");
}

void ACOMM::addMaxPurityInMARYToBuffer()
{
  float maxPurity = purity[freqIndex];
  for (int i = 1; i < mary; i++)
  {
    if (maxPurity < purity[freqIndex + i])
    {
      maxPurity = purity[freqIndex + i];
    }
  }
  addToStartBuffer(maxPurity);
}

bool ACOMM::detectStartOfMessageAboveThreshold()
{
  // the message must start with the last symbol
  // example: 1111 for 16MARY or 11 for 4MARY
  if (getSampleIndex() < fastSampleNumber)
  {
    return false;
  }
  calcPurity();
  addMaxPurityInMARYToBuffer();
  // if (getStartBufferAverage() >= startMessageThreshold)
  if (getStartBufferAverage() >= 200)
  {
    clearStartBuffer();
    return true;
  }
  return false;
}

bool ACOMM::symbolReadyToProcess()
{
  if (getSampleIndex() >= sampleNumber)
  {
    return true;
  }
  return false;
}

int ACOMM::getSampleIndex()
{
  Serial.println("This function does not work");
  return 0;
  // return g[freqIndex].getSampleIndex();
}

byte ACOMM::getSymbolAsByte()
{
  // get the purity of the signal for the specified frequencies
  calcPurity();
  byte symbolByte = 0;

  // create var to store sum of purity measurements, then take the average
  float currentMessageMaxPurity = purity[freqIndex];
  // calculate the symbol as the position with the largest purity
  // return endMessageValue if end message threshold met
  for (int i = 1; i < mary; i++)
  {
    if (purity[i + freqIndex] > purity[i + freqIndex - 1])
    {
      symbolByte = i;
    }
    currentMessageMaxPurity = max(currentMessageMaxPurity, purity[i + freqIndex]);
  }
  // if message purity is less than the threshold, return 255, else return symbolbyte
  // if (currentMessageMaxPurity <= endMessageThreshold)
  if (currentMessageMaxPurity <= 200)
  {
    return endMessageValue;
  }
  return symbolByte;
}

void ACOMM::resetBitsFromSymbol()
{
  // reset bits in bitsFromSYmbol to 0
  for (int i = 0; i < numBits; i++)
  {
    bitsFromSymbol[i] = 0;
  }
}

byte *ACOMM::convertSymbolAsByteToBitArray(byte symbolByte)
{
  // take the incoming symbolAsByte to array of bits for modulation and demod
  resetBitsFromSymbol();
  for (int i = 0; i < numBits; i++)
  {
    bitsFromSymbol[i] = ((symbolByte >> i) & 0x01);
  }
  return bitsFromSymbol;
}

byte ACOMM::convertSymbolAsBitArrayToByte(byte symbolBits[])
{
  int multiplier = 1;
  byte tempSymbol = 0;
  for (int i = 0; i < numBits; i++)
  {
    // Serial.println(symbolBits[i]);
    tempSymbol += multiplier * symbolBits[i];
    multiplier *= 2;
  }
  return tempSymbol;
}

float ACOMM::getRXFreqMicroSecondsFromSymbol(byte symbol)
{
  return freqMicroSeconds[freqIndex + symbol];
}

float ACOMM::getTXFreqMicroSecondsFromSymbol(byte symbol)
{
  if (tx == -1)
  {
    return getRXFreqMicroSecondsFromSymbol(symbol);
  }
  return freqMicroSeconds[mary * tx + symbol];
}

float ACOMM::getRXFreqFromSymbol(byte symbol)
{
  return freq[freqIndex + symbol];
}

float ACOMM::getTXFreqFromSymbol(byte symbol)
{
  if (tx == -1)
  {
    return getRXFreqFromSymbol(symbol);
  }
  return freq[mary * tx + symbol];
}

float ACOMM::getUSecondsPerSymbol()
{
  return 1 / float(symbolrate) * 1000000;
}

bool ACOMM::setRX(int chan)
{
  int tempTx = tx;
  bool success = setChannel(chan);
  if (success)
  {
    tx = tempTx;
    return true;
  }
  return false;
}

bool ACOMM::setTX(int chan)
{
  if (chan >= 0 && chan < numFreq / mary)
  {
    tx = chan;
    return true;
  }
  return false;
}

int ACOMM::getTX()
{
  return tx;
}

int ACOMM::getRX()
{
  return getCurrentChannel();
}

void ACOMM::setBaud(int baud)
{
  symbolrate = baud;
  sampleNumber = int(sampling_frequency / symbolrate);
  fastSampleNumber = sampleNumber / fastSampleDivider;
}

int ACOMM::getBaud()
{
  return symbolrate;
}
