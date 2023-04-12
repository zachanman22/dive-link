/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 * Modified to use external timer to keep ADC logging fixed at constant sampling rate
 */
#include <stdint.h>
#include <SPI.h>
#include <Adafruit_SI5351.h>

Adafruit_SI5351 clockgen = Adafruit_SI5351();

#define RELAY_SWITCH_PIN 14
#define adc_cs_pin 9
// set PGA (LTC6910-3) pins
#define G0_PIN 8
#define G1_PIN 7
#define G2_PIN 6

const uint8_t LTC2315_bits = 12; //!< Default set for 12 bits
const uint8_t LTC2315_shift = 1;
const float LTC2315_vref = 4.096;
/////////////////////////Shared Functions/////////////////////////
elapsedMicros checkTimer;
//counter for timing purposes
//sample rate is fixed at 500k (changing requires doing some math...)
uint64_t counter;
//200kHz sample rate
const int sample_freq_period_us = 4;
SPISettings spi_adc_settings(24000000, MSBFIRST, SPI_MODE0);
int gainMultiplier = 1;

void setup()
{
    pinMode(RELAY_SWITCH_PIN, INPUT);
    pinMode(adc_cs_pin, OUTPUT);
    // set PGA
    pinMode(G0_PIN, OUTPUT);
    pinMode(G1_PIN, OUTPUT);
    pinMode(G2_PIN, OUTPUT);
    setGain(0);
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
    Serial.println("Si5351 Clockgen Test"); Serial.println("");
  
    /* Initialise the sensor */
    if (clockgen.begin() != ERROR_NONE)
    {
      /* There was a problem detecting the IC ... check your connections */
      Serial.print("Ooops, no Si5351 detected ... Check your wiring or I2C ADDR!");
      while(1);
    }
  
    Serial.println("OK!");
  
    /* INTEGER ONLY MODE --> most accurate output */
    /* Setup PLLA to integer only mode @ 900MHz (must be 600..900MHz) */
    /* Set Multisynth 0 to 112.5MHz using integer only mode (div by 4/6/8) */
    /* 25MHz * 36 = 900 MHz, then 900 MHz / 8 = 112.5 MHz */
  //  Serial.println("Set PLLA to 900MHz");
  //  clockgen.setupPLLInt(SI5351_PLL_A, 36);
  //  Serial.println("Set Output #0 to 112.5MHz");
  //  clockgen.setupMultisynthInt(0, SI5351_PLL_A, SI5351_MULTISYNTH_DIV_8);
  
    /* FRACTIONAL MODE --> More flexible but introduce clock jitter */
    /* Setup PLLB to fractional mode @616.66667MHz (XTAL * 24 + 2/3) */
    /* Setup Multisynth 1 to 13.55311MHz (PLLB/45.5) */
    clockgen.setupPLL(SI5351_PLL_B, 16, 0, 1);
    Serial.println("Set Output #1 to 500kHz");
    clockgen.setupMultisynth(1, SI5351_PLL_B, 800, 0, 1);
  
    /* Multisynth 2 is not yet used and won't be enabled, but can be */
    /* Use PLLB @ 616.66667MHz, then divide by 900 -> 685.185 KHz */
    /* then divide by 64 for 10.706 KHz */
    /* configured using either PLL in either integer or fractional mode */
  
  //  Serial.println("Set Output #2 to 10.706 KHz");
  //  clockgen.setupMultisynth(2, SI5351_PLL_B, 900, 0, 1);
  //  clockgen.setupRdiv(2, SI5351_R_DIV_64);
  
  // initialize the pushbutton pin as an input:
    // Attach an interrupt to the ISR vector
    attachInterrupt(RELAY_SWITCH_PIN, demodTimerCallback, RISING);
  
    /* Enable the clocks */
    clockgen.enableOutputs(true);
}

void loop()
{
//  if (counter % 200000 == 0){
//    Serial.println(counter);
//    demodTimerCallback();
//  }
}

void pin_ISR() {
  counter += 1;
}

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

void updateRelay(bool relayOn)
{
    digitalWrite(RELAY_SWITCH_PIN, !relayOn);
}

// /////////////////////////Demodulation Functions/////////////////////////
void demodTimerCallback()
{
    uint16_t adc_code = ADCRead();
//    Serial.print(micros());
//    Serial.printf(" ");
    Serial.print(adc_code);
    //Serial.print(random(4096));
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
