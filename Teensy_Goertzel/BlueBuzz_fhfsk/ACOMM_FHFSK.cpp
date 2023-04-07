// include core Wiring API
#include "Arduino.h"
#include <ArduinoJson.h>

// include this library's description file
#include "ACOMM_FHFSK.h"

// constructor. Pass in the baud rate, and the sine_data_length which is the number of steps per period
ACOMM_FHFSK::ACOMM_FHFSK(int new_baud, int sine_data_length)
{
    // save the new baud, and calculate N steps for Goertzel processing
    baud = new_baud;
    N = int(sampling_frequency / baud);

    /*
        calculate b-c and f-g values for below...
        The fhfsk_data is structured as {a, b, c, d, e, f, g, h}
        a: 0 bit frequency
        b: 1/(a)*1000000
        c: b/SINE_DATA_LENGTH
        d: 0 bit volume (0-1.0)
        e-h: repeat for 1 bit
        Frequencies hop every bit between bit pairs

        also set up all the goertzel bins
        g[number of threads][number of fh channels][number of symbols][bins of goertzel]
        number of threads = the number of messages to solve for. This is to account for the difficulty in detecting the edge of the wave
        number of fh channels = how many fh channels there are, need goertzel for each one
        number of symbols = if doing BFSK, only 0 and 1, but MFSK could inclue 0 1 2 3.
        bins of goertzel = how many frequency bins near the target frequency should be included. This accounts for noise in the generating of the target wave
    */
    for (int i = 0; i < number_of_fh_pairs; i++)
    {
        for (int k = 0; k < number_of_symbols; k++)
        {
            fhfsk_data[i][1 + 4 * k] = 1 / (fhfsk_data[i][0 + 4 * k]) * 1000000;    // set microseconds per period
            fhfsk_data[i][2 + 4 * k] = fhfsk_data[i][1 + 4 * k] / sine_data_length; // set dac update time in microseconds
            for (int j = 0; j < numberOfGThreads; j++)
            {
                for (int h = 0; h < binsPerFrequency; h++)
                {
                    g[j][i][k][h].reinit(fhfsk_data[i][0 + 4 * k] + baud * (h - plusMinusBins), sampling_frequency, N);
                }
            }
        }
    }

    // set all messagesActive to 0
    // symbolsInIndex is the symbols detected per incoming message thread
    // fh_index is the current frequency hopping index for each incoming message thread
    for (int i = 0; i < numberOfGThreads; i++)
    {
        messagesActive[i] = 0;
        symbolsInIndex[i] = 0;
        fh_index[i] = 0;
    }

    // mean tracker is used to detect when a message has ended
    resetMeanTracker();

    // set flags to 0. MessageReadyToRead = flag for outer loop to call printMessages or addMessageToJSON
    messageReadyToRead = false;
    messageNumber = 0;

    // a function to change the end message threshold based on the baud rate
    setNewEndMessageThreshold();

    //
    Serial.println("ACOMM loaded");
    Serial.print("Baud");
    Serial.println(baud);
}

void ACOMM_FHFSK::setNewEndMessageThreshold()
{
    switch (baud)
    {
        // case 100:
        //     endMessageThreshold = 0.2;
        //     break;

    default:
        endMessageThreshold = defaultEndMessageThreshold;
        break;
    }
}

void ACOMM_FHFSK::resetMeanTracker()
{
    // set the meantracker variable to high value
    // set meantracker index to 0 for all threads
    for (int k = 0; k < numberOfGThreads; k++)
    {
        for (int i = 0; i < meanTrackerSize; i++)
            meanTracker[k][i] = 1000;
        meanTrackerIndex[k] = 0;
    }
}

float ACOMM_FHFSK::getSampleRateMicros()
{
    // return the sample rate frequency in microseconds per period
    return sampleRateMicros;
}

void ACOMM_FHFSK::resetCounters()
{
    // reset all pertinent values for next incoming message
    for (int k = 0; k < numberOfGThreads; k++)
    {
        sampleNumber[k] = 0;
        sampleNumberRecorder[k] = 0;
        symbolsInIndex[k] = 0;
        messagesActive[k] = 0;
        fh_index[k] = 0;
        for (int j = 0; j < number_of_fh_pairs; j++)
        {
            for (int i = 0; i < number_of_symbols; i++)
            {
                for (int b = 0; b < binsPerFrequency; b++)
                {
                    g[k][j][i][b].ResetGoertzel();
                }
            }
        }
    }
    resetMeanTracker();
    sdwindowIndex = 0;
    messageReadyToRead = false;
    messageNumber++;
    messageInProgress = false;
}

void ACOMM_FHFSK::addSampleForActiveMessage(float adjustedSample, int k)
{
    // adjusted sample = normalized sample
    // k = message number
    // loop through the given active message and and add the new sample to the Goertzel processing.
    for (int i = 0; i < number_of_symbols; i++)
    {
        for (int j = 0; j < binsPerFrequency; j++)
        {
            // Serial.println(tempBinIndex);
            //Serial.println(adjustedSample);
            g[k][fh_index[k]][i][j].addSample(adjustedSample);
            //Serial.println(g[k][fh_index[k]][i][j].detect());
        }
    }
    // increase sample number for this message
    // used to check when the Goertzel needs to be processed
    sampleNumber[k] += 1;
}

void ACOMM_FHFSK::addSampleForMessageStartDetection(float adjustedSample)
{
    // goal is to detect the start of the message
    // adjusted sample is the normalized sample

    // sdwindow is a sliding window, adding a sample and
    // dropping a sample every time there is a new sample
    // Goertzel is then processed on the new window.
    sdwindow[sdwindowIndex++] = adjustedSample;
    // keep index from overflowing
    if (sdwindowIndex >= slidingDetectionWindowSize)
    {
        sdwindowIndex = 0;
    }
    //  k = messageNumber.
    //  loop through every inactive message and
    //  add sampleNumber if the number isnt large enough
    for (int k = 0; k < numberOfGThreads; k++)
    {
        if (messagesActive[k] == 0)
            if (sampleNumber[k] < slidingDetectionWindowSize)
                sampleNumber[k] += 1;
    }
    // Serial.println("3:addSampleForMessageStartDetection");
}

bool ACOMM_FHFSK::isMessageInProgress()
{
    // return if the messsage is active or complete/not started
    return messageInProgress;
}

void ACOMM_FHFSK::addSample(int sample, int sampleNumberCounter)
{
    // normalize sample between [-1,1]
    //Serial.println(sample);
    float adjustedSample = float(sample - ADCCENTER) / (ADCCENTER);
    // Serial.println(adjustedSample);

    // loop through active message threads and add the normalized sample
    //  computer next symbol if message is ready
    for (int k = 0; k < numberOfGThreads; k++)
    {
        if (messagesActive[k] == 1) // 1 is message is active
        {
            addSampleForActiveMessage(adjustedSample, k);
            if (symbolReadyToProcess(k))
                getSymbol(k);
        }
    }
    // add normalized sample to inactive messages
    addSampleForMessageStartDetection(adjustedSample);
    // check if any inactive message threads should be activated
    checkHasMessageStartedWithSetDelay(sampleNumberCounter);
}

void ACOMM_FHFSK::checkHasMessageStartedWithSetDelay(int sampleNumberCounter)
{
    // check if any inactive message threads should be set to active

    // loop through inactive messages and calculate purity
    // break out of loop, as purities will be equivalent for all inactive messages
    // There is only a need to check the largest symbol frequency,
    // as the message start requires that the first symbol be the largest symbol

    // Message threads are at first started by a "purity" threshold.
    // After a certain number of messages have been started, determined by "messageNumberToSwitchToDelay",
    // activation changes from requiring a "purity" threshold to starting a specific number of samples after the previous message.
    // This is done as the initial purity of the first few messages could be caused by noise, and also the purity calculated later rises
    // very fast, causing some inactive messages to start at the same time, wasting processing power.
    if (messagesActive[messageNumberToSwitchToDelay - 1] == 0)
    {
        float purity = 0;
        for (int k = 0; k < messageNumberToSwitchToDelay; k++)
        {
            if (messagesActive[k] == 0 && sampleNumber[k] >= slidingDetectionWindowSize)
            {
                // calculate purity for the first pair of Frequency Hopping with the largest symbol
                purity = g[k][0][0][plusMinusBins].detectBatch(sdwindow, slidingDetectionWindowSize, sdwindowIndex) * 10000;
                break;
            }
        }
        //Serial.println(purity);
        if (purity > startMessageThresholds[0])
        {
            for (int k = 0; k < messageNumberToSwitchToDelay; k++)
                if (messagesActive[k] == 0 && purity >= startMessageThresholds[k])
                {
                    Serial.printf("Message %d has started\n", k);
                    sampleNumberRecorder[k] = sampleNumberCounter;
                    messagesActive[k] = 1; // 1 is message active
                    latestActiveMessage = k;
                    return;
                }
        }
    }
    else
    {
        // start messages after a certain number of samples have come in, determined by "sampleDelay"
        if (latestActiveMessage < numberOfGThreads && sampleNumberCounter >= sampleNumberRecorder[latestActiveMessage - 1] + sampleDelay)
        {
            messageInProgress = true;
            sampleNumberRecorder[latestActiveMessage] = sampleNumberCounter;
            messagesActive[latestActiveMessage] = 1; // 1 is message active
            latestActiveMessage += 1;
            // Serial.println(latestActiveMessage - 1);
            // Serial.println(sampleNumberCounter);
            // Serial.println();
            return;
        }
    }
}

void ACOMM_FHFSK::checkHasMessageStarted(int sampleNumberCounter)
{
    // check if any new messages have started

    // loop through inactive messages and calculate purity
    // break out of loop, as purities will be equivalent for all unstarted messages
    // There is only a need to check the largest symbol frequency,
    // as the message start requires that the first symbol be the largest symbol
    float purity = 0;
    for (int k = 0; k < numberOfGThreads; k++)
    {
        if (messagesActive[k] == 0 && sampleNumber[k] >= slidingDetectionWindowSize)
        {
            purity = g[k][0][number_of_symbols - 1][plusMinusBins].detectBatch(sdwindow, slidingDetectionWindowSize, sdwindowIndex) * 10000;
            Serial.print("Message started: ");
            Serial.println(purity);
            break;
        }
    }

    // check if purity is higher than the minimum threshold
    // if so, start the next message, then break
    // breaking prevents starting two messages at the same time,
    // which would waste resources.
    if (purity > startMessageThresholds[0])
    {
        for (int k = 0; k < numberOfGThreads; k++)
            if (messagesActive[k] == 0 && purity >= startMessageThresholds[k])
            {
                Serial.print("Message started: ");
                Serial.println(k);
                sampleNumberRecorder[k] = sampleNumberCounter;
                messagesActive[k] = 1; // 1 is message active
                break;
            }
    }
}

bool ACOMM_FHFSK::symbolReadyToProcess(int k)
{
    // check if the current number of samples for a message are
    //  match the expected number of samples
    if (sampleNumber[k] >= N)
    {
        return true;
    }
    return false;
}

void ACOMM_FHFSK::update_fh_index(int k)
{
    // increment the frequency hopping index for
    //  the message number k
    fh_index[k]++;
    if (fh_index[k] >= number_of_fh_pairs)
    {
        fh_index[k] = 0;
    }
}

void ACOMM_FHFSK::getSymbol(int k)
{
    // goal is to calculate the symbol from the N number of samples added
    // k = the message number

    // reset sample number for next incoming symbol
    sampleNumber[k] = 0;
    // totalPurity is used to calculate whether the message has ended
    float totalPurity = 0;

    // loop through the frequency bins of the relative symbol frequencies and their adjacent bins
    // and calculate the purity of the signal.
    // sum the frequency bins for each seperate symbol
    for (int i = 0; i < number_of_symbols; i++)
    {
        purityPerSymbol[i] = 0;
        for (int j = 0; j < binsPerFrequency; j++)
        {
            purityPerSymbol[i] += g[k][fh_index[k]][i][j].detect() * 10000;
        }
        totalPurity += purityPerSymbol[i];
    }
    // calculate mean purity of the relavent bins
    //  if the mean is too low, this will end the active demodulation
    float mean = totalPurity / (number_of_symbols * (binsPerFrequency));
    // the mean tracker is a catch to prevent one bad received symbol from ending the message prematurely
    // the mean tracker requires that the mean of multiple symbols (size of meanTrackerSize) be below the threshold
    meanTracker[k][meanTrackerIndex[k]] = mean;
    meanTrackerIndex[k] = (meanTrackerIndex[k] + 1) % meanTrackerSize;

    // if message purity mean is less than the threshold, end demodulation for message
    if (checkForEndOfMessage(k))
    {
        messagesActive[k] = 2;                  // set message to complete
        symbolsIn[k][symbolsInIndex[k]] = '\0'; // add char ending charcter to symbols
        checkForAllMessagesInactive();          // check if all messages are now inactive, and ready for publishing
        return;
    }
    // set symbol as char representing the numerical symbol value
    symbolsIn[k][symbolsInIndex[k]] = char(getSymbolFromPurityPerSymbol() + 48);
    symbolsInIndex[k] = symbolsInIndex[k] + 1;
    // update the frequency hopping index for this message
    update_fh_index(k);
}

bool ACOMM_FHFSK::isMessageReady()
{
    return messageReadyToRead;
}

void ACOMM_FHFSK::addMessagesToJson(DynamicJsonDocument &doc)
{
    doc["m"] = messageNumber;
    // doc["1"] = symbolsIn[0];
    // JsonObject obj1 = doc.createNestedObject();
    JsonArray data = doc.createNestedArray("d");
    for (int i = 0; i < numberOfGThreads; i++)
    {
        Serial.print("Symbols in index: ");
        Serial.println(symbolsInIndex[i]);
        if (messagesActive[i] == 2)
        {
            data.add(symbolsIn[i]);
        }
    }
    resetCounters();
}

void ACOMM_FHFSK::printMessages()
{
    Serial.print(messageNumber);
    for (int i = 0; i < numberOfGThreads; i++)
    {
        if (messagesActive[i] == 2)
        {
            // Serial.println(" ");
            Serial.println(symbolsIn[i]);
            // for (int j = 0; j < symbolsInIndex[i]; j++)
            //     Serial.print(symbolsIn[i][j]);
        }
    }
    // Serial.println();
    resetCounters();
}

void ACOMM_FHFSK::checkForAllMessagesInactive()
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
}

bool ACOMM_FHFSK::checkForEndOfMessage(int k)
{
  if(symbolsInIndex[k] >= 256){
    return true;
  }
  return false;
//    for (int i = 0; i < meanTrackerSize; i++)
//    {
//      //Serial.println(meanTracker[k][i]);
//        if (meanTracker[k][i] > endMessageThreshold)
//        {
//            // Serial.println(meanTracker[k][i]);
//            return false;
//        }
//    }
//    return true;
}

byte ACOMM_FHFSK::getSymbolFromPurityPerSymbol()
{
  if(purityPerSymbol[0] > 300){
    return 1;
  }
  return 0;
//    byte symbol_in = 0;
//    float maxPurity = purityPerSymbol[0];
//    //Serial.printf("0 has purity %f\n", purityPerSymbol[0]);
//    //Serial.printf("1 has purity %f\n", purityPerSymbol[1]);
//    for (int i = 1; i < number_of_symbols; i++){
//        if (purityPerSymbol[i] > maxPurity)
//        {
//            maxPurity = purityPerSymbol[i];
//            symbol_in = i;
//        }
//    }
//    return symbol_in;
}

float ACOMM_FHFSK::getUSecondsPerSymbol()
{
    return 1 / float(baud) * 1000000;
}

float ACOMM_FHFSK::getFreqMicroSecondsFromSymbol(byte symbol)
{
    return symbolFreqMicroSeconds[symbol];
}

int ACOMM_FHFSK::get_number_of_symbols()
{
    return number_of_symbols;
}

int ACOMM_FHFSK::getNumBitsPerSymbol()
{
    int numberOfBits = 0;
    int tempnumber_of_symbols = number_of_symbols;
    while (tempnumber_of_symbols >= 2)
    {
        numberOfBits++;
        tempnumber_of_symbols /= 2;
    }
    return numberOfBits;
}

int ACOMM_FHFSK::getNumSymbols()
{
    return 2;
}

int ACOMM_FHFSK::getBaud()
{
    return baud;
}

// useful modulation functions
void ACOMM_FHFSK::setModulation(bool active)
{
    if (isModulationActive != active)
    {
        isModulationActive = !isModulationActive;
        resetCounters();
    }
}

bool ACOMM_FHFSK::getModulationActive()
{
    return isModulationActive;
}

float ACOMM_FHFSK::getDACMicrosUpdate(int symbol)
{
    return fhfsk_data[fh_index[0]][symbol * 4 + 2];
}
