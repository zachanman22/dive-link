/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include "ACOMM_FHFSK.h"
#include <ArduinoJson.h>

#include "SdFat.h"

DynamicJsonDocument doc(4096);
#define SINE_DATA_LENGTH 16
// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 0
/*
  Change the value of SD_CS_PIN if you are using SPI and
  your hardware does not use the default value, SS.
  Common values are:
  Arduino Ethernet shield: pin 4
  Sparkfun SD shield: pin 8
  Adafruit SD shields and modules: pin 10
*/

// SDCARD_SS_PIN is defined for the built-in SD on some boards.
#ifndef SDCARD_SS_PIN
const uint8_t SD_CS_PIN = SS;
#else  // SDCARD_SS_PIN
// Assume built-in SD is used.
const uint8_t SD_CS_PIN = SDCARD_SS_PIN;
#endif // SDCARD_SS_PIN

// Try max SPI clock for an SD. Reduce SPI_CLOCK if errors occur.
#define SPI_CLOCK SD_SCK_MHZ(50)

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)
#else // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)
#endif // HAS_SDIO_CLASS

#if SD_FAT_TYPE == 0
SdFat sd;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 sd;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile file;
#else // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif // SD_FAT_TYPE

char line[40];
long greatestDelay = 0;

char fileBuf[100] = "";

//------------------------------------------------------------------------------
// Store error strings in flash to save RAM.
#define error(s) sd.errorHalt(&Serial, F(s))
//------------------------------------------------------------------------------
// Check for extra characters in field or find minus sign.
char *skipSpace(char *str)
{
    while (isspace(*str))
        str++;
    return str;
}
//------------------------------------------------------------------------------
int32_t parseLine(char *str)
{
    char *ptr;

    // Set strtok start of line.
    str = strtok(str, ",");
    if (!str)
        return 0;

    // Convert string to long integer.
    int32_t i32 = strtol(str, &ptr, 0);
    if (str == ptr || *skipSpace(ptr))
        return 0;

    return i32;
}
unsigned int baud = 100;
ACOMM_FHFSK acomm(baud, SINE_DATA_LENGTH); //(baud, SINE_DATA_LENGTH);
void setup()
{
    delay(100);
    Serial.begin(115200);
    while (!Serial)
    {
    }
    Serial.println("ACOMM started");
    int baudfilelen = 1;
    int baud_and_file[baudfilelen][2] = {{100, 1}};
    // sprintf_P(fileBuf, PSTR("data_%i_%i.csv"), baud, 1);
    // Serial.println(fileBuf);

    delay(500);
    for (int k = 0; k < baudfilelen; k++)
    {
        sprintf_P(fileBuf, PSTR("data_%i_%i.csv"), baud_and_file[k][0], baud_and_file[k][1]);
        // acomm = new ACOMM_FHFSK(baud_and_file[k][0], SINE_DATA_LENGTH);
        delay(100);
        Serial.println(fileBuf);
        // Initialize the SD.
        if (!sd.begin(SD_CONFIG))
        {
            sd.initErrorHalt(&Serial);
            return;
        }
        // Create the file.
        if (!file.open(fileBuf, FILE_READ))
        {
            error("open failed");
        }

        int sampleNumberCounter = 0;
        while (file.available())
        {
            int n = file.fgets(line, sizeof(line));
            if (n <= 0)
            {
                error("fgets failed");
            }
            if (line[n - 1] != '\n' && n == (sizeof(line) - 1))
            {
                error("line too long");
            }
            int sample = int(parseLine(line));
            // Serial.println(sample);
            addSample(sample, sampleNumberCounter);
            sampleNumberCounter++;
        }
        file.close();
        Serial.println("\nDone");
        delay(3000);
    }
}

void loop()
{
    ;
}

void addSample(int sample, int sampleNumberCounter)
{
    long microsnow = micros();
    acomm.addSample(sample, sampleNumberCounter);
    if (acomm.isMessageReady())
    {
        Serial.print("\nGreatest Delay: ");
        Serial.println(greatestDelay);
        acomm.addMessagesToJson(doc);
        serializeJson(doc, Serial);
        Serial.println();
        greatestDelay = 0;
        delay(10);
        return;
    }
    long microsnow2 = micros();
    microsnow2 = microsnow2 - microsnow;
    if (microsnow2 > greatestDelay)
    {
        greatestDelay = microsnow2;
    }
}
