// ensure this library description is only included once
#ifndef ACOMM_FHFSK_h
#define ACOMM_FHFSK_h

// include types & constants of Wiring core API
#include "Arduino.h"
#include "Goertzel.h"
#include <ArduinoJson.h>

// library interface description
class ACOMM_FHFSK
{
    // user-accessible "public" interface
public:
    ACOMM_FHFSK(int, int);

    bool isMessageInProgress();

    float getSampleRateMicros();
    void addSample(int, int);
    void getSymbol(int);
    float getUSecondsPerSymbol();
    float getFreqMicroSecondsFromSymbol(byte symbol);
    int getNumBitsPerSymbol();
    int getBaud();
    void checkHasMessageStarted(int);
    void checkHasMessageStartedWithSetDelay(int);
    bool isMessageReady();
    void printMessages();
    void addMessagesToJson(DynamicJsonDocument &);
    void setModulation(bool);
    bool getModulationActive();
    float getDACMicrosUpdate(int);
    int getNumSymbols();

    static constexpr float sampling_frequency = 250000;
    static constexpr float sampleRateMicros = 1 / float(sampling_frequency) * 1000000;

    static const int maximumSymbolsIn = 9000; // 16384;
    static const int numberOfGThreads = 6;
    static const int sampleDelay = 250;
    int latestActiveMessage = 0;
    static const int messageNumberToSwitchToDelay = 3;

    static constexpr float defaultEndMessageThreshold = 20;

    // The fhfsk_data is structured as {0_bit frequency, 1/(0_bit_frequency)*1000000, 1/(0_bit_frequency)*1000000/SINE_DATA_LENGTH, volume, ... repeat for 1_bit}
    /*
        The fhfsk_data is structured as {a, b, c, d, e, f, g, h}
        a: 0 bit frequency
        b: 1/(a)*1000000
        c: b/SINE_DATA_LENGTH
        d: 0 bit volume (0-1.0)
        e-h: repeat for 1 bit
        Frequencies hop every bit between bit pairs
    */
    static const int number_of_symbols = 2;
    static const int number_of_fh_pairs = 1;
    // should change to once frequency per line
    float fhfsk_data[number_of_fh_pairs][number_of_symbols * 4] = {{70000, 0, 0, 0.65, 120000, 0, 0, 0.45}};
    static const int plusMinusBins = 1; // how many nearby bins to look at for frequency
    static const int binsPerFrequency = plusMinusBins * 2 + 1;
    // library-accessible "private" interface
private:
    void resetMeanTracker();
    void addSampleForMessageStartDetection(float);
    bool checkForEndOfMessage(int);
    byte getSymbolFromPurityPerSymbol();
    void checkForAllMessagesInactive();
    void resetCounters();
    void addSampleForActiveMessage(float, int);
    int get_number_of_symbols();
    bool symbolReadyToProcess(int);
    void setNewEndMessageThreshold();
    void update_fh_index(int);

    Goertzel g[numberOfGThreads][number_of_fh_pairs][number_of_symbols][binsPerFrequency];
    int fh_index[numberOfGThreads];

    bool isModulationActive;

    int messageNumber;

    float purityPerSymbol[number_of_symbols];

    char symbolsIn[numberOfGThreads][maximumSymbolsIn];
    int symbolsInIndex[numberOfGThreads];
    int messagesActive[numberOfGThreads]; // 0-readyToStart,1-active,2-waitingForOtherMessagesToEnd
                                          //{40, 75, 120, 200, 300, 400, 500, 600, 700, 800};
    //{850, 950, 1050, 1150, 1250, 1350};
    const int startMessageThresholds[numberOfGThreads] = {10450, 11650, 12850, 13050, 14250, 15450}; //{4450, 5650, 6850, 7050, 8250, 9450}; //{100, 140, 180, 220, 250, 300, 350, 400, 450}; //{120, 200, 250, 300, 350, 400, 450, 500, 550, 600}; // 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 2000, 2500, 3000, 3500, 4000, 4500, 5000};

    int baud;
    int sampleNumber[numberOfGThreads];
    int sampleNumberRecorder[numberOfGThreads];

    float endMessageThreshold;

    float symbolFreq[16]; // input frequencies, these values are mapped to 0, 1, 2,..., n symbol values
    int symbolFreqIndexInG[16];
    float symbolFreqMicroSeconds[16];

    int N;

    bool messageInProgress;

    bool messageReadyToRead;

    const int ADCCENTER = 2048;

    static const int meanTrackerSize = 3;
    int meanTrackerIndex[numberOfGThreads];
    float meanTracker[numberOfGThreads][meanTrackerSize];

    static const int slidingDetectionWindowSize = 25;
    int sdwindowIndex = 0;
    float sdwindow[slidingDetectionWindowSize];
};

#endif
