## NTP Time and Temperature using a TM1637 4 digit LED display
### Test Configuration
* Arduino IDE v2.3.2
* ESP32 Arduino library v2.0.16
* ESP8266 Arduino library v3.1.2
* TM1637 4 digit display
* DS18B20 sensor complete with a PCB 3pin module interface

### Libraries
1. ezTime < https://github.com/ropg/ezTime >
2. TM1637_RT < https://github.com/RobTillaart/TM1637_RT >
3. DallasTemperature < https://github.com/milesburton/Arduino-Temperature-Control-Library >
4. OneWire < https://github.com/PaulStoffregen/OneWire >

### Sketch Functionality
* Sketch is configured to use the FLASH/BOOT button of your ESP32/8266 to switch between Time and Temperature (℃) display.
  * NOTE: **`On change the display will clear and there may be a short delay till the second roll's and the display updates`**
* Configure your timezone to have correct local time with auto daylight savings time correction
  ```
  const char* myZone PROGMEM   = "Pacific/Auckland";    //Zone list here < https://en.wikipedia.org/wiki/List_of_tz_database_time_zones >
  ```
* Print #define option to enable Serial Monitor output
  ```
  #define PRINT   //Comment out for no Serial output
  ```
* Sketch will auto detect your ESP32 / ESP8266. If you change any pins make sure you consider the "Strapping Pin's" boot state
* Onboard LED blinks is in sync with second change, 500mS ON > 500mS OFF
* TM1637 Time display also blinks the colon in sync with second change, 500mS ON > 500mS OFF

<br>

### Wiring
![Wiring for ESp32 and ESP8266](https://github.com/macca448/TM1637_DS18B20/blob/main/TM1637_and_DS18B20/assets/esp_tm1637_ds18b20.png)

