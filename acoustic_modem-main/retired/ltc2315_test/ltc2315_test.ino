//#define USE_SPI_CS
#define RELAY_SWITCH_PIN 14

#include <SPI.h>

const int adc_cs_pin = 9;
uint32_t csMASK;
static uint8_t LTC2315_bits = 12; //!< Default set for 12 bits
static uint8_t LTC2315_shift = 1;
float LTC2315_vref = 4.096;

SPISettings settingsB(24000000, MSBFIRST, SPI_MODE0);
SPISettings settingsA(24000000, MSBFIRST, SPI_MODE0);
void setup()
{
    pinMode(RELAY_SWITCH_PIN, OUTPUT);
    updateRelay(false);
    while (!Serial && (millis() < 2000))
        ;

    Serial.begin(115200);

    SPI.begin();
#ifdef USE_SPI_CS
    csMASK = SPI.setCS(adc_cs_pin) << 16;
    Serial.printf("CS Mask: %x\n", csMASK);
#else
    pinMode(adc_cs_pin, OUTPUT);
    digitalWriteFast(adc_cs_pin, HIGH);
#endif
}

void loop()
{
    int numOfSteps = 100;
    //  long ulStart = micros();
    //  for (auto i = 0; i < numOfSteps; i++) {
    //    SPI.beginTransaction(settingsA);
    //    ADCRead();
    //    SPI.endTransaction();
    //  }
    //  long dt = micros() - ulStart;
    //  Serial.println(dt/float(numOfSteps));
    //

    SPI.beginTransaction(settingsA);
    Serial.println(ADCRead(), 3);
    SPI.endTransaction();

    delayMicroseconds(2);
}

void updateRelay(bool relayOn)
{
    digitalWrite(RELAY_SWITCH_PIN, !relayOn);
}

float ADCRead(void)
{
    digitalWriteFast(adc_cs_pin, LOW);
    uint16_t dummy_command = 0;
    uint16_t adc_code = SPI.transfer16(0);
    digitalWriteFast(adc_cs_pin, HIGH);
    //  Serial.print("ADC: ");
    //  Serial.print(adc_code);
    float voltage;

    adc_code = adc_code << LTC2315_shift; // the data is left justified to bit_14 of a 16 bit word

    voltage = (float)adc_code;
    voltage = voltage / (pow(2, 16) - 1); //! 2) This calculates the input as a fraction of the reference voltage (dimensionless)
    voltage = voltage * LTC2315_vref;     //! 3) Multiply fraction by Vref to get the actual voltage at the input (in volts)
                                          //  Serial.print("  Voltage: ");
                                          //  Serial.println(voltage, 4);

    return (voltage);
}