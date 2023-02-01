/*Performing FSK modulation utilizing a DAC on a teensy 4.1
 * Author: Scott Mayberry
 */
#include <stdint.h>
#include "ACOMM4.h"

#include "SdFat.h"

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
ACOMM4 acomm(40);
void setup()
{
    delay(100);
    Serial.begin(115200);
    while (!Serial)
    {
    }
    Serial.println("ACOMM started");

    delay(500);
    // Initialize the SD.
    if (!sd.begin(SD_CONFIG))
    {
        sd.initErrorHalt(&Serial);
        return;
    }
    // Create the file.
    if (!file.open("data.csv", FILE_READ))
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
        addSample(sample >> 4, sampleNumberCounter);
        sampleNumberCounter++;
    }
    file.close();
    Serial.println("Done");
}

void loop()
{
    ;
}

void addSample(int sample, int sampleNumberCounter)
{
    acomm.addSample(sample, sampleNumberCounter);
    if (acomm.isMessageReady())
    {
        acomm.printMessages();
        delay(10);
    }
}
