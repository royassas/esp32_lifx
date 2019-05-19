# ESP32 LIFX

A Lifx emulator using an ESP32 and LED strips via FastLED

## Goal
Bild my own [Lifx](https://www.lifx.com/) bulb clones using an ESP32 with the possibility to do OTA firmware updates. The FastLED library allows to connect different LED strips to the ESP32. 

### Subgoals
- :heavy_check_mark: Connect to Wifi and get recognized by the Lifx app
- :heavy_check_mark: Turn light on/off
- :o: Change colors / color temperature
- :x: Add compatibilty for "Location" and "Group" functions
- :x: Add Multizone capabilites
- :x: Add Tile capabilites

## Development setup
I'm using a [Sparkfun ESP32 Thing](https://www.sparkfun.com/products/13907) and [Adafruit WS2801 RGB led strips] (https://www.adafruit.com/product/322). Code is uploaded via Arduino IDE and edited in VS Code with the Platform.io extension.

## Credit
Uses the following code/libraries:
- This project is forked from [ESP8266 Lifx by area3001](https://github.com/area3001/esp8266_lifx), which is based on the code of [Kayne Richens](https://github.com/kayno/arduinolifx) and is made possible by the efforts of the [ESP8266 Community Forum](https://github.com/esp8266) and their port of the [Arduino core for the ESP8266](https://github.com/esp8266/Arduino)
- [FastLED](http://fastled.io/) - MIT 2013 as of 19.05.2019
- [Esp32WifiManager](https://github.com/madhephaestus/Esp32WifiManager) - GNU GPL v3.0 as of 19.05.2019

##Contribution
Yes, please!

##License
To be decided...