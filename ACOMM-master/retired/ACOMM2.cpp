// include core Wiring API
#include "Arduino.h"

// include this library's description file
#include "ACOMM2.h"

ACOMM2::ACOMM2(int in_symbolrate, float in_SAMPLING_FREQUENCY, int ADC_CENTER)
{
  initialize(in_symbolrate, in_SAMPLING_FREQUENCY, ADC_CENTER);
}

ACOMM2::ACOMM2(int in_symbolrate, float in_SAMPLING_FREQUENCY)
{
  initialize(in_symbolrate, in_SAMPLING_FREQUENCY, 128);
}

void ACOMM2::initialize(int in_symbolrate, float in_SAMPLING_FREQUENCY, int ADC_CENTER)
{
  // sampling_frequency = in_SAMPLING_FREQUENCY;
  timerUpdateSpeedMicros = 1 / float(sampling_frequency) * 1000000;

  // set variables from inputs
  setBaud(in_symbolrate);
  setFrequencyHzStepsFromBaud();

  // setup defaults
  startMessageThreshold = defaultStartMessageThreshold;
  endMessageThreshold = defaultEndMessageThreshold;
  bufferStartIndex = 0;

  // set up freq
  for (int i = 0; i < numFreqBins; i++)
  {
    freq[i] = minFrequencyBound + hzStepsForGoertzel * i;
  }

  // set up freqMicroSeconds
  for (int i = 0; i < numFreqBins; i++)
  {
    freqMicroSeconds[i] = 1 / freq[i] * 1000000;
  }

  // set up Goertzel
  for (int i = 0; i < numFreqBins; i++)
  {
    g[i].reinit(freq[i], sampling_frequency, ADC_CENTER);
  }
  starti = int((lowerFrequency - minFrequencyBound) / hzStepsForGoertzel) - 4;
  endi = int((upperFrequency - minFrequencyBound) / hzStepsForGoertzel) + 4;
  resetPreviousAndDerivativePurity();
  clearStartBuffer();
  Serial.println("acomm");
}

void ACOMM2::setFrequencyHzStepsFromBaud()
{
  hzStepsForGoertzel = sampling_frequency / symbolrate;
}

float ACOMM2::getTimerUpdateSpeedMicros()
{
  return timerUpdateSpeedMicros;
}

void ACOMM2::clearStartBuffer()
{
  for (int i = 0; i < bufferMaxSize; i++)
  {
    bufferStartValues[i] = 0;
  }
}

float ACOMM2::getStartBufferAverage()
{
  float average = 0;
  for (int i = 0; i < bufferMaxSize; i++)
  {
    average += bufferStartValues[i];
  }
  // Serial.print("buffer average");
  // Serial.println(average / bufferMaxSize);
  return average / bufferMaxSize;
}

void ACOMM2::addToStartBuffer(float in)
{
  if (bufferStartIndex >= bufferMaxSize)
  {
    bufferStartIndex = 0;
  }
  bufferStartValues[bufferStartIndex++] = in;
}

void ACOMM2::setStartMessageThreshold(float newThreshold)
{
  startMessageThreshold = newThreshold;
}

void ACOMM2::setEndMessageThreshold(float newThreshold)
{
  endMessageThreshold = newThreshold;
}

void ACOMM2::addSample(int sample)
{
  Serial.print(micros());
  Serial.print(" ");
  Serial.println(sample);
  return;
  for (int i = starti; i <= endi; i++)
  {
    g[i].addSample(sample);
  }
}

void ACOMM2::calcPurity()
{
  for (int i = starti; i <= endi; i++)
  {
    purity[i] = g[i].detect();
  }
}

void ACOMM2::addMaxPurityToBuffer()
{
  float maxPurity = purity[starti];
  for (int i = starti + 1; i <= endi; i++)
  {
    if (maxPurity < purity[i])
    {
      maxPurity = purity[i];
    }
  }
  addToStartBuffer(maxPurity);
}

void ACOMM2::addVarianceSquaredToBuffer()
{
  float totalPurity = 0;
  for (int i = starti; i <= endi; i++)
  {
    if (purity[i] > 0)
    {
      totalPurity += purity[i];
    }
  }
  float meanPurity = totalPurity / (endi + 1 - starti);
  float varianceSquared = 0;
  for (int i = starti; i <= endi; i++)
  {
    varianceSquared += (purity[i] - meanPurity) * (purity[i] - meanPurity);
  }
  varianceSquared /= totalPurity;
  // Serial.print("variance squared:");
  // Serial.println(varianceSquared);
  addToStartBuffer(varianceSquared);
}

bool ACOMM2::getBufferAboveThreshold()
{
  int counter = 0;
  for (int i = 0; i < bufferMaxSize; i++)
  {
    if (bufferStartValues[i] > startMessageThreshold)
    {
      counter++;
    }
  }
  if (counter >= bufferMaxSize - 3)
  {
    return true;
  }
  return false;
}

void ACOMM2::addMeanPurityToBuffer()
{
  float totalPurity = 0;
  for (int i = starti; i <= endi; i++)
  {
    if (purity[i] > 0)
    {
      totalPurity += purity[i];
    }
  }
  float meanPurity = totalPurity / (endi + 1 - starti);
  addToStartBuffer(meanPurity);
}

bool ACOMM2::detectStartOfMessageAboveThreshold()
{
  // the message must start with the last symbol
  // example: 1111 for 16MARY or 11 for 4MARY
  if (getSampleIndex() < fastSampleNumber)
  {
    return false;
  }
  calcPurity();
  addVarianceSquaredToBuffer();
  // if (getStartBufferAverage() >= startMessageThreshold)
  if (getBufferAboveThreshold())
  {
    clearStartBuffer();
    return true;
  }
  return false;
}

bool ACOMM2::symbolReadyToProcess()
{
  if (getSampleIndex() >= sampleNumber)
  {
    return true;
  }
  return false;
}

int ACOMM2::getSampleIndex()
{
  Serial.println("this function does not work");
  return 0;
}

void ACOMM2::normalizePurityToTransducer()
{
  for (int i = starti; i <= endi; i++)
  {
    if (i < (starti + endi + 1) / 2)
    {
      purity[i] /= 3.3;
    }
    else
    {
      purity[i] /= 1.8;
    }
  }
}

byte ACOMM2::getSymbolAsByte(byte previousSymbol)
{
  // get the purity of the signal for the specified frequencies
  calcPurity();

  // calculate the symbol as the position with the largest purity
  // return endMessageValue if end message threshold met

  int threshold = 1000;
  float derivativeGain = 0.01;

  // get the mean from the purity measurements
  float totalMag = 0;
  float mean = 0;
  for (int i = starti; i <= endi; i++)
    totalMag += purity[i];
  mean = totalMag / (endi + 1 - starti);

  normalizePurityToTransducer();
  // calc the variancesquared
  // threshold the purity measurements
  // recenter new readings from threshold
  // calculate derivatives of new reading
  float varianceSquared = 0;
  for (int i = starti; i <= endi; i++)
  {
    derPurity[i] = (purity[i] - prevPurity[i]) * getBaud();
    varianceSquared += (purity[i] - mean) * (purity[i] - mean);
    prevPurity[i] = purity[i];
    if (purity[i] < 0)
    {
      purity[i] = 0;
    }
  }
  varianceSquared /= totalMag;

  totalMag = 0;
  float weightedFrequency = 0;
  float adjPurity;
  // Serial.println("Frequency");
  for (int i = starti; i <= endi; i++)
  {
    adjPurity = purity[i] + derivativeGain * derPurity[i];
    if (adjPurity < threshold)
    {
      adjPurity = 0;
    }
    weightedFrequency += freq[i] * adjPurity;
    totalMag += adjPurity;
    // Serial.print(freq[i]);
    // Serial.print(",");
    // Serial.print(purity[i]);
    // Serial.print(",");
    // Serial.println(adjPurity);
  }

  // check if the message either wasnt thresholded
  if (totalMag > 0)
  {
    weightedFrequency /= totalMag;
  }
  else
  {
    weightedFrequency = (1 - previousSymbol) * lowerFrequency + previousSymbol * upperFrequency;
  }

  // Serial.print("Weighted Frequency:");
  // Serial.println(weightedFrequency);
  // Serial.print("variance Squared:");
  // Serial.println(varianceSquared);
  // Serial.println(";");

  // if message purity is less than the threshold, return 255, else return symbolbyte
  // if (currentMessageMaxPurity <= endMessageThreshold)
  if (varianceSquared <= endMessageThreshold)
  {
    Serial.println();
    resetPreviousAndDerivativePurity();
    return endMessageValue;
  }
  if (weightedFrequency <= (upperFrequency + lowerFrequency) / 2)
    return 0;
  return 1;
}

// byte ACOMM2::getSymbolAsByte(byte previousSymbol)
// {
//   // get the purity of the signal for the specified frequencies
//   calcPurity();

//   // calculate the symbol as the position with the largest purity
//   // return endMessageValue if end message threshold met
//   float totalMag = 0;
//   float weightedFrequency = 0;
//   int freqIndex = starti;
//   float meanPurityWithoutZeros = 0;
//   float maximumPurity = 0;
//   float mean = 0;
//   float varianceSquared = 0;

//   // get the mean from the purity measurements
//   for (int i = starti; i <= endi; i++)
//     totalMag += purity[i];
//   mean = totalMag / (endi + 1 - starti);

//   // subtract the mean from the measurements
//   // and subtract the prevPurity
//   // and save the new prevPurity
//   for (int i = starti; i <= endi; i++)
//   {
//     varianceSquared += (purity[i] - mean) * (purity[i] - mean);
//     purity[i] = (purity[i] - mean);
//     if (purity[i] < 0)
//     {
//       purity[i] = 0;
//     }
//     float tempPure = purity[i];
//     purity[i] -= prevPurity[i];
//     prevPurity[i] = tempPure;
//   }
//   varianceSquared /= totalMag;

//   totalMag = 0;
//   Serial.println("Frequency");
//   for (int i = starti; i <= endi; i++)
//   {
//     // if ((previousSymbol == 0 && i >= (starti + endi) / 2) || (previousSymbol == 1 && i <= (starti + endi) / 2))
//     // {
//     //   purity[i] *= 2;
//     // }
//     if (purity[i] < 5000)
//     {
//       float abspurity = abs(purity[i]);
//       weightedFrequency += freq[endi + 1 - i] * abspurity;
//       totalMag += abspurity;
//       Serial.print(freq[i]);
//       // Serial.print("Purity:");
//       Serial.print(",");
//       Serial.println(-abspurity);
//     }
//     else if (purity[i] > 5000)
//     {
//       weightedFrequency += freq[i] * purity[i];
//       totalMag += purity[i];
//       if (purity[i] > maximumPurity)
//       {
//         maximumPurity = purity[i];
//         freqIndex = i;
//       }
//       // Serial.print("Frequency:");
//       Serial.print(freq[i]);
//       // Serial.print("Purity:");
//       Serial.print(",");
//       Serial.println(purity[i]);
//     }
//     else
//     {
//       Serial.print(freq[i]);
//       // Serial.print("Purity:");
//       Serial.print(",");
//       Serial.println(0);
//     }

//     // weightedFrequency += freq[i] * purity[i];
//   }
//   if (totalMag <= 10)
//   {
//     if (previousSymbol == 0)
//     {
//       weightedFrequency = lowerFrequency;
//     }
//     else
//     {
//       weightedFrequency = upperFrequency;
//     }
//   }
//   else
//   {
//     weightedFrequency /= totalMag;
//   }

//   // meanPurityWithoutZeros /= (endi + 1 - starti);
//   Serial.print("Weighted Frequency:");
//   Serial.println(weightedFrequency);
//   Serial.print("variance Squared:");
//   Serial.println(varianceSquared);
//   // Serial.print("Frequency: ");
//   // Serial.print(freq[freqIndex]);
//   // Serial.print("Purity: ");
//   // Serial.println(maximumPurity);
//   // Serial.print("Mean Purity:");
//   // Serial.println(meanPurityWithoutZeros);
//   Serial.println(";");

//   // if message purity is less than the threshold, return 255, else return symbolbyte
//   // if (currentMessageMaxPurity <= endMessageThreshold)
//   if (varianceSquared <= endMessageThreshold)
//   {
//     resetPreviousAndDerivativePurity();
//     return endMessageValue;
//   }
//   // if (freq[freqIndex] <= (upperFrequency + lowerFrequency) / 2)
//   //   return 0;
//   // return 1;

//   if (weightedFrequency <= (upperFrequency + lowerFrequency) / 2)
//     return 0;
//   return 1;
// }

void ACOMM2::resetPreviousAndDerivativePurity()
{
  for (int i = starti; i < endi; i++)
  {
    prevPurity[i] = 0;
    derPurity[i] = 0;
  }
}

void ACOMM2::copyPurityToPreviousPurity()
{
  for (int i = starti; i <= endi; i++)
  {
    prevPurity[i] = purity[i];
  }
}

void ACOMM2::resetBitsFromSymbol()
{
  // reset bits in bitsFromSYmbol to 0
  for (int i = 0; i < numBits; i++)
  {
    bitsFromSymbol[i] = 0;
  }
}

byte *ACOMM2::convertSymbolAsByteToBitArray(byte symbolByte)
{
  // take the incoming symbolAsByte to array of bits for modulation and demod
  resetBitsFromSymbol();
  for (int i = 0; i < numBits; i++)
  {
    bitsFromSymbol[i] = ((symbolByte >> i) & 0x01);
  }
  return bitsFromSymbol;
}

byte ACOMM2::convertSymbolAsBitArrayToByte(byte symbolBits[])
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

float ACOMM2::getUSecondsPerSymbol()
{
  return 1 / float(symbolrate) * 1000000;
}

float ACOMM2::getRXFreqMicroSecondsFromSymbol(byte symbol)
{
  if (symbol == 0)
    return freqMicroSeconds[int((lowerFrequency - minFrequencyBound) / hzStepsForGoertzel)];
  return freqMicroSeconds[int((upperFrequency - minFrequencyBound) / hzStepsForGoertzel)];
}

int ACOMM2::getNumBits()
{
  return 1;
}

void ACOMM2::setBaud(int baud)
{
  symbolrate = baud;
  sampleNumber = int(sampling_frequency / symbolrate);
  fastSampleNumber = sampleNumber / fastSampleDivider;
}

int ACOMM2::getBaud()
{
  return symbolrate;
}
