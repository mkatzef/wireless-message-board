# wireless-message-board

A sketch for an Arduino to act as a basic web server, receiving text to display on an LED matrix. Uses an Arduino Nano, a Freetronics 32*16 LED display, and an ESP8266 (ESP-01) WiFi module.

## Getting Started

### Project Components
This project consists of the following files:  
* `ESP8266Communication.cpp` - A set of functions to interface with the ESP8266 module, with corresponding header file.
* `TextScroll.cpp` - A set of functions to display scrolling text on the LED matrix display, with corresponding header file.
* `WifiCredentialsTemplate.h`\* - A template for a required file to connect to a WiFi network.
* `wireless-message-board.ino` - The main Arduino sketch, serves webpage, displays response text (scrolling) on display.

For the main sketch to compile, all repository files must be present in the same directory. The contents of the included template file (marked \*) must be copied to an additional file (named `WifiCredentials.h`) in the same directory. The strings `WIFI_SSID` and `WIFI_PASSPHRASE` (of the new file) must be populated with the SSID and password of the network to which the ESP8266 must connect.

### 3rd-Party Libraries

The project sketch depends on the following 3rd-party library.

* `DMD` - "A library for driving the Freetronics 512 pixel dot matrix LED display "DMD"". Available [here](https://github.com/freetronics/DMD).

After this library is added to the local `Arduino/libraries` directory, the sketch `wireless-message-board.ino` may be programmed on an Arduino device in the usual method using the Arduino IDE.

### Circuit

The LED matrix used in this project is the Techbrands XC4622 (based on a Freetronics design) available and described [here](https://www.jaycar.com.au/white-led-dot-matrix-display-for-arduino/p/XC4622). 

The Baud rate of the ESP8266 was set to 9600 (with command `AT+CIOBAUD=9600`). This module was used with an adapter providing logic-level shifting to prevent damage.

The included sketch requires the following circuit to be constructed.

| Arduino Pin Number | Pin Label | XC4622 Pin Number | Pin Label | ESP8266 Adapter Pin Label |  
| --- | --- | --- | --- | -- |  
| - | +5V | - | Vcc\* | Vcc |  
| - | GND | 15 | - | GND |  
| 6 | D6 | 2 | A-D6 | - |  
| 7 | D7 | 4 | B-D7 | - |  
| 8 | D8 | 10 | SCLK-D9 | - |  
| 9 | D9 | 1 | OE-D9 | - |  
| 11 | D11 | 12 | DATA-D11 | - |  
| 13 | D13 | 8 | CLK-D13 | - |  
| 16 | A2 | - | - | TX |  
| 17 | A3 | - | - | RX |  

\* Either of the centre connectors on the back of the XC4622.

The ESP8266 adapter made four pins available in a breadboard-compatible format. These were the result of the following internal connections.

| ESP8266 Pin Label | ESP8266 Adapter Pin Label |  
| --- | --- |  
| Vcc | Vcc |  
| GND | GND |  
| UTXD | TX |  
| URXD | RX |  
| CH_PD | Vcc |  

Please note that the ESP8266 is not 5V tolerant and should not be connected directly to the Arduino device.

### Running

An Arduino device programmed with the project sketch begins a lengthy initialization process when powered on. This initialization includes connecting to the network described by `WifiCredentials.h`, and enabling a web server at port 80.

During the above initialization, the ESP8266's assigned IP address is printed to serial (at 9600 Baud). After initialization, the LED matrix display presents the text `Ready!`.

Once the devices are ready, a user may connect to the Arduino device through a web browser on another device connected to the same network. Simply browse to the IP address assigned to the Arduino.

The Arduino should serve a basic web page displaying the current message and a text box. The current message may be changed by entering a new message in the text box and clicking the "Send" button.

## To do

The included ESP8266 communication library is currently limited in functionality and performs no error checking. This occasionally leads to improper initialization. An overhaul is required.

The Arduino's IP address should replace the "Ready!" text after initialization. This would require extracting information from an ESP8266 response message - a feature that should be included in the ESP8266 communication library.

## Authors

* **Marc Katzef** - [mkatzef](https://github.com/mkatzef)

## Acknowledgements

* **Jaycar Electronics** - Part supplier and author of [this guide](https://www.jaycar.co.nz/diy-arduino-clock) showing how to interact with the LED matrix display.

* [**fuho**](https://github.com/fuho) - Author of [this helpful guide](https://room-15.github.io/blog/2015/03/26/esp8266-at-command-reference/) to the ESP8266 command set.
