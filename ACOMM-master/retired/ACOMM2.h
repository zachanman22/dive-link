/*
  The Goertzel algorithm is long standing so see
  http://en.wikipedia.org/wiki/Goertzel_algorithm for a full description.
  It is often used in DTMF tone detection as an alternative to the Fast
  Fourier Transform because it is quick with low overheard because it
  is only searching for a single frequency rather than showing the
  occurrence of all frequencies.

  This work is entirely based on the Kevin Banks code found at
  http://www.eetimes.com/design/embedded/4024443/The-Goertzel-Algorithm
  so full credit to him for his generic implementation and breakdown. I've
  simply massaged it into an Arduino library. I recommend reading his article
  for a full description of whats going on behind the scenes.

  See Contributors.md and add yourself for pull requests
  Released into the public domain.
*/

// ensure this library description is only included once
#ifndef ACOMM2_h
#define ACOMM2_h

// include types & constants of Wiring core API
#include "Arduino.h"
#include "Goertzel.h"

// library interface description
class ACOMM2
{
  // user-accessible "public" interface
public:
  ACOMM2(int, float, int);
  ACOMM2(int, float);

  void addSample(int);

  bool detectStartOfMessageAboveThreshold();
  byte getSymbolAsByte(byte);
  byte *convertSymbolAsByteToBitArray(byte symbolByte);
  byte convertSymbolAsBitArrayToByte(byte symbolBytes[]);

  float getRXFreqMicroSecondsFromSymbol(byte symbol);

  void setBaud(int);
  int getBaud();

  void setStartMessageThreshold(float);
  void setEndMessageThreshold(float);

  int getNumBits();

  bool symbolReadyToProcess();
  int getSampleIndex();

  float getTimerUpdateSpeedMicros();

  float getUSecondsPerSymbol();

  static const int sampling_frequency = 100000;

  static const int endMessageValue = 255;
  static constexpr float defaultStartMessageThreshold = 100000000;
  static constexpr float defaultEndMessageThreshold = 100;
  static const int frequencySteps = 2000;
  static constexpr float minFrequencyBound = 23000;
  static constexpr float maxFrequencyBound = 40000;
  static const int numFreqBins = 120;

  // library-accessible "private" interface
private:
  void initialize(int, float, int);
  void clearStartBuffer();
  float getStartBufferAverage();
  void addToStartBuffer(float in);
  void setFrequencyHzStepsFromBaud();

  void addMeanPurityToBuffer();

  void addMaxPurityToBuffer();

  void calcPurity();
  void resetBitsFromSymbol();
  void resetPreviousAndDerivativePurity();
  void copyPurityToPreviousPurity();
  void normalizePurityToTransducer();
  void addVarianceSquaredToBuffer();
  bool getBufferAboveThreshold();

  Goertzel g[numFreqBins];
  float freq[numFreqBins];
  float prevPurity[numFreqBins];
  float derPurity[numFreqBins];
  float freqMicroSeconds[numFreqBins];
  float purity[numFreqBins];
  float timerUpdateSpeedMicros;

  int tx;
  int numBits = 1;

  byte bitsFromSymbol[int(sqrt(numFreqBins)) + 1];

  int symbolrate;
  int sampleNumber;
  int fastSampleNumber;

  float startMessageThreshold;
  float endMessageThreshold;

  float lowerFrequency = 25000;
  float upperFrequency = 33000;

  int starti;
  int endi;

  int hzStepsForGoertzel = 500;

  static const int fastSampleDivider = 10;
  static const int bufferMaxSize = fastSampleDivider - 1;
  float bufferStartValues[bufferMaxSize];
  int bufferStartIndex;
};

#endif
