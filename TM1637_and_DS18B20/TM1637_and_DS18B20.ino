/*
 * Author: R McCleery < 

 * This Sketch uses ezTime library to create an NTP Local Time Clock
 * Time will adjust automatically if your zone supports Daylight Time
 * 
 * Sketch configured for ESP32 or ESP8266 with TM1637 4 digit display
 * 
 * The default NTP resync period of ezTime is 30 minutes
 * If PRINT enabled
 * It prints a simple "Local Time" for seconds 1 to 9 then a detailed time print every 10 seconds
 * 
 * 
 * Provide official timezone names
 * https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
 * 
 */

#define PRINT                      //Comment out for no Serial monitor prints

#include <ezTime.h>                //Library to use our system clock for NTP time
#include "TM1637.h"                //4 digit 7 seg display
#include <OneWire.h>
#include <DallasTemperature.h>

#if defined (ESP32)               //Lets the compiler work with
    #include <WiFi.h>             //ESP32
    #define TM_CLK        23
    #define TM_DIO        22
    #define ONE_WIRE_BUS  21
    #define LED           2       //Built-in LED
    #define ON            true
    #define OFF           false
#elif defined (ESP8266)           //or
    #include <ESP8266WiFi.h>      //ESP8266 without changing any script
    #define TM_CLK        14
    #define TM_DIO        12
    #define ONE_WIRE_BUS  13
    #define LED           2       //Built-in LED
    #define ON            false   //ESP8266 built-in LED is Active LOW
    #define OFF           true
#endif

TM1637 TM;                            //TM1637 Display object
Timezone myTZ;                        //Creates our Timezone object to get "Local Time"
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);  //Pass our oneWire reference to Dallas Temperature.

#define TM_DIGITS 4

#define BUTTON 0                                      //This is the FLASH/BOOT button on an ESP. You can use 

const char* myZone PROGMEM   = "Pacific/Auckland";    //Zone list here < https://en.wikipedia.org/wiki/List_of_tz_database_time_zones >

const char* ssid  PROGMEM    = "MillFlat_El_Rancho";
const char* password PROGMEM = "140824500925";

#define HALF_SEC  500UL                      //Creates the blinking colon IE 500mS ON > 500mS OFF
#define DB        50UL                       //Button de-bounce timing

bool blink = false, ledState = OFF,
    buttonPressed = false, pressed, 
    lastPress = true, display = true;
uint8_t lastSec, lastMin, lastHour;
int lastTempC;
volatile uint32_t blinkStart, currentMillis, dbMillis;

#ifdef PRINT
  void getTheTime(bool simple){
      if(simple){
          Serial.printf("Local Time  %02u:%02u:%02u\n", myTZ.hour(), myTZ.minute(), myTZ.second());
      }else{
          Serial.println("\n" + myTZ.dateTime() + "\n");
      }
      return;
  }
#endif

void setup() {
  #ifdef PRINT
    Serial.begin(115200);while(!Serial){;}
    Serial.print(" Conecting to WiFi ");
  #endif
  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  TM.begin(TM_CLK, TM_DIO, TM_DIGITS);       //  Starts the TM1637 display "TM.begin(clockPin, dataPin, #digits);""
  TM.displayClear();
  TM.setBrightness(4);
  sensors.begin();                            // Starts the DallasTemperature OneWire Sensors
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  delay(100);
  while(WiFi.status() != WL_CONNECTED){
    digitalWrite(LED, !digitalRead(LED));
    #ifdef PRINT  
      Serial.print(". ");
    #endif
      delay(300);
  }
  digitalWrite(LED, ON);
  #ifdef PRINT
    Serial.println("\n Now Conected to WiFi and getting Time Update\n");
  #endif
  waitForSync();                                                              //Default method for NTP sync every 30min
  myTZ.setLocation(myZone);                       //Sets our Timezone object see < https://en.wikipedia.org/wiki/List_of_tz_database_time_zones >
  lastSec = myTZ.second();
  lastMin = myTZ.minute() -1;                                                 // -1 so we update TM display on first loop
}

void loop() {
    
  currentMillis = millis();
  
  events();                                                 /* Needed by ezTime for NTP updates */

/*************************************************************************************************
*********************  Button to toggle TEMP / CLOCK  ********************************************
*************************************************************************************************/

  pressed = digitalRead(BUTTON);
    if(pressed != lastPress) {
        dbMillis = currentMillis;
        buttonPressed = true;
    }
    if(currentMillis - dbMillis >= DB) {
      if(buttonPressed) {
        if(pressed == LOW)
          {
            display = !display;
          }
      }
      buttonPressed = false; 
    }
/*************************************************************************************************
****************************************  Button END  ********************************************
*************************************************************************************************/
  
  if(myTZ.second() != lastSec) {                          /* Using One Second to do updates */
    lastSec = myTZ.second();
    digitalWrite(LED, ON);                                /* blinking LED IE 500mS ON > 500mS OFF */
    blink = true;
    blinkStart = currentMillis;
      if(display){
          //NTP Clock
          TM.displayTime(myTZ.hour(), myTZ.minute(), true);
          digitalWrite(LED, ON);
            if(myTZ.minute() != lastMin) {                          /* One Minute Clock Update */
              lastMin = myTZ.minute();
              TM.displayTime(myTZ.hour(), myTZ.minute(), true);     
            }
          #ifdef PRINT
            if(lastSec == 0 || lastSec == 10 || lastSec == 20 || 
              lastSec == 30 || lastSec == 40 || lastSec == 50) {
                getTheTime(0);
            } else {
                getTheTime(1);
            }
          #endif
        } else {                                                   /* Temperature with DS18B20 sensor */
          sensors.requestTemperatures(); // Send the command to get temperatures
          float temp_C = sensors.getTempCByIndex(0);
          int8_t tempC= ((float)round(temp_C));
          #ifdef PRINT
            Serial.print("Temperature for the device 1 (index 0) is: ");
            Serial.print(temp_C, 1); Serial.println(" ℃");
          #endif
          // Check if reading was successful
            if (tempC != DEVICE_DISCONNECTED_C) {
                if(tempC != lastTempC) {
                  TM.displayCelsius(tempC, false);
                }
            } else {
              #ifdef PRINT
                Serial.println("Error: Could not read temperature data");
              #endif
            }
            lastTempC = tempC;
        }
  }

  if(currentMillis - blinkStart >= HALF_SEC && blink) {
    if(display) {                                                 /* Half Second Colon Blink */
      TM.displayTime(myTZ.hour(), myTZ.minute(), false);
    }
    digitalWrite(LED, OFF);                                       /* blinking LED IE 500mS ON > 500mS OFF */
    blink = false;
  }
  lastPress = pressed;
}