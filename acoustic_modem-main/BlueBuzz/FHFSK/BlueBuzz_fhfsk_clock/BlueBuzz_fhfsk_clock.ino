/*Performing FSK modulation utilizing a DAC on a teensy 4.1
   Author: Scott Mayberry
*/
#include <stdint.h>
#include "BLUEBUZZ_FHFSK.h"
#include <ArduinoJson.h>
#include <Adafruit_SI5351.h>
#include <SPI.h>
#include <MCP49xx.h>
//#include <Entropy.h>
#include "sdios.h"

#define MESSAGE_MAXIMUM_SYMBOL_SIZE 16384 * 4

// set T/R switch
#define RELAY_SWITCH_PIN 14

// set DAC (MCP4921) and ADC (LTC2315-12) pins
#define ADC_CS_PIN 9
#define DAC_CS_PIN 10 // system select pin for SPI communication utilizing MCP4901 DAC

// set PGA (LTC6910-3) pins
#define G0_PIN 8
#define G1_PIN 7
#define G2_PIN 6

// set CLK (Si5351) pins
#define CLK0_PIN 17
#define CLK1_PIN 16
#define CLK2_PIN 15

float clk0_frequency = 200000;
float clk1_frequency = 400000;
float clk2_frequency;

#define SINE_DATA_LENGTH 16

DynamicJsonDocument transmitDoc(8192);
DynamicJsonDocument receiveDoc(1024);
JsonObject root = transmitDoc.to<JsonObject>();

BLUEBUZZ_FHFSK bluebuzz(200, clk0_frequency, clk1_frequency);

const uint8_t LTC2315_bits = 12; //!< Default set for 12 bits
const uint8_t LTC2315_shift = 1;
const float LTC2315_vref = 4.096;

Adafruit_SI5351 clockgen = Adafruit_SI5351();

/////////////////////////fhfsk out/////////////////////////
MCP49xx dac(MCP49xx::MCP4921, DAC_CS_PIN);

uint16_t messageSymbolSize;                              // tracks the number of symbols to modulate
byte symbolValueOfMessage[MESSAGE_MAXIMUM_SYMBOL_SIZE];  // encoded symbols to modulate
byte symbolValueNotEncoded[MESSAGE_MAXIMUM_SYMBOL_SIZE]; // non-encoded symbols.

char outgoingMessagesBytes[MESSAGE_MAXIMUM_SYMBOL_SIZE / 8 + 1]; // the incoming chars/bytes are stored to be turned into non-encoded symbols
int outgoingMessagesBytesIndex;

int symbolIndex;
bool modulationActive;

uint16_t sine_data_index;

float volume = 0.01; // volume from 0 to 1

// https://www.wolframalpha.com/input?i=table+round%2825%2B103*%28sin%282*pi*t%2F20%29%2B1%29%29+from+0+to+19
// sine-wave including volume
//
// #define SINE_DATA_LENGTH 16                                                                                                             // number of steps to take per sine wave
// uint16_t sine_data_raw[SINE_DATA_LENGTH] = {2048, 2793, 3425, 3848, 3996, 3848, 3425, 2793, 2048, 1303, 671, 248, 100, 248, 671, 1303}; //{128, 167, 201, 223, 231, 223, 201, 167, 128, 89, 55, 33, 25, 33, 55, 89}; // pre-computed sine wave
// int16_t sine_data[SINE_DATA_LENGTH];

// const int number_of_vol_levels = 20;
// int16_t sine_data_by_vol[number_of_vol_levels][SINE_DATA_LENGTH];
// int volume_index = 0;

// acomm(baud rate, update rate)
// update rate/baud rate must be an integer.
// { 47000, 0.95}
// const int num_frequency_pair = 6;
// float frequency_data[num_frequency_pair][4] = {{25000, 0.3, 32500, 0.5}};
//  6-6-22 tests
// float frequency_data[num_frequency_pair][4] = {{25000, 0.45, 33000, 0.2},
//                                                {28000, 0.35, 36000, 0.45},
//                                                {26000, 0.2, 34500, 0.5},
//                                                {30000, 0.4, 36500, 0.7},
//                                                {27000, 0.25, 35500, 0.35},
//                                                {32500, 0.5, 42000, 0.85}};

float currentMicroseconds;
uint64_t previousCPURead;
uint64_t currentCPURead;
uint32_t freq_sample_counter = 0;
uint64_t offset_amount = 0;
uint32_t symbolUpdateTimerCounter = 1;
const float micro_freq_period_us = float(F_CPU) / 1000000;
float dac_freq_us = 10;
int gainMultiplier = 1;

elapsedMicros symbolUpdateTimer;

int modStepCounter = 0;

/////////////////////////Shared Functions/////////////////////////
SPISettings spi_adc_settings(24000000, MSBFIRST, SPI_MODE0);


/**
 * @brief Set the Gain of LTC6910-3 used in preamp
 *
 * @param gainMultiplierUpdate new value of the gain multiplier, range: [0,7]
 */
void setGain(int gainMultiplierUpdate)
{
    if (gainMultiplierUpdate >= 0 && gainMultiplierUpdate <= 7)
    {
        gainMultiplier = gainMultiplierUpdate;
        switch (gainMultiplier)
        {
        case 0:
            updatePGA(false, false, false);
            break;
        case 1:
            updatePGA(true, false, false);
            break;
        case 2:
            updatePGA(false, true, false);
            break;
        case 3:
            updatePGA(true, true, false);
            break;
        case 4:
            updatePGA(false, false, true);
            break;
        case 5:
            updatePGA(true, false, true);
            break;
        case 6:
            updatePGA(false, true, true);
            break;
        case 7:
            updatePGA(true, true, true);
            break;
        default:
            setGain(1);
        }
    }
}

/**
 * @brief updates the LTC6910-3 with the below values
 *
    | g2 | g1 | g0 | value |
    |  0 |  0 |  0 |   0   |
    |  0 |  0 |  1 |   1   |
    |  0 |  1 |  0 |   2   |
    |  0 |  1 |  1 |   3   |
    |  1 |  0 |  0 |   4   |
    |  1 |  0 |  1 |   5   |
    |  1 |  1 |  0 |   6   |
    |  1 |  1 |  1 |   7   |
 *
 * @param g0val
 * @param g1val
 * @param g2val
 */
void updatePGA(bool g0val, bool g1val, bool g2val)
{
    digitalWrite(G0_PIN, g0val);
    digitalWrite(G1_PIN, g1val);
    digitalWrite(G2_PIN, g2val);
}

/////////////////////////////setup////////////////////////////////////
void setup()
{
    Serial.begin(115200); // begin Serial communication
    setupPins();
    Serial.println("Pins Configured");
    delay(100);
    // begin the SPI to communicate with MCP4901 DAC
    SPI.begin();
    dac.output(0);
    Serial.println("SPI Configured");
    delay(100);
    setupClocks();
    Serial.println("CLK Generator Configured");
    delay(1000);
    Serial.println("Modem Online");
    Serial.print("Baud: ");
    Serial.println(bluebuzz.getBaud());
    delay(500);
    //attachModulationInterrupt();
    attachDemodulationInterrupt();
    delay(10);
    clockgen.enableOutputs(true);
}

void attachDemodulationInterrupt()
{
    attachInterrupt(CLK0_PIN, readAndAddSample, RISING);
}

void detachDemodulationInterrupt()
{
    detachInterrupt(CLK0_PIN);
}

void attachModulationInterrupt()
{
    attachInterrupt(CLK1_PIN, updateModulation, RISING);
}

void detachModulationInterrupt()
{
    detachInterrupt(CLK1_PIN);
}

void readAndAddSample()
{
    uint16_t sample = ADCRead();
    // long microsnow = micros();
    bluebuzz.addSample(sample, gainMultiplier);
    if (bluebuzz.isMessageReady())
    {   
        clockgen.enableOutputs(false);
        detachDemodulationInterrupt();
        // Serial.print("\nGreatest Delay: ");
        // Serial.println(greatestDelay);
        bluebuzz.addMessagesToJson(transmitDoc);
        serializeJson(transmitDoc, Serial);
        transmitDoc.clear();
        Serial.println();
        Serial.flush();
        delay(10);
        clockgen.enableOutputs(true);
        attachDemodulationInterrupt();
        return;
    }
}

/**
 * @brief sets up pinmodes and initial outputs for used pins
 *
 */
void setupPins()
{
    // set adc
    pinMode(ADC_CS_PIN, OUTPUT);
    digitalWriteFast(ADC_CS_PIN, HIGH);

    // set PGA
    pinMode(G0_PIN, OUTPUT);
    pinMode(G1_PIN, OUTPUT);
    pinMode(G2_PIN, OUTPUT);
    setGain(1);

    // set CLK
    pinMode(CLK0_PIN, INPUT_PULLUP);
    pinMode(CLK1_PIN, INPUT_PULLUP);
    pinMode(CLK2_PIN, INPUT_PULLUP);

    // set Relay
    pinMode(RELAY_SWITCH_PIN, OUTPUT);
    updateRelay(false);
}

/**
 * @brief sets up the CLK generator with set frequencies;
 * CLK0 - 200kHz;
 * CLK1 - 250kHz;
 * CLK2 - unused;
 *
 */
void setupClocks()
{
    /* Initialise the sensor */
    if (clockgen.begin() != ERROR_NONE)
    {
        /* There was a problem detecting the IC ... check your connections */
        Serial.print("Ooops, no Si5351 detected ... Check your wiring or I2C ADDR!");
        while (1)
            ;
    }
    /* CLK0_PIN*/
    /* FRACTIONAL MODE --> More flexible but introduce clock jitter */
    /* Setup PLLA to fractional mode @400MHz (XTAL * 16 + 0/1) */
    /* Setup Multisynth to 200kHz (PLLA/2000) = 400MHz/2000 = 200kHz */

    clockgen.setupPLL(SI5351_PLL_A, 16, 0, 1);
    clockgen.setupMultisynth(0, SI5351_PLL_A, 2000, 0, 1);
    clk0_frequency = 200000;

    /* CLK1_PIN*/
    /*setup Multisynth to 250kHz (PLLA/1600) = 400MHz/1600 = 400kHz */
    //clockgen.setupPLL(SI5351_PLL_A, 16, 0, 1);
    clockgen.setupMultisynth(1, SI5351_PLL_A, 1000, 0, 1);
    clk1_frequency = 400000;

    /* CLK2_PIN*/
    /*unused */
}


/**
 * @brief check for serial data if there is not an active message
 * 
 */
void loop()
{
    if (!bluebuzz.isMessageInProgress() && !bluebuzz.getModulationActive() && !bluebuzz.isMessageReady())
    {
        readIncomingSerialDataForModulation();
    }
}

// /////////////////////////Demodulation Functions/////////////////////////

uint16_t ADCRead(void)
{
    SPI.beginTransaction(spi_adc_settings);
    digitalWriteFast(ADC_CS_PIN, LOW);
    uint16_t adc_code = SPI.transfer16(0);
    digitalWriteFast(ADC_CS_PIN, HIGH);
    SPI.endTransaction();
    return adc_code >> 3;
}

float convertADCCodeToVoltage(uint16_t adc_code)
{
    float voltage;
    voltage = (float)adc_code;
    voltage = voltage / (pow(2, 16) - 1); //! 2) This calculates the input as a fraction of the reference voltage (dimensionless)
    voltage = voltage * LTC2315_vref;     //! 3) Multiply fraction by Vref to get the actual voltage at the input (in volts)
                                          //  Serial.print("  Voltage: ");
                                          //  Serial.println(voltage, 4);
    return (voltage);
}
float convertADCCodeToNormalizedValue(uint16_t adc_code)
{
    float normalizedValue;
    normalizedValue = (float)adc_code;
    normalizedValue = (normalizedValue - pow(2, 15)) / pow(2, 15);
    return (normalizedValue);
}


/////////////////////////Modulation Functions/////////////////////////

/**
 * @brief interrupt for modulation.
 * 
 */
void updateModulation()
{
    //Serial.println(0);
    bluebuzz.incrementDACSymbolIndexIfNeeded(symbolIndex);
    if(symbolIndex >= messageSymbolSize){
        messageCompleteAndModulationReset();
    }
    int newDacValue = bluebuzz.getDACModulationValue(symbolValueOfMessage[symbolIndex], volume);
    dac.output(newDacValue);
}



/**
 * @brief sets the relay on or off depending on boolean;
 * True = transmission ON;
 * False = transmission OFF;
 * 
 * @param relayOn 
 */
void updateRelay(bool relayOn)
{
    digitalWrite(RELAY_SWITCH_PIN, !relayOn);
}

/**
 * @brief end message modulation
 * 
 */
void messageCompleteAndModulationReset()
{
    detachModulationInterrupt();
    bluebuzz.setModulation(false);

    //Serial.println("Message Complete");

    // ground output, then disconnect transducer from amplifier
    delayMicroseconds(50);
    updateRelay(false);

    symbolIndex = 0;
    outgoingMessagesBytesIndex = 0;
    messageSymbolSize = 0;
    
    delay(10);
    attachDemodulationInterrupt();   
}

void startModulation()
{
    //Serial.println("Message Started");
    detachDemodulationInterrupt();
    updateRelay(true);
    symbolIndex = 0;
    delay(10);
    bluebuzz.setModulation(true);
    attachModulationInterrupt();
}

void saveBitToSymbolOut(int index, int tempBit)
{
    symbolValueOfMessage[index] = tempBit;
}

bool convertBitsOutToSymbolsOut()
{
    symbolValueOfMessage[0] = bluebuzz.number_of_symbols - 1;
    // Serial.print(symbolValueOfMessage[0]);
    for (int i = 1; i < outgoingMessagesBytesIndex; i += 1)
    {
        int symbol = int(outgoingMessagesBytes[i - 1] - '0');
        if (symbol < 0 || symbol > 9)
        {
            Serial.println("Message stopped");
            return false;
        }
        symbolValueOfMessage[i] = symbol;
        // Serial.print(symbolValueOfMessage[i]);
    }
    // Serial.println();
    //  symbolValueOfMessage[outgoingMessagesBytesIndex] = 0;
    //  Serial.print(symbolValueOfMessage[outgoingMessagesBytesIndex]);
    //  symbolValueOfMessage[outgoingMessagesBytesIndex + 1] = 0;
    //  Serial.println(symbolValueOfMessage[outgoingMessagesBytesIndex + 1]);
    messageSymbolSize = outgoingMessagesBytesIndex;
    return true;
}

void handle_config_messages(){
    DeserializationError error = deserializeJson(receiveDoc, outgoingMessagesBytes);

    // Test if parsing succeeds.
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    if(!receiveDoc.containsKey("bluebuzz")){
        receiveDoc.clear();
        //Serial.println("no bluebuzz key");
        return;
    }

    float newVolume = receiveDoc["volume"];
    if(newVolume > 0 && newVolume <= 1){
        volume = newVolume;
    }

    int newBaud = receiveDoc["baud"];
    if(newBaud != bluebuzz.getBaud()){
        switch(newBaud){
            case 0: break;
            case 50: bluebuzz.reinit(50, clk0_frequency, clk1_frequency);
                     break;
            case 80: bluebuzz.reinit(80, clk0_frequency, clk1_frequency);
                     break;
            case 100: bluebuzz.reinit(100, clk0_frequency, clk1_frequency);
                      break;
            case 125: bluebuzz.reinit(125, clk0_frequency, clk1_frequency);
                      break;
            case 150: bluebuzz.reinit(150, clk0_frequency, clk1_frequency);
                      break;
            case 200: bluebuzz.reinit(200, clk0_frequency, clk1_frequency);
                      break;
            case 250: bluebuzz.reinit(250, clk0_frequency, clk1_frequency);
                      break; 
            default: break;
        }
    }

    if(receiveDoc.containsKey("response") && receiveDoc["response"].is<bool>()){
        bool responseRequest = receiveDoc["response"];
        Serial.println("response requested");
        if(responseRequest){
            receiveDoc.clear();
            receiveDoc["volume"] = volume;
            receiveDoc["baud"] = bluebuzz.getBaud();
            serializeJson(receiveDoc, Serial);
            Serial.println();
        }
    }

    receiveDoc.clear();
    

}

void readIncomingSerialDataForModulation()
{
    if (Serial.available())
    {
        char newestChar = Serial.read();
        //Serial.print(newestChar);
        outgoingMessagesBytes[outgoingMessagesBytesIndex++] = newestChar;
        if (newestChar == '\n')
        {
            if (!convertBitsOutToSymbolsOut())
            {
                outgoingMessagesBytes[outgoingMessagesBytesIndex+1] = '\0';
                // converting to bits does not work, try json conversion
                handle_config_messages();
                messageCompleteAndModulationReset();
                return;
            }
            startModulation();
        }
    }
}