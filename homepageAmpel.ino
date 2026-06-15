#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
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

#define RED_BUTTON_PIN 21
#define YELLOW_BUTTON_PIN 20
#define GREEN_BUTTON_PIN 10
#define RED_LED_PIN 1
#define YELLOW_LED_PIN 2
#define GREEN_LED_PIN 3

int buttonStateRED = LOW;  // variable for reading the pushbutton status
int buttonStateYELLOW = LOW;  // variable for reading the pushbutton status
int buttonStateGREEN = LOW;  // variable for reading the pushbutton status
int status = RED;  // variable for reading the pushbutton status

WiFiClient wifiClient;
HTTPClient http;
String wifiStatus = "";


void setup() {
  Serial.begin(115200);
  
  delay(250); // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
  //display.setContrast (0); // dim display
  display.display();
  printMessageToDisplay(1, "Init ....");
  connectToWifi();

  //Buttons
  pinMode(RED_BUTTON_PIN, INPUT);
  pinMode(YELLOW_BUTTON_PIN, INPUT);
  pinMode(GREEN_BUTTON_PIN, INPUT);

  //LEDs
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  // Initial state = RED
  digitalWrite(RED_LED_PIN, HIGH);
  updateStatus(RED);
}

void loop() {
  buttonStateRED = digitalRead(RED_BUTTON_PIN);
  if (buttonStateRED == HIGH) {
    // turn LED on:
    digitalWrite(RED_LED_PIN, HIGH);
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
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
  } 

  buttonStateGREEN = digitalRead(GREEN_BUTTON_PIN);
  if (buttonStateGREEN == HIGH) {
    // turn LED on:
    digitalWrite(GREEN_LED_PIN, HIGH);
    updateStatus(GREEN);
    // turn others off
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
  }

  delay(100);

  //check WIFI
  if (WiFi.status() != WL_CONNECTED) {
     connectToWifi();
  }

}

void updateStatus(int requestedStatus){
  String message = "Anfrage Status nach: ";
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
  Serial.println(wifiStatus);
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

String convertStatusToString(int status){
  String statusString = "Unbekannt";
  switch (status) {
    case WL_STOPPED:
      statusString = "Gestoppt";
      break;
    case WL_IDLE_STATUS:
      statusString = "Idle";
      break;
    case WL_NO_SSID_AVAIL:
      statusString = "Keine SSID gefunden";
      break;
    case WL_SCAN_COMPLETED:
      statusString = "Scan fertig";
      break;
    case WL_CONNECTED:
      statusString = "Verbunden";
      break;
    case WL_CONNECT_FAILED:
      statusString = "Fehler";
      break;
    case WL_CONNECTION_LOST:
      statusString = "Verbindung verloren";
      break;
    case WL_DISCONNECTED:
      statusString = "Getrennt";
      break;
  }
  return statusString;
}


void connectToWifi(){
  int status = 0;
  if (status != WL_CONNECTED){
    // Attempt to connect to Wifi network
    Serial.print("Attempting to connect to Wifi SSID: >");
    Serial.print(wifiSsid);
    Serial.println("<");
    WiFi.begin(wifiSsid, wifiPassword);
    status = WiFi.status();

    while (status != WL_CONNECTED) {
      // failed, retry
      wifiStatus = "Wifi: " + convertStatusToString(status);
      printMessageToDisplay(1,"Retry.");
      delay(5000);
      status = WiFi.status();
      ScanWiFi();
    }
    Serial.print("You're connected to the wifi network. Status: ");
    Serial.println(WiFi.status());
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    wifiStatus = "Wifi: " + convertStatusToString(status);
  }
}


void ScanWiFi() {
  Serial.println("Scan start");
  // WiFi.scanNetworks will return the number of networks found.
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.printf("%2d", i + 1);
      Serial.print(" | ");
      Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
      Serial.print(" | ");
      Serial.printf("%4" PRIi32, WiFi.RSSI(i));
      Serial.print(" | ");
      Serial.printf("%2" PRIi32, WiFi.channel(i));
      Serial.print(" | ");
      switch (WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN:            Serial.print("open"); break;
        case WIFI_AUTH_WEP:             Serial.print("WEP"); break;
        case WIFI_AUTH_WPA_PSK:         Serial.print("WPA"); break;
        case WIFI_AUTH_WPA2_PSK:        Serial.print("WPA2"); break;
        case WIFI_AUTH_WPA_WPA2_PSK:    Serial.print("WPA+WPA2"); break;
        case WIFI_AUTH_WPA2_ENTERPRISE: Serial.print("WPA2-EAP"); break;
        case WIFI_AUTH_WPA3_PSK:        Serial.print("WPA3"); break;
        case WIFI_AUTH_WPA2_WPA3_PSK:   Serial.print("WPA2+WPA3"); break;
        case WIFI_AUTH_WAPI_PSK:        Serial.print("WAPI"); break;
        default:                        Serial.print("unknown");
      }
      Serial.println();
      delay(10);
    }
  }

  // Delete the scan result to free memory for code below.
  WiFi.scanDelete();
  Serial.println("-------------------------------------");
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
      const char* parsedStatus = doc["status"];
      returnMessage = String(parsedMessage) + " -> " + String (parsedStatus);
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