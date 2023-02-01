/*Performing FSK modulation utilizing a DAC on a teensy 4.1
   Author: Scott Mayberry
*/
#include <stdint.h>
#include "ACOMM_FHFSK.h"
#include <ArduinoJson.h>
#include <SPI.h>
#include <MCP49xx.h>
//#include <Entropy.h>
#include "sdios.h"

#define MESSAGE_MAXIMUM_SYMBOL_SIZE 16384 * 4
#define adc_cs_pin 9
#define RELAY_SWITCH_PIN 14
#define g0 8
#define g1 7
#define g2 6
/*
    | g2 | g1 | g0 | value |
    |  0 |  0 |  0 |   0   |
    |  0 |  0 |  1 |   1   |
    |  0 |  1 |  0 |   2   |
    |  0 |  1 |  1 |   3   |
    |  1 |  0 |  0 |   4   |
    |  1 |  0 |  1 |   5   |
    |  1 |  1 |  0 |   6   |
    |  1 |  1 |  1 |   7   |
*/
#define SINE_DATA_LENGTH 16

DynamicJsonDocument doc(2048);
DynamicJsonDocument responseDoc(256);
JsonObject root = doc.to<JsonObject>();

ACOMM_FHFSK acomm(100, SINE_DATA_LENGTH);

const uint8_t LTC2315_bits = 12; //!< Default set for 12 bits
const uint8_t LTC2315_shift = 1;
const float LTC2315_vref = 4.096;

/////////////////////////Shared Functions/////////////////////////
elapsedMicros checkTimer;
uint64_t counter;
const int sample_freq_period_us = 5;
SPISettings spi_adc_settings(24000000, MSBFIRST, SPI_MODE0);

uint32_t sampleNumberCounter = 0;

void setup()
{
  setupPins();
  // begin the SPI to communicate with MCP4901 DAC
  SPI.begin();

  // turn of relay (on == transmit)
  updateRelay(false);

  Serial.begin(115200); // begin Serial communication
  delay(2000);
  Serial.println("Modem Online");
  Serial.print("Baud: ");
  Serial.println(acomm.getBaud());
  Serial.println("Receive Only");
  delay(500);
}

void setupPins()
{
  pinMode(RELAY_SWITCH_PIN, OUTPUT);
  pinMode(adc_cs_pin, OUTPUT);
  pinMode(g0, OUTPUT);
  pinMode(g1, OUTPUT);
  pinMode(g2, OUTPUT);
  digitalWrite(g0, HIGH);
  digitalWrite(g1, LOW);
  digitalWrite(g2, LOW);
  digitalWriteFast(adc_cs_pin, HIGH);
}

void loop()
{
  if (checkTimer / sample_freq_period_us >= counter)
  {
    if (checkTimer > 10000000)
    {
      checkTimer = 0;
      counter = 0;
    }
    counter++;
    addSample();
  }
}

void updateRelay(bool relayOn)
{
  digitalWrite(RELAY_SWITCH_PIN, true);
  // digitalWrite(RELAY_SWITCH_PIN, !relayOn);
}

// /////////////////////////Demodulation Functions/////////////////////////

void addSample()
{
  uint16_t sample = ADCRead();
  // long microsnow = micros();
  acomm.addSample(sample, sampleNumberCounter);
  sampleNumberCounter++;
  if (acomm.isMessageReady())
  {
    // Serial.print("\nGreatest Delay: ");
    // Serial.println(greatestDelay);
    acomm.addMessagesToJson(doc);
    serializeJson(doc, Serial);
    doc.clear();
    Serial.println();
    sampleNumberCounter = 0;
    delay(10);
    return;
  }
  // long microsnow2 = micros();
  // microsnow2 = microsnow2 - microsnow;
  // if (microsnow2 > greatestDelay)
  // {
  //     greatestDelay = microsnow2;
  // }
}

uint16_t ADCRead(void)
{
  SPI.beginTransaction(spi_adc_settings);
  digitalWriteFast(adc_cs_pin, LOW);
  uint16_t adc_code = SPI.transfer16(0);
  digitalWriteFast(adc_cs_pin, HIGH);
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
