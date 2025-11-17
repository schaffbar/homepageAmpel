
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include "wifiSettings.h"

#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// define schaffbar states
#define CLOSED 0
#define PENDING 1
#define OPEN 2

#define CLOSE_BUTTON_PIN 1
#define PENDING_BUTTON_PIN 2
#define OPEN_BUTTON_PIN 3
#define CLOSE_LED_PIN 21
#define PENDING_LED_PIN 20
#define OPEN_LED_PIN 10

int buttonStateClosed = LOW;  // variable for reading the pushbutton status
int buttonStatePending = LOW;  // variable for reading the pushbutton status
int buttonStateOpen = LOW;  // variable for reading the pushbutton status
int status = CLOSED;  // variable for reading the pushbutton status

WiFiClient wifiClient;
String wifiStatus = "";

void setup() {
  Serial.begin(9600);

  //Buttons
  pinMode(CLOSE_BUTTON_PIN, INPUT);
  pinMode(PENDING_BUTTON_PIN, INPUT);
  pinMode(OPEN_BUTTON_PIN, INPUT);

  //LEDs
  pinMode(CLOSE_LED_PIN, OUTPUT);
  pinMode(PENDING_LED_PIN, OUTPUT);
  pinMode(OPEN_LED_PIN, OUTPUT);

  delay(250); // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
  //display.setContrast (0); // dim display
  display.display();

  // Clear the buffer.
  printMessageToDisplay(1, "Init ....");

  connectToWifi();

  // Initial state = CLOSED
  digitalWrite(CLOSE_LED_PIN, HIGH);
  updateStatus(CLOSED);
}

void loop() {
  buttonStateClosed = digitalRead(CLOSE_BUTTON_PIN);
  if (buttonStateClosed == HIGH) {
    // turn LED on:
    digitalWrite(CLOSE_LED_PIN, HIGH);
    updateStatus(CLOSED);
    // turn others off
    digitalWrite(PENDING_LED_PIN, LOW);
    digitalWrite(OPEN_LED_PIN, LOW);
  } 

  buttonStatePending = digitalRead(PENDING_BUTTON_PIN);
  if (buttonStatePending == HIGH) {
    // turn LED on:
    digitalWrite(PENDING_LED_PIN, HIGH);
    updateStatus(PENDING);
    // turn others off
    digitalWrite(CLOSE_LED_PIN, LOW);
    digitalWrite(OPEN_LED_PIN, LOW);
  } 

  buttonStateOpen = digitalRead(OPEN_BUTTON_PIN);
  if (buttonStateOpen == HIGH) {
    // turn LED on:
    digitalWrite(OPEN_LED_PIN, HIGH);
    updateStatus(OPEN);
    // turn others off
    digitalWrite(CLOSE_LED_PIN, LOW);
    digitalWrite(PENDING_LED_PIN, LOW);
  }

  delay(100);

  //check WIFI
  if (WiFi.status() != WL_CONNECTED) {
     connectToWifi();
  }

}

void updateStatus(int requestedStatus){
  String message = "Request status change to: ";
  message.concat(requestedStatus);
  
  printMessageToDisplay(2, message);
  
  switch (requestedStatus) {
    case CLOSED:
      message = "Schaffbar geschlossen";
      status = CLOSED;
      break;
    case PENDING:
      if (status == CLOSED) {
        message = "Schaffbar oeffnet bald";
      }
      if (status == OPEN) {
        message = "Schaffbar schliesst bald";
      }
      //status = PENDING;
      break;
    case OPEN:
      message = "Schaffbar offen";
      status = OPEN;
      break;
  }
 
  printMessageToDisplay(2, message);
}

void printMessageToDisplay(int line, String message) {
  Serial.println(message);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println(wifiStatus);
  display.setCursor(0, line*11);
  display.println(message);
  display.display();
}

void connectToWifi(){
  if (WiFi.status() != WL_CONNECTED){
    // Attempt to connect to Wifi network
    Serial.print("Attempting to connect to Wifi SSID: ");
    Serial.println(wifiSsid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid, wifiPassword);
    while (WiFi.status() != WL_CONNECTED) {
      // failed, retry
      printMessageToDisplay(1,"Wifi failed ! Retry.");
      delay(5000);
    }
    Serial.print("You're connected to the wifi network. Status: ");
    Serial.println(WiFi.status());
    wifiStatus = "Wifi connected. ";
    printMessageToDisplay(1,wifiStatus + WiFi.status());
  }
}
