// include core Wiring API
#include "Arduino.h"

// include this library's description file
#include "ACOMM3.h"

ACOMM3::ACOMM3(int in_symbolrate, int ADC_CENTER)
{
  initialize(in_symbolrate, ADC_CENTER);
}

ACOMM3::ACOMM3(int in_symbolrate)
{
  initialize(in_symbolrate, 128);
}

void ACOMM3::initialize(int in_symbolrate, int ADC_CENTER)
{
  // sampling_frequency = in_SAMPLING_FREQUENCY;
  timerUpdateSpeedMicros = 1 / float(sampling_frequency) * 1000000;
  symbolrate = in_symbolrate;
  ADCCENTER = ADC_CENTER;
  N = int(sampling_frequency / symbolrate);

  setFrequencyHzStepsFromBaud();

  numSymbols = 2;
  symbolFreq[0] = 25000;
  symbolFreq[1] = 32000;
  // set up freqMicroSeconds
  for (int i = 0; i < numSymbols; i++)
  {
    symbolFreqMicroSeconds[i] = 1 / symbolFreq[i] * 1000000;
    symbolFreqIndexInG[i] = int((symbolFreq[i] - minFrequencyBound) / hzStepsPerBin);
  }
  // set variables from inputs

  // set up freq
  for (int i = 0; i < numFreqBins; i++)
  {
    freq[i] = minFrequencyBound + hzStepsPerBin * i;
    g[i].reinit(freq[i], sampling_frequency, N);
  }

  // setup defaults
  startMessageThreshold = defaultStartMessageThreshold;
  endMessageThreshold = defaultEndMessageThreshold;

  // set up Goertzel
  resetMeanTracker();
  sampleNumber = 0;
  messageInProgress = false;
  Serial.println("acomm");
}

void ACOMM3::resetMeanTracker()
{
  for (int i = 0; i < meanTrackerSize; i++)
  {
    meanTracker[i] = 1000;
  }
  meanTrackerIndex = 0;
}

void ACOMM3::setFrequencyHzStepsFromBaud()
{
  hzStepsForGoertzel = sampling_frequency / symbolrate;
  stepsBetweenBins = hzStepsForGoertzel / hzStepsPerBin;
}

float ACOMM3::getTimerUpdateSpeedMicros()
{
  return timerUpdateSpeedMicros;
}

void ACOMM3::setStartMessageThreshold(float newThreshold)
{
  startMessageThreshold = newThreshold;
}

void ACOMM3::setEndMessageThreshold(float newThreshold)
{
  endMessageThreshold = newThreshold;
}

void ACOMM3::addSampleForActiveMessage(float adjustedSample)
{
  for (int i = 0; i < numSymbols; i++)
  {
    for (int k = -plusMinusBins; k <= plusMinusBins; k++)
    {
      int tempBinIndex = symbolFreqIndexInG[i] + k * stepsBetweenBins;
      g[tempBinIndex].addSample(adjustedSample);
    }
  }
  sampleNumber++;
}

void ACOMM3::addSampleForMessageStartDetection(float adjustedSample)
{
  sdwindow[sdwindowIndex++] = adjustedSample;
  if (sdwindowIndex >= slidingDetectionWindowSize)
  {
    sdwindowIndex = 0;
  }
  if (sampleNumber < slidingDetectionWindowSize)
  {
    sampleNumber += 1;
  }
}

int ACOMM3::getSampleIndex()
{
  return sampleNumber;
}

void ACOMM3::setMessageInProgress(bool isInProgress)
{
  sampleNumber = 0;
  sdwindowIndex = 0;
  messageInProgress = isInProgress;
}

bool ACOMM3::isMessageInProgress()
{
  return messageInProgress;
}

void ACOMM3::addSample(int sample)
{
  float adjustedSample = float(sample - ADCCENTER) / (ADCCENTER);
  // Serial.println(adjustedSample);
  if (isMessageInProgress())
  {
    addSampleForActiveMessage(adjustedSample);
  }
  else
  {
    addSampleForMessageStartDetection(adjustedSample);
  }
}

bool ACOMM3::hasMessageStarted()
{
  if (sampleNumber >= slidingDetectionWindowSize)
  {
    // long newmicros = micros();
    // Serial.print("Frequency: ");
    // Serial.println(freq[symbolFreqIndexInG[numSymbols - 1]]);
    float purity = g[symbolFreqIndexInG[numSymbols - 1]].detectBatch(sdwindow, slidingDetectionWindowSize, sdwindowIndex);
    // Serial.print("Has message started:");
    // Serial.println(micros() - newmicros);
    // Serial.print("Purity: ");
    // Serial.println(purity);
    if (purity >= startMessageThreshold)
      return true;
  }
  return false;
}

bool ACOMM3::symbolReadyToProcess()
{
  if (sampleNumber >= N)
  {
    return true;
  }
  return false;
}

float ACOMM3::normalizePurityToTransducer(float frequency, float purity)
{
  if (frequency > 28000)
  {
    return purity / 7.0;
  }
  return purity;
}

bool ACOMM3::isSymbolEndSymbol(byte symbolToCheck)
{
  if (symbolToCheck == endMessageValue)
  {
    return true;
  }
  return false;
}

byte ACOMM3::getSymbol()
{
  sampleNumber = 0;
  // calc the variancesquared
  // recenter new readings from threshold
  float weightedFrequency = 0;
  float totalPurity = 0;
  for (int i = 0; i < numSymbols; i++)
  {
    for (int k = -plusMinusBins; k <= plusMinusBins; k++)
    {
      int tempBinIndex = symbolFreqIndexInG[i] + k * stepsBetweenBins;
      float purity = normalizePurityToTransducer(freq[tempBinIndex], g[tempBinIndex].detect());
      // Serial.println(purity);
      // Serial.println(tempBinIndex);
      // Serial.println(freq[tempBinIndex]);
      totalPurity += purity;
      weightedFrequency += freq[tempBinIndex] * purity;
    }
  }
  float mean = totalPurity / (numSymbols + plusMinusBins * 2 + 1);
  meanTracker[meanTrackerIndex] = mean;
  meanTrackerIndex = (meanTrackerIndex + 1) % meanTrackerSize;
  weightedFrequency /= totalPurity;
  // Serial.print("Weighted Frequency:");
  // Serial.println(weightedFrequency);
  // Serial.println(";");

  // if message purity is less than the threshold, return 255, else return symbolbyte
  if (checkForEndOfMessage())
  {
    resetMeanTracker();
    return endMessageValue;
  }
  return getSymbolFromWeightedFrequency(weightedFrequency);
}

bool ACOMM3::checkForEndOfMessage()
{
  for (int i = 0; i < meanTrackerSize; i++)
  {
    if (meanTracker[i] > endMessageThreshold)
    {
      return false;
    }
  }
  return true;
}

byte ACOMM3::getSymbolFromWeightedFrequency(float weightedFrequency)
{
  int indexOfFrequency = 0;
  int minFreqDist = abs(int(weightedFrequency) - int(symbolFreq[0]));
  for (int i = 1; i < numSymbols; i++)
  {
    int newFreqDist = abs(int(weightedFrequency) - int(symbolFreq[i]));
    if (newFreqDist < minFreqDist)
    {
      minFreqDist = newFreqDist;
      indexOfFrequency = i;
    }
  }
  return indexOfFrequency;
}

float ACOMM3::getUSecondsPerSymbol()
{
  return 1 / float(symbolrate) * 1000000;
}

float ACOMM3::getFreqMicroSecondsFromSymbol(byte symbol)
{
  return symbolFreqMicroSeconds[symbol];
}

int ACOMM3::getNumSymbols()
{
  return numSymbols;
}

int ACOMM3::getNumBitsPerSymbol()
{
  int numberOfBits = 0;
  int tempNumSymbols = numSymbols;
  while (tempNumSymbols >= 2)
  {
    numberOfBits++;
    tempNumSymbols /= 2;
  }
  return numberOfBits;
}

void ACOMM3::setBaud(int baud)
{
  if (baud == 10 || baud == 100 || baud == 250 || baud == 500 ||
      baud == 1000 || baud == 1250 || baud == 2000)
  {
    symbolrate = baud;
    N = int(sampling_frequency / baud);
    for (int i = 0; i < numFreqBins; i++)
      g[i].reinit(freq[i], sampling_frequency, N);
  }
  else
  {
    Serial.println("incorrect baud");
  }
}

int ACOMM3::getBaud()
{
  return symbolrate;
}
