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
#ifndef ACOMM_h
#define ACOMM_h

// include types & constants of Wiring core API
#include "Arduino.h"
#include "Goertzel.h"

// library interface description
class ACOMM
{
  // user-accessible "public" interface
public:
  ACOMM(int, float, int);
  ACOMM(int, float);

  int getNumChannels();
  int getCurrentChannel();
  int setMary(int);
  bool setChannel(int);
  void addSample(int);

  bool detectStartOfMessageAboveThreshold();
  byte getSymbolAsByte();
  byte *convertSymbolAsByteToBitArray(byte symbolByte);
  byte convertSymbolAsBitArrayToByte(byte symbolBytes[]);

  float getRXFreqMicroSecondsFromSymbol(byte symbol);
  float getTXFreqMicroSecondsFromSymbol(byte symbol);

  float getRXFreqFromSymbol(byte symbol);
  float getTXFreqFromSymbol(byte symbol);

  bool setRX(int chan);
  bool setTX(int chan);

  int getTX();
  int getRX();

  void setBaud(int);
  int getBaud();

  void setStartMessageThreshold(float);
  void setEndMessageThreshold(float);

  bool symbolReadyToProcess();
  int getSampleIndex();

  int getMary();
  int getNumBits();
  float getTimerUpdateSpeedMicros();

  float getUSecondsPerSymbol();

  static const int endMessageValue = 255;
  static constexpr float defaultStartMessageThreshold = 0.4;
  static constexpr float defaultEndMessageThreshold = 0.01;
  static const int frequencySteps = 8000;
  static const int numFreq = 16;

  // library-accessible "private" interface
private:
  void initialize(int, float, int);
  void clearStartBuffer();
  float getStartBufferAverage();
  void addToStartBuffer(float in);

  void addMaxPurityInMARYToBuffer();

  void calcPurity();
  void resetBitsFromSymbol();

  Goertzel g[numFreq];
  float freq[numFreq];
  float freqMicroSeconds[numFreq];
  float purity[numFreq];
  float timerUpdateSpeedMicros;

  int freqIndex;
  int mary;
  int numBits;

  int tx;

  int symbolrate;
  int sampleNumber;
  int fastSampleNumber;

  int sampling_frequency;

  float startMessageThreshold;
  float endMessageThreshold;

  byte bitsFromSymbol[int(sqrt(numFreq)) + 1];

  static const int fastSampleDivider = 4;
  static const int bufferMaxSize = fastSampleDivider - 1;
  float bufferStartValues[bufferMaxSize];
  int bufferStartIndex;
};

#endif
