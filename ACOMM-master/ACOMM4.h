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
#ifndef ACOMM4_h
#define ACOMM4_h

// include types & constants of Wiring core API
#include "Arduino.h"
#include "Goertzel.h"

// library interface description
class ACOMM4
{
  // user-accessible "public" interface
public:
  ACOMM4(int, int);
  ACOMM4(int);

  bool isMessageInProgress();
  void setMessageInProgress(bool);

  int getNumSymbols();
  int getSampleIndex();
  float getTimerUpdateSpeedMicros();
  void setStartMessageThreshold(float);
  void setEndMessageThreshold(float);
  void addSample(int, int);
  bool symbolReadyToProcess();
  byte getSymbol();
  bool isSymbolEndSymbol(byte);
  float getUSecondsPerSymbol();
  float getFreqMicroSecondsFromSymbol(byte symbol);
  int getNumBitsPerSymbol();
  void setBaud(int);
  int getBaud();
  bool hasMessageStarted(int);
  bool isMessageReady();
  void printMessages();

  static constexpr float sampling_frequency = 200000;

  static const int endMessageValue = 255;

  static const int maximumSymbolsIn = 12000; // 16384;
  static const int numberOfGThreads = 10;

  static constexpr float defaultEndMessageThreshold = 500;

  static constexpr float minFrequencyBound = 22000;                                       // min frequency
  static constexpr float maxFrequencyBound = 38000;                                       // max frequency
  static const int hzStepsPerBin = 25;                                                    // hz distance between each bin
  static const int numFreqBins = (maxFrequencyBound - minFrequencyBound) / hzStepsPerBin; // number of total frequency bins
  static const int plusMinusBins = 1;                                                     // how many nearby bins to look at for frequency
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
  void checkForAllMessagesInactive();
  void resetCounters();
  void addSampleForActiveMessageSingle(float, int);
  byte getSymbolSingle(int);
  bool checkForEndOfMessageSingle(int);
  bool symbolReadyToProcessSingle(int);
  void setPurityDivider();
  void setNewEndMessageThreshold();

  Goertzel g[numberOfGThreads][numFreqBins];
  float freq[numFreqBins];

  int messageNumber;

  float purityDivider;

  byte symbolsIn[numberOfGThreads][maximumSymbolsIn];
  int symbolsInIndex[numberOfGThreads];
  int messagesActive[numberOfGThreads]; // 0-readyToStart,1-active,2-waitingForOtherMessagesToEnd
                                        //{0.0075, 0.01, 0.04, 0.1, 0.18, 0.25, 0.42, 0.5, 0.75, 1.0, 1.1};
                                        // {0.01, 0.02, 0.04, 0.1, 0.18, 0.25, 0.37, 0.42, 0.5, 0.75, 1.0, 1.1, 1.2};
                                        // {0.04, 0.08, 0.1, 0.14, 0.18, 0.25, 0.37, 0.42, 0.5, 0.75, 1.0, 1.1, 1.2};
  const int startMessageThresholds[numberOfGThreads] = {40, 75, 120, 200, 300, 400, 500, 600, 700, 800};
  //{25, 29, 33, 38, 41, 50};
  float purity[numFreqBins];
  float timerUpdateSpeedMicros;

  int symbolrate;
  int stepsBetweenBins;
  int sampleNumber[numberOfGThreads];

  float endMessageThreshold;

  float symbolFreq[16]; // input frequencies, these values are mapped to 0, 1, 2,..., n symbol values
  int symbolFreqIndexInG[16];
  float symbolFreqMicroSeconds[16];
  int numSymbols;

  int N;

  bool messageInProgress;

  bool messageReadyToRead;

  int ADCCENTER = 2048;

  static const int meanTrackerSize = 3;
  int meanTrackerIndex[numberOfGThreads];
  float meanTracker[numberOfGThreads][meanTrackerSize];

  static const int slidingDetectionWindowSize = 50;
  int sdwindowIndex = 0;
  float sdwindow[slidingDetectionWindowSize];
};

#endif
