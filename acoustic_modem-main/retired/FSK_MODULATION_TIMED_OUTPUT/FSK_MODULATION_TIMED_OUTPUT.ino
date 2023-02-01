/*Performing FSK modulation utilizing a DAC on a teensy 3.2
 * Author: Scott Mayberry
 */

#define OUTGOING_MESSAGE_MAXIMUM_SYMBOL_SIZE 16384
#define BAUD_RATE 3000
#define RELAY_SWITCH_PIN 23

int messageSymbolSize;
byte symbolValueOfMessage[OUTGOING_MESSAGE_MAXIMUM_SYMBOL_SIZE];

char outgoingMessagesBytes[OUTGOING_MESSAGE_MAXIMUM_SYMBOL_SIZE / 8 + 1];
int outgoingMessagesBytesIndex;

volatile uint16_t symbolIndex;
volatile uint16_t sine_data_index;
bool modulationActive;

bool hammingActive = true;

float minFrequency = 27027.027;
float maxFrequency = 29126.2136;
double minFrequencyMicroSeconds = 1.0 / minFrequency * 1000000;
double maxFrequencyMicroSeconds = 1.0 / maxFrequency * 1000000;
float uSECONDS_PER_SYMBOL;
float volume = 0.5;

IntervalTimer outgoingSineWaveDACTimer;
IntervalTimer outgoingSymbolUpdateTimer;

//http://www.wolframalpha.com/input/?i=table+round%28100%2B412*%28sin%282*pi*t%2F20%29%2B1%29%29+from+0+to+19
// #define SINE_DATA_LENGTH 20
// int16_t sine_data[SINE_DATA_LENGTH] = {512, 639, 754, 845, 904, 924, 904, 845, 754, 639, 512, 385, 270, 179, 120, 100, 120, 179, 270, 385};

#define SINE_DATA_LENGTH 16
int16_t sine_data[SINE_DATA_LENGTH] = {512, 670, 803, 893, 924, 893, 803, 670, 512, 354, 221, 131, 100, 131, 221, 354};

void setup()
{
    Serial.begin(115200); //begin Serial communication
    while (!Serial)
    {
        ;
    }
    Serial.println("Starting up Modulation"); //Send a note saying boot up
    analogWriteResolution(10);
    pinMode(RELAY_SWITCH_PIN, OUTPUT);
    updateRelay(false);
    outgoingSineWaveDACTimer.priority(100);
    outgoingSymbolUpdateTimer.priority(99);
    uSECONDS_PER_SYMBOL = 1 / (float(BAUD_RATE)) * 1000000;
    //outgoingSineWaveDACTimer.begin(timer0_callback, 1.25);
    pinMode(A14, OUTPUT);
    analogWrite(A14, 0);
    reduceSineWave();
}

void reduceSineWave()
{
    for (int i = 0; i < SINE_DATA_LENGTH; i++)
    {
        sine_data[i] = int((sine_data[i] - 512) * volume) + 512;
    }
}

void updateRelay(bool relayOn)
{
    if (relayOn)
    {
        digitalWrite(RELAY_SWITCH_PIN, HIGH);
    }
    else
    {
        digitalWrite(RELAY_SWITCH_PIN, LOW);
    }
}

void messageCompleteAndModulationReset()
{
    endTimersForModulation();
    analogWrite(A14, 0);
    delay(500);
    updateRelay(false);
    delay(200);
    symbolIndex = 0;
    sine_data_index = 0;
    outgoingMessagesBytesIndex = 0;
    messageSymbolSize = 0;
    Serial.println("Message Complete");
    modulationActive = false;
}

void startModulation()
{
    modulationActive = true;
    updateRelay(true);
    delay(1000);
    symbolIndex = 0;
    sine_data_index = 0;
    analogWrite(A14, sine_data[0]);
    beginTimersForModulation();
}

void endTimersForModulation()
{
    outgoingSineWaveDACTimer.end();
    outgoingSymbolUpdateTimer.end();
}

void beginTimersForModulation()
{
    while (!outgoingSineWaveDACTimer.begin(outgoingSineWaveDACTimerCallback, convertIncomingBitToMicrosecondsPeriodForDACUpdate(symbolValueOfMessage[0])))
    {
        ;
    }
    while (!outgoingSymbolUpdateTimer.begin(outgoingSymbolUpdateTimerCallback, uSECONDS_PER_SYMBOL))
    {
        ;
    }
}

void outgoingSymbolUpdateTimerCallback()
{
    symbolIndex++;
    if (symbolValueOfMessage[symbolIndex - 1] != symbolValueOfMessage[symbolIndex])
    {
        outgoingSineWaveDACTimer.update(convertIncomingBitToMicrosecondsPeriodForDACUpdate(symbolValueOfMessage[symbolIndex]));
    }
    sine_data_index = sine_data_index % SINE_DATA_LENGTH;
    if (symbolIndex >= messageSymbolSize)
    {
        messageCompleteAndModulationReset();
    }
}

void outgoingSineWaveDACTimerCallback()
{
    analogWrite(A14, sine_data[sine_data_index % SINE_DATA_LENGTH]);
    sine_data_index++;
}

void loop()
{
    if (!modulationActive)
    {
        readIncomingSerialDataForModulation();
    }
}

float convertIncomingBitToMicrosecondsPeriodForDACUpdate(int bit)
{
    if (bit == 0)
    {
        return minFrequencyMicroSeconds / SINE_DATA_LENGTH;
    }
    else
    {
        return maxFrequencyMicroSeconds / SINE_DATA_LENGTH;
    }
}

void addParityBitsToDACandPeriod(int tempBits[])
{
    int parityBits[5];
    parityBits[0] = tempBits[6] ^ tempBits[5] ^ tempBits[3] ^ tempBits[2] ^ tempBits[0];
    parityBits[1] = tempBits[6] ^ tempBits[4] ^ tempBits[3] ^ tempBits[1] ^ tempBits[0];
    parityBits[2] = tempBits[5] ^ tempBits[4] ^ tempBits[3];
    parityBits[3] = tempBits[2] ^ tempBits[1] ^ tempBits[0];
    parityBits[4] = tempBits[0] ^ tempBits[1] ^ tempBits[2] ^ tempBits[3] ^ tempBits[4] ^ tempBits[5] ^ tempBits[6] ^ parityBits[0] ^ parityBits[1] ^ parityBits[2] ^ parityBits[3];
    for (int i = 0; i < 5; i++)
    {
        Serial.print(parityBits[i]);
        saveBitToSymbolOut(outgoingMessagesBytesIndex * 12 + 7 + i, parityBits[i]);
    }
    Serial.print(" ");
}

void saveBitToSymbolOut(int index, int tempBit)
{
    symbolValueOfMessage[index] = tempBit;
}

void readIncomingSerialDataForModulation()
{
    if (Serial.available())
    {
        if (outgoingMessagesBytesIndex == 0)
        {
            outgoingMessagesBytes[outgoingMessagesBytesIndex] = 'w';
        }
        else
        {
            outgoingMessagesBytes[outgoingMessagesBytesIndex] = Serial.read();
        }

        //Serial.print(char(outgoingMessagesBytes[outgoingMessagesBytesIndex]));
        if (hammingActive)
        {
            int tempBits[7];
            for (int i = 6; i >= 0; i--) //convert new byte into bits and then convert bits into half frequency times for interval timer
            {
                tempBits[(6 - i)] = (outgoingMessagesBytes[outgoingMessagesBytesIndex] >> i) & 1;
                Serial.print(tempBits[(6 - i)]);
                saveBitToSymbolOut(outgoingMessagesBytesIndex * 12 + (6 - i), tempBits[(6 - i)]);
            }
            addParityBitsToDACandPeriod(tempBits);
            messageSymbolSize += 12;
        }
        else
        {
            for (int i = 7; i >= 0; i--) //convert new byte into bits and then convert bits into half frequency times for interval timer
            {
                int tempBit = (outgoingMessagesBytes[outgoingMessagesBytesIndex] >> i) & 1;
                Serial.print(tempBit);
                saveBitToSymbolOut(outgoingMessagesBytesIndex * 8 + (7 - i), tempBit);
            }
            Serial.print(" ");
            messageSymbolSize += 8;
        }
        if (outgoingMessagesBytes[outgoingMessagesBytesIndex] == '\n') //if character is a new line, end of message
        {
            Serial.println();
            Serial.println("Starting Message Modulation");
            startModulation();
        }
        outgoingMessagesBytesIndex++;
    }
}