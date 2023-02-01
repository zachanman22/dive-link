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
#ifndef ACOMM3_h
#define ACOMM3_h

// include types & constants of Wiring core API
#include "Arduino.h"
#include "Goertzel.h"

// library interface description
class ACOMM3
{
  // user-accessible "public" interface
public:
  ACOMM3(int, int);
  ACOMM3(int);

  bool isMessageInProgress();
  void setMessageInProgress(bool);

  int getNumSymbols();
  int getSampleIndex();
  float getTimerUpdateSpeedMicros();
  void setStartMessageThreshold(float);
  void setEndMessageThreshold(float);
  void addSample(int);
  bool symbolReadyToProcess();
  byte getSymbol();
  bool isSymbolEndSymbol(byte);
  float getUSecondsPerSymbol();
  float getFreqMicroSecondsFromSymbol(byte symbol);
  int getNumBitsPerSymbol();
  void setBaud(int);
  int getBaud();
  bool hasMessageStarted();

  static constexpr float sampling_frequency = 100000;

  static const int endMessageValue = 255;
  static constexpr float defaultStartMessageThreshold = 1.0;
  static constexpr float defaultEndMessageThreshold = 0.001;

  static constexpr float minFrequencyBound = 22000;                                       // min frequency
  static constexpr float maxFrequencyBound = 38000;                                       // max frequency
  static const int hzStepsPerBin = 25;                                                    // hz distance between each bin
  static const int numFreqBins = (maxFrequencyBound - minFrequencyBound) / hzStepsPerBin; // number of total frequency bins
  static const int plusMinusBins = 2;                                                     // how many nearby bins to look at for frequency
  // library-accessible "private" interface
private:
  void initialize(int, int);
  void resetMeanTracker();
  void setFrequencyHzStepsFromBaud();
  void addSampleForActiveMessage(float);
  void addSampleForMessageStartDetection(float);
  float normalizePurityToTransducer(float, float);
  bool checkForEndOfMessage();
  byte getSymbolFromWeightedFrequency(float);

  Goertzel g[numFreqBins];
  float freq[numFreqBins];

  float purity[numFreqBins];
  float timerUpdateSpeedMicros;

  int symbolrate;
  int stepsBetweenBins;
  int sampleNumber;

  float startMessageThreshold;
  float endMessageThreshold;

  float symbolFreq[16]; // input frequencies, these values are mapped to 0, 1, 2,..., n symbol values
  int symbolFreqIndexInG[16];
  float symbolFreqMicroSeconds[16];
  int numSymbols;

  int N;

  bool messageInProgress;

  int hzStepsForGoertzel = 500;
  int ADCCENTER = 128;

  static const int meanTrackerSize = 3;
  int meanTrackerIndex = 0;
  float meanTracker[meanTrackerSize];

  static const int slidingDetectionWindowSize = 20;
  int sdwindowIndex = 0;
  float sdwindow[slidingDetectionWindowSize];
};

#endif
