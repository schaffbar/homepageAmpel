This project is about updating a Webpage from a physical device with some physical buttons in the real world.

# Idea 
1. First init the device and the OLED panel
2. Connect to Wifi
3. Set the initial satte to CLOSED
4. If a button is pressed, then
    1. A REST call to the homepage is made and if successful then
        1. The appropriate internal state is set
        2. The OLED display the new state
        3. The correct LED is powered

# Prerequisites 
* ESP32C3 Super mini
* OLED SH1106
* 3 Buttons
* 3 LED's
* some resitors

# Used ports
CLOSE_BUTTON_PIN 1

PENDING_BUTTON_PIN 2

OPEN_BUTTON_PIN 3

CLOSE_LED_PIN 21

PENDING_LED_PIN 20

OPEN_LED_PIN 10

# Setup
This is a Arduino Studio based application. 

* Start download the Arduino IDE from: https://docs.arduino.cc/software/ide-v2/tutorials/getting-started/ide-v2-downloading-and-installing/
* Follow the instructions from Expressiv: https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html and update the board libs.
* In the Arduino Lib Manager download the lib named:
    * Adafruit SH110X and all dependencies   
* Clone the repository
* Open Arduino Studio
* Select ESP32C3 Dev Module in the board manager
* Enable USB CDC on Boot
* Modify the wifiSettings.h
* Compile and upload the file to the ESP
* Have fun.
