/*
 * -----------------------------------------------------------------------------------------------
 * 
 * Almanac for specific Latitude and Longitudes
 * Operates independantly not needing an Internet connection
 * 
 * Bob Smith https://github.com/bethanysciences/almanac
 *  
 * Written and compiled under version 1.8.12+ of the Arduino IDE using the 
 * AVRISP mkII programmer for Arduino for Nano 33 IoT and Arduino Nano 33 
 * BLE based microprocessors
 * 
 * This program distributed WITHOUT ANY WARRANTY; without even the implied 
 * warranty of MERCHANTIBILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  
 * Predictions generated by this program should NOT be used for navigation.
 * No accuracy or warranty is given or implied for these predictions.
 * 
 * 
 * 
 * * * * * KEY FUNCTIONS USED * * * * 
 * -----------------------------------------------------------------------------------------------
 * Tide events calculated using Luke Miller's library of NOAA harmonic data 
 * https://github.com/millerlp/Tide_calculator derived from David Flater's
 * XTide application https://flaterco.com/xtide/xtide.html. Futher logic is
 * derived from rabbitcreek's https://github.com/rabbitcreek/tinytideclock
 * 
 * Sun events calculated using SunEvents library 
 * https://github.com/bethanysciences/SunEvents outputting sunrises and sets
 * to a RTClib DateTime object, a derivative of DM Kishi's library 
 * https://github.com/dmkishi/Dusk2Dawn, based on a port of NOAA's Solar 
 * Calculator https://www.esrl.noaa.gov/gmd/grad/solcalc/
 * 
 * 
 * * * * * * HARDWARE USED * * * * 
 * -----------------------------------------------------------------------------------------------
 * 
 * Microprocessors written for (others may work)
 * Arduino Nano 33 BLE Sense https://store.arduino.cc/usa/nano-33-ble-sense
 * Arduino Nano 33 BLE https://store.arduino.cc/usa/nano-33-ble
 * 
 * 
 * Time Maintenance
 * Adafruit's battery backed I2C Maxim DS3231 Real Time Clock (RTC) Module 
 * https://www.adafruit.com/product/3013 using Adafruit's 
 * library https://github.com/adafruit/RTClib a fork of JeeLab's library 
 * https://git.jeelabs.org/jcw/rtclib - installed from Arduino IDE as [RTClib]
 * 
 * 
 * Lightening Events
 * Sparkfun's Franklin AS3935 Lightning Detector Breakout Board V2
 * https://www.sparkfun.com/products/15441 using Sparkfun's library 
 * https://github.com/sparkfun/SparkFun_AS3935_Lightning_Detector_Arduino_Library
 *  - installed from Arduino IDE as [SparkFun AS3935]
 * 
 * distanceToStorm()           strike distance in kilometers x .621371 for miles
 * clearStatistics()           zero distance registers
 * resetSettings()             resets to defaults
 * setNoiseLevel(int)          read/set noise floor 1-7 (2 default)
 * readNoiseLevel()
 * maskDisturber(bool)         read/set mask disturber events (false default)
 * readMaskDisturber(bool)
 * setIndoorOutdoor(hex)       0xE, 0x12 (default) attunates for inside use
 * readIndoorOutdoor(hex)
 * watchdogThreshold(int)      read/set watchdog threshold 1-10 (2 default)
 * readWatchdogThreshold()
 * spikeRejection(int)         read/set spike reject 1-11 (2 default)
 * readSpikeRejection()
 * lightningThreshold(int)     read/set # strikes to trip interrupt 1,5,9, or 26
 * readLightningThreshold()
 * lightningEnergy(long)       strike energy
 * lightning.powerDown()       wake after power down resets internal resonators
 * lightning.wakeUp()          to default antenna resonance frequency (500kHz)
 *                             skewing built-up calibrations. Calibrate 
 *                             antenna before using this function
 * 
 *  *  * * * * * * MICE TYPE * * * * 
 * ---------------------------------------------------------------------------------------------
 * 
 * This application and project are open source using MIT License see 
 * license.txt.
 *  
 * See included library github directories for their respecive licenses
 *  
 *  
 *  
 *  
 *  
 *  
 * -----------------------------------------------------------------------------------------------
 */


// ---------------  SERIAL PRINT DEBUG SWITCH -------------- //
#define DEBUG false                                          // true setup and use serial print


// ------------  DS3231 REAL-TIME CLOCK  ------------------ //
#include <RTClib.h>                                         // version=1.8.0
// RTC_DS3231 rtc;                                          // instantiate RTC module
RTC_Millis rtc;                                             // instantiate soft RTC

#define GMT_OFFSET              -5                          // LST hours GMT offset
char ampm[2][3]                 = {"pm", "am"};             // AM/PM
#define USE_USLDT               true                        // US daylight savings?
char LOC[3][4]                  = {"LST",                   // Local Standard Time (LST)
                                   "LDT"};                  // Local Daylight Time (LDT)
char dayNames[7][4]             = {"Sun", "Mon", "Tue", 
                                   "Wed", "Thu", "Fri", "Sat"};
char monthNames[12][4]          = {"Jan", "Feb", "Mar", "Apr", 
                                   "May", "Jun", "Jul", "Aug",
                                   "Sep", "Oct", "Nov", "Dec"};


// ---------------  SUN EVENTS  --------------------------- //
#include <SunEvent.h>                                       // version=0.1.0
float LATITUDE                  = 38.5393;                  // Bethany Beach, DE, USA
float LONGITUDE                 = -75.0547;
char SUN_LOCATION[]             = "Bethany Beach, DE";
SunEvent BethanyBeach(LATITUDE, LONGITUDE, GMT_OFFSET);
DateTime nextrise, nextset;


// ---------------  TIDE CALCULATIONS  -------------------- //
#include "tides.h"
TideCalc myTideCalc;
DateTime adjHigh, adjLow;
char TIDE_LOCATION[]            = "Indian River, DE";


// -----------  SSD1351 OLED Displays  ------------ //
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <Fonts/FreeSans9pt7b.h>                   // 12, 18 24 pt   
#include <Fonts/FreeSans12pt7b.h>
#include "curve.h"
#define OLED_W          128
#define OLED_H          128
#define LINE_HEIGHT     9

#define BLACK           0x0000                  
#define RED             0xF800
#define ORANGE          0xFDCD
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define GREEN           0x07E0
#define CYAN            0x07FF
#define BLUE            0x001F
#define WHITE           0xFFFF

#define SCLK_PIN        13
#define MOSI_PIN        11
#define DC_PIN_0        7
#define DC_PIN_1        8
#define CS_PIN_0        9
#define CS_PIN_1        10
#define RST_PIN_0       2
#define RST_PIN_1       3

Adafruit_SSD1351 sunOled = Adafruit_SSD1351(OLED_W, OLED_H, &SPI, CS_PIN_0, DC_PIN_0, RST_PIN_0);
Adafruit_SSD1351 tideOled  = Adafruit_SSD1351(OLED_W, OLED_H, &SPI, CS_PIN_1, DC_PIN_1, RST_PIN_1);

bool isAM(const DateTime& test) {
    if (test.hour() > 12 | test.hour() == 0 | test.hour() == 12) return false;
    else return true;
}

bool isLDT(const DateTime& curLST) {
    if(!USE_USLDT) return false;            // location uses US daylight savings
    int y = curLST.year() - 2000;
    int x = (y + y/4 + 2) % 7;              // set boundary Sundays

    if(curLST.month() == 3 &&
       curLST.day() == (14 - x) &&
       curLST.hour() >= 2)
       return true;                         // time is Local Daylight Time (LDT)

    if(curLST.month() == 3 && 
       curLST.day() > (14 - x) || 
       curLST.month() > 3) 
       return true;                         // time is Local Daylight Time (LDT)

    if(curLST.month() == 11 && 
       curLST.day() == (7 - x) && 
       curLST.hour() >= 2) 
       return false;                        // time is Local Standard Time (LST)

    if(curLST.month() == 11 &&
       curLST.day() > (7 - x) || 
       curLST.month() > 11 || 
       curLST.month() < 3) 
       return false;                        // time is Local Standard Time (LST)
}


void setup() {
    asm(".global _printf_float");                               // printf renders floats

#if DEBUG
    Serial.begin(115200);
    while (!Serial);                                            // wait for serial port to open
    Serial.println("\nsetup() ------------------------------------------------------------");
#endif

//    if (! rtc.begin()) while (1);
//    if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    tideOled.begin();
    sunOled.begin();
    setDisplays();
    
    delay(1000);                                                // flush registers
}


void setDisplays(){
    sunOled.fillScreen(BLACK);
    sunOled.setRotation(2);
    sunOled.setTextColor(WHITE);
    sunOled.setFont(&FreeSans9pt7b);
    sunOled.setCursor(24, 15);
    sunOled.print("Almanac");
    
    sunOled.fillCircle(12, 40, 10, ORANGE);
    sunOled.fillRect(   2, 40, 27, 12, BLACK);
    
    sunOled.fillCircle(12, 60, 10, YELLOW);
    
    sunOled.fillCircle(12, 90, 10, RED);
    sunOled.fillRect(   2, 90, 27, 12, BLACK);
    
    sunOled.setFont();
    sunOled.setTextColor(WHITE, BLACK);
    sunOled.setCursor(4, 117);
    sunOled.print(SUN_LOCATION);
    delay(100);

    tideOled.setRotation(2);
    tideOled.fillScreen(BLACK);
    tideOled.drawBitmap(0, 0, curve, 128, 128, WHITE);

    delay(100);
}


void Tides() {
    float       INTERVAL                = 1 * 5 * 60L;                  // tide calc interval
    float       results;                                              // needed to print tide height
    DateTime    futureHigh;
    DateTime    futureLow;
    DateTime    future;
    uint8_t     slope;
    uint8_t     i                       = 0;
    uint8_t     zag                     = 0;
    bool        gate                    = 1;
    bool        hiLow;
    float       tidalDifference         = 0;
    bool        bing                    = 1;
    bool        futureLowGate           = 0;
    bool        futureHighGate          = 0;
    char        time_buf[40]            = "\0";
    char        curr_buf[40]            = "\0";
    char        lo_buf[40]              = "\0";
    char        hi_buf[40]              = "\0";
    uint16_t    line_height             = 20;
    uint16_t    time_line               = 60;
    uint16_t    curr_line               = 60;
    uint16_t    hi_line                 = 15;
    uint16_t    lo_line                 = 115;
    
    DateTime now = rtc.now();                                   // current time from RTC

    sprintf(time_buf, "%d:%02d%s", now.twelveHour(), now.minute(), ampm[isAM(now)]);
    tideOled.fillRect(0, time_line - line_height, 60, line_height, BLACK);
    tideOled.setFont(&FreeSans9pt7b);
    tideOled.setCursor(1, time_line);
    tideOled.print(time_buf);

#ifdef DEBUG      
    Serial.print("Time() ---- ");
    Serial.println(time_buf);
#endif
    
    DateTime adjnow(now.unixtime() - (isLDT(now) * 3600));      // convert to LST during LDT
    float pastResult = myTideCalc.currentTide(adjnow);

    sprintf(curr_buf, "%d.%d ft", (int)pastResult, (int)(pastResult*10)%10);
    tideOled.fillRect(75, curr_line - line_height, OLED_W - 75, line_height, BLACK);
    tideOled.setCursor(75, curr_line);
    tideOled.setFont(&FreeSans9pt7b);
    tideOled.print(curr_buf);

    while(bing){
        i++;
        DateTime future(adjnow.unixtime() + (i * INTERVAL));
        results = myTideCalc.currentTide(future);
        tidalDifference = results - pastResult;
        if (gate){
            if(tidalDifference < 0) slope = 0;
            else slope = 1;
            gate = 0;
        }
        if(tidalDifference > 0 && slope == 0) {
            futureLow = future;
            gate = 1;
            //bing = 0;
            futureLowGate = 1;
        }
        else if(tidalDifference < 0 && slope == 1){
            futureHigh = future;
            gate = 1;
            //bing = 0;
            futureHighGate = 1;
        }
        if(futureHighGate && futureLowGate) {
            float resultsHigh = myTideCalc.currentTide(futureHigh);
            float resultsLow  = myTideCalc.currentTide(futureLow);
            if(int(futureHigh.unixtime() - futureLow.unixtime()) < 0) hiLow = 1;
            if(int(futureHigh.unixtime() - futureLow.unixtime()) > 0) hiLow = 0;

            if (hiLow) {
                DateTime adjLow(futureLow.unixtime() + (isLDT(futureLow) * 3600));
                DateTime adjHigh(futureHigh.unixtime() + (isLDT(futureHigh) * 3600));

                sprintf(lo_buf, "%d:%02d%s", adjLow.twelveHour(),  adjLow.minute(),  ampm[isAM(adjLow)]);
                sprintf(hi_buf, "%d:%02d%s", adjHigh.twelveHour(), adjHigh.minute(), ampm[isAM(adjHigh)]);

                tideOled.fillRect(0, lo_line - line_height, 70, line_height, BLACK);
                tideOled.setCursor(2, lo_line);
                tideOled.print(lo_buf);

                tideOled.fillRect(50, hi_line - line_height, OLED_W - 50, line_height, BLACK);
                tideOled.setCursor(50, hi_line);
                tideOled.print(hi_buf);

                
#ifdef DEBUG      
                Serial.print("Tides() --- low ");
                Serial.print(lo_buf);
                Serial.print("  high ");
                Serial.print(hi_buf);
#endif
            }
            else {
                DateTime adjLow(futureLow.unixtime() + (isLDT(futureLow) * 3600));
                DateTime adjHigh(futureHigh.unixtime() + (isLDT(futureHigh) * 3600));

                sprintf(lo_buf, "%d:%02d%s", adjLow.twelveHour(),  adjLow.minute(),  ampm[isAM(adjLow)]);
                sprintf(hi_buf, "%d:%02d%s", adjHigh.twelveHour(), adjHigh.minute(), ampm[isAM(adjHigh)]);

                tideOled.fillRect(0, lo_line - 15, OLED_W, line_height * 2, BLACK);
                tideOled.setCursor(2, lo_line);
                tideOled.print(lo_buf);

                tideOled.fillRect(50, hi_line - 15, OLED_W, line_height * 2, BLACK);
                tideOled.setCursor(50, hi_line);
                tideOled.print(hi_buf);

#ifdef DEBUG      
                Serial.print("Tides() --- low ");
                Serial.print(lo_buf);
                Serial.print("  high ");
                Serial.print(hi_buf);
#endif
            }
            results = myTideCalc.currentTide(adjnow);
            gate = 1;
            bing = 0;
            futureHighGate = 0;
            futureLowGate  = 0;
        }
        pastResult = results;
    }
}


void Sun() {
    uint16_t    time_line               = 105;
    uint16_t    rise_line               = 40;   
    uint16_t    light_line              = 65;
    uint16_t    set_line                = 90;
    char        rise_buf[40]            = "\0";
    char        set_buf[40]             = "\0";
    char        light_buf[40]           = "\0";

    DateTime now = rtc.now();
    char time_buf[80];
    sprintf(time_buf, "%s %s %d %d:%02d%s %s", 
            dayNames[now.dayOfTheWeek()], monthNames[now.month()], now.day(),
            now.twelveHour(), now.minute(), ampm[isAM(now)], LOC[isLDT(now)]);

    sunOled.setFont();
    sunOled.setTextColor(WHITE, BLACK);
    sunOled.setCursor(2, time_line);
    sunOled.print(time_buf);

#ifdef DEBUG      
    Serial.print("Time() ---- ");
    Serial.println(time_buf);
#endif

    DateTime nextrise = BethanyBeach.sunrise(rtc.now());
    DateTime nextset  = BethanyBeach.sunset(rtc.now());
    TimeSpan sunlight = nextset.unixtime() - nextrise.unixtime();

    sprintf(rise_buf, "%d:%02d%s", nextrise.twelveHour(), nextrise.minute(), ampm[isAM(nextrise)]);
    sprintf(set_buf, "%d:%02d%s", nextset.twelveHour(), nextset.minute(), ampm[isAM(nextset)]);   
    sprintf(light_buf, "%dhr %dmin", sunlight.hours(), sunlight.minutes());

    sunOled.fillRect(32, rise_line - 20, OLED_W - 25, 25, BLACK);
    sunOled.setFont(&FreeSans12pt7b);
    sunOled.setCursor(32, rise_line);
    sunOled.print(rise_buf);

    sunOled.fillRect(25, light_line - 15, OLED_W - 25, 25, BLACK);
    sunOled.setFont(&FreeSans9pt7b);
    sunOled.setCursor(25, light_line);
    sunOled.print(light_buf);

    sunOled.fillRect(32, set_line - 20, OLED_W - 25, 25, BLACK);
    sunOled.setFont(&FreeSans12pt7b);
    sunOled.setCursor(32, set_line);
    sunOled.print(set_buf);

#ifdef DEBUG      
    Serial.print("Sun() ----- Rise ");
    Serial.print(rise_buf);
    Serial.print("  Set ");
    Serial.print(set_buf);
    Serial.print("  ");
    Serial.println(light_buf);
#endif

}

void loop() {
    Sun();
    Tides();

    Serial.println();
    delay(60 * 1000);  
}
