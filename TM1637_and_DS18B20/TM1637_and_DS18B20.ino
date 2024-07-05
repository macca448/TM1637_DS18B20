/*
 * Author: R McCleery < https://github.com/macca448/TM1637_DS18B20 >
 * Email : macca448@gmail.com
 * Date  : 5th July 2004
 *
 *
 * Official timezone names list < https://en.wikipedia.org/wiki/List_of_tz_database_time_zones >
 * 
 * GPL-3.0 license (GNU) GENERAL PUBLIC LICENSE - Version 3, 29 June 2007 - <https://fsf.org/>
 * Copyright (c) 2016 by R McCleery'. 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
 * documentation files (the "Software"), to deal in the Software without restriction, including without 
 * limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:    
 * The above copyright ('as annotated') notice and this permission notice shall be included in all copies 
 * or substantial portions of the Software and where the software use is visible to an end-user.  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT 
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  
 * TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
 * OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 */

//#define PRINT                    //Comment out for no Serial monitor prints

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

#define HALF_SEC  10                         //Creates the blinking (10 x DB) 500mS ON > 500mS OFF
#define DB        50UL                       //Button de-bounce timing

bool      blink = false, buttonPressed = false, 
          pressed, lastPress = true, display = true;    
uint8_t   lastSec, lastMin, lastHour, blinkCount = 0;
int8_t    lastTempC;
uint32_t  dbMillis;

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

void doTemp(void) {
  sensors.requestTemperatures(); // Send the command to get temperatures
  float temp_C = sensors.getTempCByIndex(0);
  int8_t tempC = ((float)round(temp_C));
  
  #ifdef PRINT
    Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.print(temp_C, 1); Serial.println(" ℃");
  #endif
    if (tempC != DEVICE_DISCONNECTED_C) {       /* Check if reading was successful */
        if(tempC != lastTempC) {
          TM.displayCelsius(tempC, false);
        }
    } else {
      #ifdef PRINT
        Serial.println("Error: Could not read temperature data");
      #endif
    }
    lastTempC = tempC;
  return;
}

void doTime(uint8_t hour, uint8_t minute) {
  TM.displayTime(hour, minute, true);
  #ifdef PRINT
    if(lastSec == 0 || lastSec == 10 || lastSec == 20 || 
      lastSec == 30 || lastSec == 40 || lastSec == 50) {
        getTheTime(0);
    } else {
        getTheTime(1);
    }
  #endif
  return;
}

void setup() {
  #ifdef PRINT
    Serial.begin(115200);
    while(!Serial){;}
    Serial.print(" Conecting to WiFi ");
  #endif
  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT);
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
  
  events();                                                 /* Needed by ezTime for NTP updates */
  uint8_t hour = myTZ.hour(),
        minute = myTZ.minute(),
        second = myTZ.second();


/*************************************************************************************************
*************************  Button to toggle TEMP / CLOCK  ****************************************
*************************************************************************************************/

  pressed = digitalRead(BUTTON);
    if(pressed != lastPress) {
        dbMillis = millis();
        buttonPressed = true;
    }
    
    if(millis() - dbMillis >= DB) {
      blinkCount++;
      dbMillis = millis();
      
      if(buttonPressed) {
        if(pressed == LOW) {
          display = !display;
          lastTempC = -10;
          TM.displayClear();
        }
        buttonPressed = false;
      }
      
      if(blinkCount >= HALF_SEC && blink){
        if(display) {                                                 /* Half Second Colon Blink */
          TM.displayTime(hour, minute, false);
          blink = false;
        }
        digitalWrite(LED, OFF);                                       /* blinking LED IE 500mS ON > 500mS OFF */
      }
    }
/***************************************↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑********************************************/
/***************************************|  Button END |********************************************/
/***************************************↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓********************************************/
  
  if(second != lastSec) {                          /* Using One Second to do updates */
    digitalWrite(LED, ON);                         /* blinking LED IE 500mS ON > 500mS OFF */
    blink = true;
    blinkCount = 0;
      if(display) {                                /* NTP Clock */
          doTime(hour, minute);
        } else {                                   /* Temperature with DS18B20 sensor */
          doTemp();
        }
        lastSec = second;
  }
  lastPress = pressed;
}
