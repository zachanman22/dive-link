/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include <SPI.h>

#define RELAY_SWITCH_PIN 14
#define adc_cs_pin 9
//const int pin_3k = 8;
//const int pin_10k = 7;
//const int pin_30k = 6;

#define G0_PIN 8
#define G1_PIN 7
#define G2_PIN 6

const uint8_t LTC2315_bits = 12; //!< Default set for 12 bits
const uint8_t LTC2315_shift = 1;
const float LTC2315_vref = 4.096;
/////////////////////////Shared Functions/////////////////////////
elapsedMicros checkTimer;
uint64_t counter;
const int sample_freq_period_us = 5;
SPISettings spi_adc_settings(24000000, MSBFIRST, SPI_MODE0);

int gainMultiplier = 1;

void setup()
{
    pinMode(RELAY_SWITCH_PIN, OUTPUT);
//    pinMode(adc_cs_pin, OUTPUT);
//    pinMode(pin_3k, OUTPUT);
//    pinMode(pin_10k, OUTPUT);
//    pinMode(pin_30k, OUTPUT);
//    changeGain(pin_10k);
    setGain(gainMultiplier);
    digitalWriteFast(adc_cs_pin, HIGH);

    // begin the SPI to communicate with the LTC2315-12
    SPI.begin();

    // turn off relay (true == transmit)
    updateRelay(false);

    // set priority of timers to prevent interruption
    //    demodTimer.priority(128);

    Serial.begin(115200); // begin Serial communication
    delay(2000);
    Serial.printf("?");
    counter = 1;
    checkTimer = 0;
}

//void changeGain(int pinNum){
//  digitalWrite(pin_3k, LOW);
//  digitalWrite(pin_10k, LOW);
//  digitalWrite(pin_30k, LOW);
//  digitalWrite(pinNum, HIGH);
//}

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
        demodTimerCallback();
    }
}

void updateRelay(bool relayOn)
{
    digitalWrite(RELAY_SWITCH_PIN, !relayOn);
}

// /////////////////////////Demodulation Functions/////////////////////////
void demodTimerCallback()
{
    uint16_t adc_code = ADCRead();
    Serial.print(micros());
    Serial.printf(" ");
    Serial.print(adc_code);
//    Serial.print(convertADCCodeToNormalizedValue(adc_code));
    Serial.print(" ");
//    Serial.print(convertADCCodeToVoltage(adc_code));
    //    Serial.print(random(4096));
    Serial.printf("$\n?");
}

uint16_t ADCRead(void)
{
    SPI.beginTransaction(spi_adc_settings);
    digitalWriteFast(adc_cs_pin, LOW);
    uint16_t dummy_command = 0;
    uint16_t adc_code = SPI.transfer16(0);
    digitalWriteFast(adc_cs_pin, HIGH);
    SPI.endTransaction();
    return adc_code >> 3;
}

void updatePGA(bool g0val, bool g1val, bool g2val)
{
    digitalWrite(G0_PIN, g0val);
    digitalWrite(G1_PIN, g1val);
    digitalWrite(G2_PIN, g2val);
}

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
