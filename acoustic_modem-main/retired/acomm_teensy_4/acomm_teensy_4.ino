/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include <Hamming.h>
#include "Goertzel.h"
#include "ACOMM.h"
#include <ArduinoJson.h>
#include <SPI.h>
#include <MCP49xx.h>

#define MESSAGE_MAXIMUM_SYMBOL_SIZE 16384 / 2
#define RELAY_SWITCH_PIN 9
#define demod_pin 14
#define SS_PIN 10 //system select pin for SPI communication utilizing MCP4901 DAC

DynamicJsonDocument doc(2048);
DynamicJsonDocument responseDoc(256);
JsonObject root = doc.to<JsonObject>();

////////////////////////////////////////////////////////////////////
//Shared Variables
////////////////////////////////////////////////////////////////////
bool hammingActive = true; //hamming (12, 7)
Hamming hamming;
const char leadChar = char(247);

////////////////////////////////////////////////////////////////////
//Modulation Variables
////////////////////////////////////////////////////////////////////
MCP49xx dac(MCP49xx::MCP4901, SS_PIN);

uint16_t messageSymbolSize;                              //tracks the number of symbols to modulate
byte symbolValueOfMessage[MESSAGE_MAXIMUM_SYMBOL_SIZE];  //encoded symbols to modulate
byte symbolValueNotEncoded[MESSAGE_MAXIMUM_SYMBOL_SIZE]; //non-encoded symbols.

char outgoingMessagesBytes[MESSAGE_MAXIMUM_SYMBOL_SIZE / 8 + 1]; //the incoming chars/bytes are stored to be turned into non-encoded symbols
int outgoingMessagesBytesIndex;

uint16_t symbolIndex;
bool modulationActive;

uint16_t sine_data_index;

float volume = 0.75; //volume from 0 to 1

//modulation timers
IntervalTimer outgoingSineWaveDACTimer;
IntervalTimer outgoingSymbolUpdateTimer;

//http://www.wolframalpha.com/input/?i=table+round%28100%2B412*%28sin%282*pi*t%2F20%29%2B1%29%29+from+0+to+19

#define SINE_DATA_LENGTH 16                                                                                          //number of steps to take per sine wave
uint8_t sine_data_raw[SINE_DATA_LENGTH] = {128, 166, 197, 219, 226, 219, 197, 166, 128, 90, 59, 37, 30, 37, 59, 90}; //pre-computed sine wave
int16_t sine_data[SINE_DATA_LENGTH];                                                                                 //sine-wave including volume

////////////////////////////////////////////////////////////////////
//Demodulation Variables
////////////////////////////////////////////////////////////////////
volatile bool incomingSignalDetected = false;

char incomingMessageBytes[MESSAGE_MAXIMUM_SYMBOL_SIZE / 8 + 1];

byte incomingSymbols[MESSAGE_MAXIMUM_SYMBOL_SIZE];        //encoded symbols received
byte incomingSymbolsDecoded[MESSAGE_MAXIMUM_SYMBOL_SIZE]; //parity check and decoded symbols
byte *incomingSymbolsAdj;                                 //pointer to align incoming symbol
int inSymbolIndex;

//acomm(baud rate, update rate)
//update rate/baud rate must be an integer.
ACOMM acomm(2000, 250000);

//look into:
//pool test, confirm transducers are proper for our purposes before ordering more
//flag for switching back to listening channel
//send on one channel
//receive on another
//add saving data and waveform option

IntervalTimer demodTimer;

/////////////////////////Shared Functions/////////////////////////

void setup()
{
    pinMode(RELAY_SWITCH_PIN, OUTPUT);
    pinMode(demod_pin, INPUT);

    //set analog read resolution and average so that Goertzel can be calculated in real time
    analogReadResolution(8);
    analogReadAveraging(1);

    //begin the SPI to communicate with MCP4901 DAC
    SPI.begin();
    dac.output(0);

    //turn of relay (on == transmit)
    updateRelay(false);

    //set priority of timers to prevent interruption
    outgoingSineWaveDACTimer.priority(100);
    outgoingSymbolUpdateTimer.priority(50);
    demodTimer.priority(101);

    updateVolume();
    Serial.begin(115200); //begin Serial communication
    while (!Serial)
    {
        ;
    }

    //set hamming encoding
    hamming.Init(8);

    //set ACOMM, which handles the Comm channels which utilize Goertzel
    acomm.setMary(2);
    acomm.setChannel(0);

    beginStartCheckDemodTimer(); //start demod timer/checker for incoming message

    Serial.println("Modem Online"); //Send a note saying boot up
}

void loop()
{
    if (!modulationActive)
    {
        readIncomingSerialDataForModulation();
    }
}

void endAllTimers()
{
    demodTimer.end();
    outgoingSineWaveDACTimer.end();
    outgoingSymbolUpdateTimer.end();
}

/////////////////////////Modulation Functions/////////////////////////

void updateVolume()
{
    for (int i = 0; i < SINE_DATA_LENGTH; i++)
    {
        sine_data[i] = int((sine_data_raw[i] - sine_data_raw[0]) * volume) + sine_data_raw[0];
    }
}

void updateRelay(bool relayOn)
{
    digitalWrite(RELAY_SWITCH_PIN, !relayOn);
}

void messageCompleteAndModulationReset()
{
    endAllTimers();
    dac.output(byte(0));

    //ground output, then disconnect transducer from amplifier
    delay(50);
    updateRelay(false);
    delay(50);

    symbolIndex = 0;
    sine_data_index = 0;
    outgoingMessagesBytesIndex = 0;
    messageSymbolSize = 0;
    endDemodulationWithoutMessage();
    modulationActive = false;
    interrupts();
}

void startModulation()
{
    endAllTimers();
    modulationActive = true;
    updateRelay(true);
    delay(50);
    symbolIndex = 0;
    sine_data_index = 0;
    dac.output(byte(sine_data[0]));
    interrupts();
    beginTimersForModulation();
}

void endTimersForModulation()
{
    outgoingSineWaveDACTimer.end();
    outgoingSymbolUpdateTimer.end();
}

void beginTimersForModulation()
{
    while (!outgoingSymbolUpdateTimer.begin(outgoingSymbolUpdateTimerCallback, acomm.getUSecondsPerSymbol()))
    {
        ;
    }
    while (!outgoingSineWaveDACTimer.begin(outgoingSineWaveDACTimerCallback, convertIncomingBitToMicrosecondsPeriodForDACUpdate(symbolValueOfMessage[symbolIndex])))
    {
        ;
    }
}

void outgoingSymbolUpdateTimerCallback()
{
    symbolIndex++;
    if (symbolIndex >= messageSymbolSize)
    {
        messageCompleteAndModulationReset();
        return;
    }
    if (symbolValueOfMessage[symbolIndex - 1] != symbolValueOfMessage[symbolIndex])
    {
        outgoingSineWaveDACTimer.update(convertIncomingBitToMicrosecondsPeriodForDACUpdate(symbolValueOfMessage[symbolIndex]));
    }
    sine_data_index = sine_data_index % SINE_DATA_LENGTH;
}

void outgoingSineWaveDACTimerCallback()
{
    dac.output(byte(sine_data[sine_data_index % SINE_DATA_LENGTH]));
    sine_data_index++;
}

float convertIncomingBitToMicrosecondsPeriodForDACUpdate(byte symbol)
{
    return acomm.getRXFreqMicroSecondsFromSymbol(byte(symbol)) / SINE_DATA_LENGTH;
}

void saveBitToSymbolOut(int index, int tempBit)
{
    symbolValueOfMessage[index] = tempBit;
}

void convertBitsOutToSymbolsOut()
{
    for (int i = 0; i < messageSymbolSize; i += acomm.getNumBits())
    {
        byte *tempSymArray = symbolValueOfMessage + i;
        symbolValueOfMessage[i / acomm.getNumBits()] = acomm.convertSymbolAsBitArrayToByte(tempSymArray);
    }
    // messageSymbolSize = messageSymbolSize / acomm.getNumBits() + 1;
}

void convertMessageCharsToBitsOut()
{
    byte tempBits[8];
    messageSymbolSize = 0;
    for (int k = 0; k < outgoingMessagesBytesIndex; k++)
    {
        for (int i = 7; i >= 0; i--)
        {
            symbolValueNotEncoded[(7 - i) + k * 8] = (outgoingMessagesBytes[k] >> i) & 1;
            messageSymbolSize++;
        }
    }

    //if hamming is not active, copy to encoded file and return;
    if (!hammingActive)
    {
        //copy message from not encoded to encoded
        for (int i = 0; i < messageSymbolSize; i++)
        {
            symbolValueOfMessage[i] = symbolValueNotEncoded[i];
        }
        return;
    }

    //clear any bits that may cross-pollinate a new message
    for (int i = messageSymbolSize; i < messageSymbolSize + hamming.getEncodedMessageLength(); i++)
    {
        symbolValueNotEncoded[i] = 0;
    }

    byte *tempEncoded;
    byte *tempNotEncoded;
    int notEncInd = 0;
    int encInd = 0;
    while (notEncInd < messageSymbolSize)
    {
        tempNotEncoded = symbolValueNotEncoded + notEncInd;
        tempEncoded = symbolValueOfMessage + encInd;
        hamming.encode(tempNotEncoded, tempEncoded);
        notEncInd += hamming.getMessageLength();
        encInd += hamming.getEncodedMessageLength();
    }
    messageSymbolSize = encInd;
}

void printModulationAfterSymbolsReady()
{
    Serial.println();
    Serial.print("[");
    for (int i = 0; i < messageSymbolSize; i++)
    {
        Serial.print(symbolValueOfMessage[i]);
        Serial.print(" ");
    }
    Serial.println("]");
    Serial.println(messageSymbolSize);
}

void handleJson()
{

    if (root.containsKey("volume"))
    {
        Serial.println("volume json detected");
        volume = root["volume"];
        volume = max(0, min(1, volume));
        updateVolume();
    }

    if (root.containsKey("channel"))
    {
        Serial.println("channel json detected");
        int channel = root["channel"];
        channel = int(max(0, min(acomm.getNumChannels() - 1, channel)));
        acomm.setChannel(channel);
    }

    if (root.containsKey("baud"))
    {
        int newBaud = root["baud"];
        newBaud = int(max(500, min(2000, newBaud)));
        acomm.setBaud(newBaud);
    }

    if (root.containsKey("tx"))
    {
        Serial.println("tx json detected");
        int channel = root["tx"];
        acomm.setTX(channel);
    }

    if (root.containsKey("rx"))
    {
        Serial.println("rx json detected");
        int channel = root["rx"];
        acomm.setRX(channel);
    }
    if (root.containsKey("response"))
    {
        responseDoc.clear();
        responseDoc["volume"] = volume;
        if (acomm.getRX() == acomm.getTX() || acomm.getTX() == -1)
        {
            responseDoc["channel"] = acomm.getCurrentChannel();
        }
        else
        {
            responseDoc["tx"] = acomm.getTX();
            responseDoc["rx"] = acomm.getRX();
            responseDoc["channel"] = -1;
        }
        responseDoc["baud"] = acomm.getBaud();
        serializeJson(responseDoc, Serial); // {"hello":"world"}
    }
}

bool isIncomingSerialMessageJSON(byte pj[])
{
    DeserializationError error = deserializeJson(doc, pj);
    // Test if parsing succeeds.
    if (error)
    {
        //Serial.print(F("deserializeJson() failed: "));
        //Serial.println(error.f_str());
        return false;
    }
    return true;
}

bool doesJSONIncludeMessage()
{
    if (root.containsKey("msg"))
    {
        const char *temp = root["msg"];
        int leng = strlen(temp);
        for (int i = 0; i < leng; i++)
        {
            outgoingMessagesBytes[i + 1] = temp[i];
        }
        outgoingMessagesBytes[leng + 1] = '\n';
        outgoingMessagesBytesIndex = leng + 2;
        return true;
    }
    return false;
}

void readIncomingSerialDataForModulation()
{
    if (Serial.available())
    {
        if (outgoingMessagesBytesIndex == 0)
        {
            interruptDemodulationWithModulation();
            outgoingMessagesBytes[outgoingMessagesBytesIndex++] = leadChar;
            return;
        }

        outgoingMessagesBytes[outgoingMessagesBytesIndex++] = Serial.read();
        if (outgoingMessagesBytes[outgoingMessagesBytesIndex - 1] == '\n') //if character is a new line, end of message
        {
            byte *possibleJson = outgoingMessagesBytes + 1;
            if (isIncomingSerialMessageJSON(possibleJson))
            {
                handleJson();
                if (!doesJSONIncludeMessage())
                {
                    messageCompleteAndModulationReset();
                    return;
                }
            }

            convertMessageCharsToBitsOut();
            convertBitsOutToSymbolsOut();
            interrupts();
            startModulation();
        }
    }
}

// /////////////////////////Demodulation Functions/////////////////////////
void setLeadingDemodSymbols()
{
    for (int i = 0; i < 2; i++)
    {
        if (acomm.getNumBits() == 1)
        {
            addSymbolToIncomingSymbols(1);
        }
        else
        {
            addSymbolToIncomingSymbols(int(pow(acomm.getNumBits(), 2) - 1));
        }
        inSymbolIndex++;
    }
}

void beginStartCheckDemodTimer()
{
    setLeadingDemodSymbols();
    while (!demodTimer.begin(checkingForDemodStartCondition, acomm.getTimerUpdateSpeedMicros()))
    {
        ;
    }
}

void beginActiveDemodTimer()
{
    while (!demodTimer.begin(activeDemodTimerCallback, acomm.getTimerUpdateSpeedMicros()))
    {
        ;
    }
}

void checkingForDemodStartCondition()
{
    acomm.addSample(analogRead(demod_pin));
    noInterrupts();
    if (acomm.detectStartOfMessageAboveThreshold())
    {
        demodTimer.end();
        beginActiveDemodTimer();
    }
    interrupts();
}

void activeDemodTimerCallback()
{
    acomm.addSample(analogRead(demod_pin));
    if (acomm.symbolReadyToProcess())
    {
        noInterrupts();
        byte symbol = acomm.getSymbolAsByte();
        if (symbol != acomm.endMessageValue)
        {
            addSymbolToIncomingSymbols(symbol);
            inSymbolIndex++;
        }
        else
        {
            demodTimer.end();
            endDemodulation();
        }
        interrupts();
    }
}

void interruptDemodulationWithModulation()
{
    endAllTimers();
    inSymbolIndex = 0;
    incomingSignalDetected = false;
}

void endDemodulationWithoutMessage()
{
    endAllTimers();
    inSymbolIndex = 0;
    beginStartCheckDemodTimer();
    interrupts();
    incomingSignalDetected = false;
}

void printRawSymbolsIn()
{
    for (int i = 0; i < inSymbolIndex; i++)
    {
        Serial.print(incomingSymbols[i]);
        Serial.print(" ");
    }
    Serial.println();
}

void endDemodulation()
{
    endAllTimers();
    incomingSymbolsAdj = incomingSymbols;
    //printRawSymbolsIn();
    //convertRawSymbolsToBits();
    int shifter = amountToShift();
    // Serial.print("shifter: ");
    // Serial.println(shifter);
    if (shifter < 0)
    {
        return endDemodulationWithoutMessage();
    }
    incomingSymbolsAdj = incomingSymbols + shifter;

    if (hammingActive)
    {
        inSymbolIndex = hamming.decode(incomingSymbolsAdj, incomingSymbolsDecoded, inSymbolIndex);
    }
    else
    {
        copyIncomingSymbolsToDecoded();
    }
    //align incoming bits with lead char. This corrects for if the first
    //bit was missed or double counted
    //take symbols read in, and convert into bytes
    convertBitsIntoMessageBytes();
    //publish the data to the Serial port
    //publishBinaryMessage();
    publishIncomingData();
    //reset all tracking variables
    inSymbolIndex = 0;
    beginStartCheckDemodTimer();
    interrupts();
    //attach intterupt timer to check for signal start conditions.
    //reset demoulation flag
    incomingSignalDetected = false;
}

void addSymbolToIncomingSymbols(int symbol)
{
    incomingSymbols[inSymbolIndex] = symbol;
}

void copyIncomingSymbolsToDecoded()
{
    for (int i = 0; i < inSymbolIndex; i++)
    {
        incomingSymbolsDecoded[i] = incomingSymbolsAdj[i];
    }
}

byte convertBitsToByte(byte *bits)
{
    byte temp = 0;
    byte pow = 1;
    for (int i = 7; i >= 0; i--)
    {
        // Serial.print(bits[i]);
        // Serial.print(" ");
        temp += pow * bits[i];
        pow *= 2;
    }
    Serial.println();
    return temp;
}

int amountToShift()
{
    //can only handle hamming 8. need to add other hamming options
    byte *temp = incomingSymbols;
    int ind = 0;
    bool matches = false;
    if (hammingActive)
    {
        byte holder[hamming.getEncodedMessageLength()];
        while (!matches)
        {
            temp = incomingSymbols + ind;
            hamming.parityCheckAndErrorCorrectionNoEdit(temp, holder);
            if (convertBitsToByte(holder) == leadChar)
            {
                return ind;
            }
            ind += 1;
            if (ind > 10)
            {
                return -1;
            }
        }
    }
    else
    {
        while (!matches)
        {
            temp = incomingSymbols + ind;
            if (convertBitsToByte(temp) == leadChar)
            {
                return ind;
            }
            ind += 1;
            if (ind > 10)
            {
                return -1;
            }
        }
    }
}

void correctForShiftedMessage()
{
    if (incomingSymbols[4] == 0)
    {
        return;
    }
    //Serial.println("had to shift message");
    int pos = 0;
    while (incomingSymbols[pos] != 0)
    {
        pos++;
    }
    int correctAmount = abs(pos - 4);
    if (pos - 4 > 0)
    {
        for (int i = 0; i < inSymbolIndex; i++)
        {
            incomingSymbols[i] = incomingSymbols[i + correctAmount];
        }
        inSymbolIndex -= correctAmount;
    }
    else
    {
        for (int i = inSymbolIndex - 1; i >= 0; i--)
        {
            incomingSymbols[i + correctAmount] = incomingSymbols[i];
        }
        for (int i = 0; i < correctAmount; i++)
        {
            incomingSymbols[i] = 1;
        }
        inSymbolIndex += correctAmount;
    }
}

void publishBinaryMessage()
{
    for (int i = 0; i < inSymbolIndex; i++)
    {
        if (i % hamming.getEncodedMessageLength() == 0 && i != 0)
        {
            Serial.print(" ");
        }
        Serial.print(incomingSymbols[i]);
    }
    Serial.println();
}

void publishIncomingData() //publishes the available data in bytesIn to the Serial stream
{
    int pos = 0;
    pos = inSymbolIndex / 8;
    while (pos > 1 && incomingMessageBytes[pos] != '\n')
    {
        pos--;
    }
    if (pos <= 1)
    {
        return;
    }
    incomingMessageBytes[pos] = 0;
    Serial.println(incomingMessageBytes + 1);
}

void convertBitsIntoMessageBytes()
{
    for (int i = 0; i < inSymbolIndex; i += 8)
    {
        incomingMessageBytes[i / 8] = convertSymbolsToByteAtIndex(incomingSymbolsDecoded, i);
    }
}

char convertSymbolsToByteAtIndex(byte arr[], int incomingSymbolsTempIndex)
{
    int byteAtIndex = (arr[incomingSymbolsTempIndex + 7]) + (arr[incomingSymbolsTempIndex + 6] * 2) +
                      (arr[incomingSymbolsTempIndex + 5] * 4) + (arr[incomingSymbolsTempIndex + 4] * 8) +
                      (arr[incomingSymbolsTempIndex + 3] * 16) + (arr[incomingSymbolsTempIndex + 2] * 32) +
                      (arr[incomingSymbolsTempIndex + 1] * 64) + (arr[incomingSymbolsTempIndex + 0] * 128);
    return char(byteAtIndex);
}
