#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "wifiSettings.h"

#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// define schaffbar states
#define RED 0
#define YELLOW 1
#define RED_YELLOW 2
#define GREEN 3

const String SERVER_URL = "http://schaffbar-bb.de/wp-json/ampel/v1/status?status=";
const String RED_TEXT = "red";
const String RED_YELLOW_TEXT = "redyellow";
const String GREEN_TEXT = "green";
const String YELLOW_TEXT = "yellow";

#define CLOSE_BUTTON_PIN 1
#define YELLOW_BUTTON_PIN 2
#define GREEN_BUTTON_PIN 3
#define CLOSE_LED_PIN 21
#define YELLOW_LED_PIN 20
#define GREEN_LED_PIN 10

int buttonStateRED = LOW;  // variable for reading the pushbutton status
int buttonStateYELLOW = LOW;  // variable for reading the pushbutton status
int buttonStateGREEN = LOW;  // variable for reading the pushbutton status
int status = RED;  // variable for reading the pushbutton status

WiFiClient wifiClient;
HTTPClient http;
String wifiStatus = "";

void setup() {
  Serial.begin(9600);

  //Buttons
  pinMode(CLOSE_BUTTON_PIN, INPUT);
  pinMode(YELLOW_BUTTON_PIN, INPUT);
  pinMode(GREEN_BUTTON_PIN, INPUT);

  //LEDs
  pinMode(CLOSE_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  delay(250); // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
  //display.setContrast (0); // dim display
  display.display();

  // Clear the buffer.
  printMessageToDisplay(1, "Init ....");

  connectToWifi();

  // Initial state = RED
  digitalWrite(CLOSE_LED_PIN, HIGH);
  updateStatus(RED);
}

void loop() {
  buttonStateRED = digitalRead(CLOSE_BUTTON_PIN);
  if (buttonStateRED == HIGH) {
    // turn LED on:
    digitalWrite(CLOSE_LED_PIN, HIGH);
    updateStatus(RED);
    // turn others off
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
  } 

  buttonStateYELLOW = digitalRead(YELLOW_BUTTON_PIN);
  if (buttonStateYELLOW == HIGH) {
    //TODO: set LEDs correct
    
    // turn LED on:
    digitalWrite(YELLOW_LED_PIN, HIGH);
    updateStatus(YELLOW);
    // turn others off
    digitalWrite(CLOSE_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
  } 

  buttonStateGREEN = digitalRead(GREEN_BUTTON_PIN);
  if (buttonStateGREEN == HIGH) {
    // turn LED on:
    digitalWrite(GREEN_LED_PIN, HIGH);
    updateStatus(GREEN);
    // turn others off
    digitalWrite(CLOSE_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
  }

  delay(100);

  //check WIFI
  if (WiFi.status() != WL_CONNECTED) {
     connectToWifi();
  }

}

void updateStatus(int requestedStatus){
  String message = "Request status change to: ";
  switch (requestedStatus){
    case RED:
      message.concat(String(RED_TEXT));
      break;
    case GREEN:
      message.concat(String(GREEN_TEXT));
      break;
    case YELLOW:
      message.concat(String(YELLOW_TEXT));
      break;
    case RED_YELLOW:
      message.concat(String(RED_YELLOW_TEXT));
      break;
  }
  
  printMessageToDisplay(2, message);
  
  message = sendStatusToWebServer(requestedStatus);
  status = requestedStatus;
 
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

String sendStatusToWebServer(int status) {
  String returnMessage = "";
  String fullServerURL = "";
  JsonDocument doc;

  switch (status){
    case RED:
      fullServerURL = SERVER_URL + RED_TEXT;
      break;
    case GREEN: 
      fullServerURL = SERVER_URL + GREEN_TEXT;
      break;
    case YELLOW:
      fullServerURL = SERVER_URL + YELLOW_TEXT;
      break;
    case RED_YELLOW:
      fullServerURL = SERVER_URL + RED_YELLOW_TEXT;
      break;
  }

  Serial.print("Connect to: ");
  Serial.println(fullServerURL);
  http.begin(wifiClient, fullServerURL.c_str());

  http.addHeader("x-api-key",apiKey);
  int httpResponseCode = http.POST("");
  
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);

    //parse the payload
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      Serial.println("Payload parsed sucessful");
      const char* parsedMessage = doc["message"];
      returnMessage = String(parsedMessage);
      Serial.print("Parsed message: ");
      Serial.println(returnMessage);
    } else {
      Serial.println("Parse payload failed");
      returnMessage = "Parse payload failed";
    }

  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    returnMessage = "Failed HTTP call: " + String(httpResponseCode);
  }
  // Free resources
  http.end();



  return returnMessage;
}