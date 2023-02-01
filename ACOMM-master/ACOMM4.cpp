// include core Wiring API
#include "Arduino.h"

// include this library's description file
#include "ACOMM4.h"

ACOMM4::ACOMM4(int in_symbolrate, int ADC_CENTER)
{
  initialize(in_symbolrate, ADC_CENTER);
}

ACOMM4::ACOMM4(int in_symbolrate)
{
  initialize(in_symbolrate, 2048);
}

void ACOMM4::initialize(int in_symbolrate, int ADC_CENTER)
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
    Serial.print("Symbol Freq Index in G: ");
    Serial.println(symbolFreqIndexInG[i]);
  }
  // set variables from inputs

  // set up freq
  for (int i = 0; i < numFreqBins; i++)
    freq[i] = minFrequencyBound + hzStepsPerBin * i;

  for (int k = 0; k < numberOfGThreads; k++)
    for (int i = 0; i < numFreqBins; i++)
    {
      g[k][i].reinit(freq[i], sampling_frequency, N);
      // Serial.print("Freq: ");
      // Serial.println(g[k][i].getTargetFreq());
    }

  for (int i = 0; i < numberOfGThreads; i++)
  {
    messagesActive[i] = 0;
    symbolsInIndex[i] = 0;
  }
  endMessageThreshold = defaultEndMessageThreshold;

  // set up Goertzel
  resetMeanTracker();
  messageReadyToRead = false;
  messageNumber = 0;
  setPurityDivider();
  setNewEndMessageThreshold();
  Serial.println("acomm");
}

void ACOMM4::setNewEndMessageThreshold()
{
  switch (symbolrate)
  {
  case 100:
    // endMessageThreshold = 0.01;
    endMessageThreshold = 0.2;
    break;

  default:
    endMessageThreshold = defaultEndMessageThreshold;
    break;
  }
}

void ACOMM4::setPurityDivider()
{
  switch (symbolrate)
  {
  case 100:
    purityDivider = 1.2;
    break;

  default:
    purityDivider = 1;
    break;
  }
}

void ACOMM4::resetMeanTracker()
{
  for (int k = 0; k < numberOfGThreads; k++)
  {
    for (int i = 0; i < meanTrackerSize; i++)
      meanTracker[k][i] = 1000;
    meanTrackerIndex[k] = 0;
  }
}

void ACOMM4::setFrequencyHzStepsFromBaud()
{
  // the bin width of the Goertzel is samplerate/N, which happens to equal symbolrate
  int realHzBinWidth = symbolrate;
  stepsBetweenBins = realHzBinWidth / hzStepsPerBin;
}

float ACOMM4::getTimerUpdateSpeedMicros()
{
  return timerUpdateSpeedMicros;
}

void ACOMM4::setStartMessageThreshold(float newThreshold)
{
  Serial.println("Start message doesnt work");
  // startMessageThreshold = newThreshold;
}

void ACOMM4::setEndMessageThreshold(float newThreshold)
{
  endMessageThreshold = newThreshold;
}

void ACOMM4::resetCounters()
{
  for (int k = 0; k < numberOfGThreads; k++)
  {
    sampleNumber[k] = 0;
    symbolsInIndex[k] = 0;
    messagesActive[k] = 0;
  }
  resetMeanTracker();
  sdwindowIndex = 0;
  messageReadyToRead = false;
  messageNumber++;
}

void ACOMM4::addSampleForActiveMessageSingle(float adjustedSample, int k)
{
  // Serial.println("1:addSampleForActiveMessageSingle");
  for (int i = 0; i < numSymbols; i++)
  {
    for (int j = -plusMinusBins; j <= plusMinusBins; j++)
    {

      int tempBinIndex = symbolFreqIndexInG[i] + j * stepsBetweenBins;
      // Serial.println(tempBinIndex);
      g[k][tempBinIndex].addSample(adjustedSample);
    }
  }
  // Serial.println("3:addSampleForActiveMessageSingle");
  sampleNumber[k] += 1;
}

void ACOMM4::addSampleForActiveMessage(float adjustedSample)
{
  Serial.println("DNW: Add sample for active message");
}

void ACOMM4::addSampleForMessageStartDetection(float adjustedSample)
{
  // Serial.println("1:addSampleForMessageStartDetection");
  sdwindow[sdwindowIndex++] = adjustedSample;
  if (sdwindowIndex >= slidingDetectionWindowSize)
  {
    sdwindowIndex = 0;
  }
  // Serial.println("2:addSampleForMessageStartDetection");
  for (int k = 0; k < numberOfGThreads; k++)
  {
    if (messagesActive[k] == 0)
      if (sampleNumber[k] < slidingDetectionWindowSize)
        sampleNumber[k] += 1;
  }
  // Serial.println("3:addSampleForMessageStartDetection");
}

int ACOMM4::getSampleIndex()
{
  Serial.println("DNW: get sample index");
  return sampleNumber[0];
}

void ACOMM4::setMessageInProgress(bool isInProgress)
{
  Serial.println("DNW: set message in progress");
}

bool ACOMM4::isMessageInProgress()
{
  return messageInProgress;
}

void ACOMM4::addSample(int sample, int sampleNumberCounter)
{
  // Serial.println(sample);
  float adjustedSample = float(sample - ADCCENTER) / (ADCCENTER);
  // Serial.println(adjustedSample);
  // delay(1);
  for (int k = 0; k < numberOfGThreads; k++)
  {
    if (messagesActive[k] == 1) // 1 is message is active
    {
      // Serial.print("k:");
      // Serial.println(k);
      addSampleForActiveMessageSingle(adjustedSample, k);
      if (symbolReadyToProcessSingle(k))
        getSymbolSingle(k);
    }
  }
  addSampleForMessageStartDetection(adjustedSample);
  hasMessageStarted(sampleNumberCounter);
}

bool ACOMM4::hasMessageStarted(int sampleNumberCounter)
{
  float purity = 0;
  for (int k = 0; k < numberOfGThreads; k++)
  {
    if (messagesActive[k] == 0 && sampleNumber[k] >= slidingDetectionWindowSize)
    {
      purity = g[k][symbolFreqIndexInG[numSymbols - 1]].detectBatch(sdwindow, slidingDetectionWindowSize, sdwindowIndex) * 10000;
      // Serial.println(purity);
      break;
    }
  }

  if (purity > startMessageThresholds[0])
  {
    // Serial.print("Above threshold: ");
    // Serial.println(purity);
    for (int k = 0; k < numberOfGThreads; k++)
      if (messagesActive[k] == 0 && purity >= startMessageThresholds[k])
      {
        Serial.print("Message Started: ");
        Serial.print(k);
        Serial.print("  Sample Number: ");
        Serial.println(sampleNumberCounter);
        messagesActive[k] = 1;
        // Serial.println("started");
      }
  }
  return true;
}

bool ACOMM4::symbolReadyToProcessSingle(int k)
{
  if (sampleNumber[k] >= N)
  {
    return true;
  }
  return false;
}

bool ACOMM4::symbolReadyToProcess()
{
  Serial.println("DNW: symbol ready to process");
  if (sampleNumber[0] >= N)
  {
    return true;
  }
  return false;
}

float ACOMM4::normalizePurityToTransducer(float frequency, float purity)
{
  if (frequency > 30000)
  {
    return purity * purityDivider;
  }
  return purity;
}

bool ACOMM4::isSymbolEndSymbol(byte symbolToCheck)
{
  if (symbolToCheck == endMessageValue)
  {
    return true;
  }
  return false;
}

byte ACOMM4::getSymbolSingle(int k)
{
  sampleNumber[k] = 0;
  // recenter new readings from threshold
  float weightedFrequency = 0;
  float totalPurity = 0;
  for (int i = 0; i < numSymbols; i++)
  {
    for (int j = -plusMinusBins; j <= plusMinusBins; j++)
    {
      int tempBinIndex = symbolFreqIndexInG[i] + j * stepsBetweenBins;
      float purity = normalizePurityToTransducer(freq[tempBinIndex], g[k][tempBinIndex].detect() * 10000);
      // Serial.print("k: ");
      // Serial.print(k);
      // Serial.print(" Freq: ");
      // Serial.print(freq[tempBinIndex]);
      // Serial.print(" Purity: ");
      // Serial.println(purity);
      totalPurity += purity;
      weightedFrequency += freq[tempBinIndex] * purity;
    }
  }
  float mean = totalPurity / (numSymbols + (plusMinusBins * 2 + 1));
  meanTracker[k][meanTrackerIndex[k]] = mean;
  meanTrackerIndex[k] = (meanTrackerIndex[k] + 1) % meanTrackerSize;
  weightedFrequency /= totalPurity;
  // Serial.print("Weighted Frequency:");
  // Serial.println(weightedFrequency);
  // Serial.println(";");

  // if message purity is less than the threshold, return 255, else return symbolbyte
  if (checkForEndOfMessageSingle(k))
  {
    // Serial.print("k ended: ");
    // Serial.println(k);
    messagesActive[k] = 2;
    checkForAllMessagesInactive();
    return endMessageValue;
  }
  symbolsIn[k][symbolsInIndex[k]] = getSymbolFromWeightedFrequency(weightedFrequency);
  symbolsInIndex[k] = symbolsInIndex[k] + 1;
  return 0;
}

bool ACOMM4::isMessageReady()
{
  return messageReadyToRead;
}

void ACOMM4::printMessages()
{
  Serial.print(messageNumber);
  for (int i = 0; i < numberOfGThreads; i++)
  {
    if (messagesActive[i] == 2)
    {
      Serial.println(" ");
      for (int j = 0; j < symbolsInIndex[i]; j++)
        Serial.print(symbolsIn[i][j]);
    }
  }
  Serial.println();
  resetCounters();
}

void ACOMM4::checkForAllMessagesInactive()
{
  for (int k = 0; k < numberOfGThreads; k++)
  {
    if (messagesActive[k] == 1)
    {
      messageReadyToRead = false;
      return;
    }
  }
  for (int k = 0; k < numberOfGThreads; k++)
  {
    if (messagesActive[k] == 0)
      messagesActive[k] = 3;
  }
  messageReadyToRead = true;
  // Serial.print("Message: ");
  // Serial.println(messageNumber);
}

byte ACOMM4::getSymbol()
{
  Serial.println("DNW: get symbol");
  return 0;
}

bool ACOMM4::checkForEndOfMessageSingle(int k)
{
  for (int i = 0; i < meanTrackerSize; i++)
  {
    if (meanTracker[k][i] > endMessageThreshold)
    {
      // Serial.println(meanTracker[k][i]);
      return false;
    }
  }
  return true;
}

bool ACOMM4::checkForEndOfMessage()
{
  Serial.println("DNW: check fo end of message");
  return true;
}

byte ACOMM4::getSymbolFromWeightedFrequency(float weightedFrequency)
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

float ACOMM4::getUSecondsPerSymbol()
{
  return 1 / float(symbolrate) * 1000000;
}

float ACOMM4::getFreqMicroSecondsFromSymbol(byte symbol)
{
  return symbolFreqMicroSeconds[symbol];
}

int ACOMM4::getNumSymbols()
{
  return numSymbols;
}

int ACOMM4::getNumBitsPerSymbol()
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

void ACOMM4::setBaud(int baud)
{
  if (baud == 10 || baud == 100 || baud == 250 || baud == 500 ||
      baud == 1000 || baud == 1250 || baud == 2000)
  {
    symbolrate = baud;
    N = int(sampling_frequency / baud);
    for (int k = 0; k < numberOfGThreads; k++)
      for (int i = 0; i < numFreqBins; i++)
        g[k][i].reinit(freq[i], sampling_frequency, N);
  }
  else
  {
    Serial.println("incorrect baud");
  }
}

int ACOMM4::getBaud()
{
  return symbolrate;
}
