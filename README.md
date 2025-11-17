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
