#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

hd44780_I2Cexp lcd;

#define LDR_PIN 34

const char* ssid     = "xxxxxxxx";
const char* password = "yyyyyyyy";

const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 0;

const unsigned long LUX_SCREEN_TIME  = 6000;
const unsigned long TIME_SCREEN_TIME = 500;
float filteredADC = 0;

bool showLux = true;
unsigned long lastScreenSwitch = 0;

byte sunIcon[8] =
{
    B00100,
    B10101,
    B01110,
    B11111,
    B01110,
    B10101,
    B00100,
    B00000
};

void syncTime()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting...");

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Syncing Time");

    configTime(
        gmtOffset_sec,
        daylightOffset_sec,
        "pool.ntp.org",
        "time.nist.gov"
    );

    struct tm timeinfo;

    while (!getLocalTime(&timeinfo))
    {
        delay(500);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RTC Synced");
    delay(1500);

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

void setup()
{
    Serial.begin(115200);

    analogReadResolution(12);

    filteredADC = analogRead(LDR_PIN);

    lcd.begin(16, 2);

    lcd.createChar(0, sunIcon);

    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("LUX METER");

    delay(1000);

    syncTime();

    lcd.clear();
}

void loop()
{
    // Average 20 samples
    long totalADC = 0;

    for(int i = 0; i < 20; i++)
    {
        totalADC += analogRead(LDR_PIN);
        delay(2);
    }

    int rawADC = totalADC / 20;

    // Moving Average Filter
    filteredADC = (0.9 * filteredADC) + (0.1 * rawADC);

    int adcValue = (int)filteredADC;

    // Practical indoor lux estimate
    float lux = adcValue * 0.0245;
    if (lux < 0)
        lux = 0;

    if (lux > 3000)
        lux = 3000;

    struct tm timeinfo;
    getLocalTime(&timeinfo);

    unsigned long currentInterval =
        showLux ? LUX_SCREEN_TIME : TIME_SCREEN_TIME;

    if (millis() - lastScreenSwitch >= currentInterval)
    {
        showLux = !showLux;
        lastScreenSwitch = millis();

        lcd.clear();
    }

    // Header Line
    lcd.setCursor(2, 0);
    lcd.write((uint8_t)0);
    lcd.print(" LUX METER");

    // Clear second line
    lcd.setCursor(0, 1);
    lcd.print("                ");

    if (showLux)
    {
        lcd.setCursor(4, 1);

        lcd.print(lux);
        lcd.print(" Lx");
    }
    else
    {
        char timeBuffer[12];

        strftime(
            timeBuffer,
            sizeof(timeBuffer),
            "%I:%M %p",
            &timeinfo);

        lcd.setCursor(4, 1);
        lcd.print(timeBuffer);
    }

    // Serial Debug
    Serial.print("ADC: ");
    Serial.print(adcValue);

    Serial.print("  Lux: ");
    Serial.print(lux);

    Serial.print("  Time: ");

    char serialTime[12];
    strftime(
        serialTime,
        sizeof(serialTime),
        "%I:%M %p",
        &timeinfo);

    Serial.println(serialTime);

    delay(200);
}