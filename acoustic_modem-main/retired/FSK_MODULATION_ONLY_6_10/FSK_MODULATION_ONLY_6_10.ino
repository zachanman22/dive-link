/*Performing FSK modulation utilizing a DAC on a teensy 3.2
 * Author: Scott Mayberry
 */

#define OUTGOING_MESSAGE_MAXIMUM_SYMBOL_SIZE 2048
#define uSECONDS_PER_SYMBOL 2000

int messageSymbolSize;
int numPeriodsPerSymbol[OUTGOING_MESSAGE_MAXIMUM_SYMBOL_SIZE];
float DACUpdateSpeedPerSymbol[OUTGOING_MESSAGE_MAXIMUM_SYMBOL_SIZE];

char outgoingMessagesBytes[OUTGOING_MESSAGE_MAXIMUM_SYMBOL_SIZE + 1];
int outgoingMessagesBytesIndex;

volatile uint16_t periodCounter;
volatile uint16_t symbolIndex;
volatile uint16_t sine_data_index;
bool modulationActive;

bool hammingActive = true;

float minFrequency = 27000;
float maxFrequency = 28000;
double minFrequencySeconds = 1.0 / minFrequency;
double maxFrequencySeconds = 1.0 / maxFrequency;

IntervalTimer outgoingSineWaveDACTimer;

//http://www.wolframalpha.com/input/?i=table+round%28100%2B412*%28sin%282*pi*t%2F20%29%2B1%29%29+from+0+to+19
// #define SINE_DATA_LENGTH 20
// int16_t sine_data[SINE_DATA_LENGTH] = {512, 639, 754, 845, 904, 924, 904, 845, 754, 639, 512, 385, 270, 179, 120, 100, 120, 179, 270, 385};

#define SINE_DATA_LENGTH 16
int16_t sine_data[SINE_DATA_LENGTH] = {512, 670, 803, 893, 924, 893, 803, 670, 512, 354, 221, 131, 100, 131, 221, 354};
//half sine amount = [512, 575, 633, 678, 708, 718, 708, 678, 633, 575, 512, 448, 391, 345, 316, 306, 316, 345, 391, 448]
void setup()
{
  Serial.begin(57600); //begin Serial communication
  while (!Serial)
  {
    ;
  }
  Serial.println("Starting up Modulation"); //Send a note saying boot up
  analogWriteResolution(10);
  //outgoingSineWaveDACTimer.begin(timer0_callback, 1.25);
  pinMode(A14, OUTPUT);
  analogWrite(A14, 0);
  reduceSineWave();
}

void reduceSineWave()
{
  for (int i = 0; i < SINE_DATA_LENGTH; i++)
  {
    sine_data[i] = (sine_data[i] - 512) / 10 + 512;
  }
}

void messageCompleteAndModulationReset()
{
  analogWrite(A14, 0);
  periodCounter = 0;
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
  symbolIndex = 0;
  periodCounter = 0;
  sine_data_index = 0;
  outgoingSineWaveDACTimer.begin(outgoingSineWaveDACTimerCallback, DACUpdateSpeedPerSymbol[0]);
  analogWrite(A14, sine_data[0]);
}

void outgoingSineWaveDACTimerCallback()
{
  analogWrite(A14, sine_data[sine_data_index++]);
  if (sine_data_index >= SINE_DATA_LENGTH)
  {
    sine_data_index = 0;
    periodCounter++;
    if (periodCounter >= numPeriodsPerSymbol[symbolIndex])
    {
      periodCounter = 0;
      outgoingSineWaveDACTimer.update(DACUpdateSpeedPerSymbol[++symbolIndex]);
      //Serial.println(DACUpdateSpeedPerSymbol[symbolIndex - 1]);
      if (symbolIndex >= messageSymbolSize)
      {
        outgoingSineWaveDACTimer.end();
        messageCompleteAndModulationReset();
      }
    }
  }
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
    // if (minFrequencySeconds * 1000000 > float(uSECONDS_PER_SYMBOL) / SINE_DATA_LENGTH)
    // {
    //   return uSECONDS_PER_SYMBOL;
    // }
    return minFrequencySeconds * 1000000 / SINE_DATA_LENGTH;
  }
  else
  {
    return maxFrequencySeconds * 1000000 / SINE_DATA_LENGTH;
  }
}

int convertIncomingBitToNumOfPeriods(int bit)
{
  float incomingFrequencySeconds = 0;
  if (bit == 0)
  {
    // if (minFrequencySeconds * 1000000 > float(uSECONDS_PER_SYMBOL) / SINE_DATA_LENGTH)
    // {
    //   return 1;
    // }
    incomingFrequencySeconds = minFrequencySeconds * 1000000;
  }
  else
  {
    incomingFrequencySeconds = maxFrequencySeconds * 1000000;
  }
  return int(uSECONDS_PER_SYMBOL / incomingFrequencySeconds);
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
    convertBitToDACandPeriodUpdate(outgoingMessagesBytesIndex * 12 + 7 + i, parityBits[i]);
  }
  Serial.print(" ");
}

void convertBitToDACandPeriodUpdate(int index, int tempBit)
{
  DACUpdateSpeedPerSymbol[index] = convertIncomingBitToMicrosecondsPeriodForDACUpdate(tempBit);
  numPeriodsPerSymbol[index] = convertIncomingBitToNumOfPeriods(tempBit);
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
        convertBitToDACandPeriodUpdate(outgoingMessagesBytesIndex * 12 + (6 - i), tempBits[(6 - i)]);
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
        convertBitToDACandPeriodUpdate(outgoingMessagesBytesIndex * 8 + (7 - i), tempBit);
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