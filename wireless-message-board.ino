#include <stdlib.h>
#include <stdbool.h>
#include <SoftwareSerial.h>
#include <string.h>
#include "TextScroll.h"
#include "WifiCredentials.h"
#include "ESP8266Communication.h"

#define COMMAND_TIMEOUT_MILLIS 15000
#define STRING_TIMEOUT_MILLIS 5
#define BAUD_RATE_PC 9600
#define BAUD_RATE_ESP 9600
#define SERIAL_READ_DELAY 1
#define TEXT_SCROLL_BUFFER_SIZE 100
#define ESP_RX_PIN 16
#define ESP_TX_PIN 17

SoftwareSerial espSerial(ESP_RX_PIN, ESP_TX_PIN);

// Should always send webPageTop + currentMessage + webPageBottom
const String g_webPageTop = "<html><head><title>Arduino</title></head><meta><link rel=\"icon\" href=\"data:;base64,=\"></meta><h1>Current Message: ";
const String g_webPageBottom = "</h1><form>Message:<br><input type=\"text\" name=\"message\"><input type=\"submit\" value=\"Send\"></form></html>";

uint16_t g_baseLength = g_webPageTop.length() + g_webPageBottom.length();

String CONNECTION_START_TERM = "+IPD,";
String MESSAGE_START_TERM = ":GET /";
String MESSAGE_END_TERM = " HTTP/";
String MESSAGE_PREAMBLE = "?message=";

char g_textScrollBuffer[TEXT_SCROLL_BUFFER_SIZE];




void clearBuffer(void) {
  while (espSerial.available()) {
    espSerial.read();
    delay(SERIAL_READ_DELAY);
  }
}

void respondToRequest (void) {
  char newChar;
  String newMessage;
  bool wasNewMessage;
  uint16_t responseLength;
  String incomingText;
  uint16_t textPointer = 0;

  incomingText = espSerial.readString();
  Serial.print("\n\n\n\n\n");
  Serial.println(incomingText);
  int16_t conStartIndicator = incomingText.indexOf(CONNECTION_START_TERM);
  if (conStartIndicator < 0) {
    //Serial.println("not HTTP data")
    return;
  }
  conStartIndicator += CONNECTION_START_TERM.length();

  int16_t conEndIndicator = conStartIndicator;
  char conChar = incomingText[conEndIndicator];
  while ((conChar >= '0') && (conChar <= '9')) {
    conChar = incomingText[++conEndIndicator];
  }
  
  String connection = incomingText.substring(conStartIndicator, conEndIndicator);
  
  int16_t mesStartIndicator = incomingText.indexOf(MESSAGE_START_TERM);
  if (mesStartIndicator < 0) {
    return;
  }
  mesStartIndicator += MESSAGE_START_TERM.length() + MESSAGE_PREAMBLE.length();

  int16_t mesEndIndicator = incomingText.indexOf(MESSAGE_END_TERM);
  if (mesEndIndicator < 0) {
    return;
  }

  if (mesStartIndicator < mesEndIndicator) {
    for (uint16_t index = mesStartIndicator; index < mesEndIndicator; index++) {
      char contender = incomingText[index];
      g_textScrollBuffer[index - mesStartIndicator] = contender == '+' ? ' ' : contender; // check for overrun
    }
    g_textScrollBuffer[mesEndIndicator - mesStartIndicator] = 0;
  }
  
  updateMessage();
  
  responseLength = g_baseLength + strlen(g_textScrollBuffer);

  espSerial.print("AT+CIPSEND=");
  espSerial.print(connection);
  espSerial.print(',');
  espSerial.print(responseLength);
  espSerial.print("\r\n");

  while (!espSerial.available());
  clearBuffer();

  espSerial.print(g_webPageTop);
  espSerial.print(g_textScrollBuffer);
  espSerial.print(g_webPageBottom);

  delay(100);
  clearBuffer();

  espSerial.print("AT+CIPCLOSE=");
  espSerial.print(connection);
  espSerial.print("\r\n");
  
  while (!espSerial.available());
  clearBuffer();
}

void setup(){
  Serial.begin(BAUD_RATE_PC);
  Serial.setTimeout(STRING_TIMEOUT_MILLIS);
  espSerial.begin(BAUD_RATE_ESP);
  espSerial.setTimeout(STRING_TIMEOUT_MILLIS);
  
  initESP8266(espSerial, espSerial, 256);
  
  sendCommand("AT+CWMODE=3");
  Serial.println(espSerial.readString());
  delay(1000);
  Serial.println("Access point and station active!");

  String connectionCommand = "AT+CWJAP=\"" + WIFI_SSID + "\",\"" + WIFI_PASSPHRASE + "\"";
  sendCommand(connectionCommand);
  delay(15000);
  Serial.println(espSerial.readString());

  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  
  Serial.println("Connection info:");
  sendCommand("AT+CIPSTA?");
  Serial.println(espSerial.readString());
  
  sendCommand("AT+CIPMUX=1");
  Serial.println(espSerial.readString());

  Serial.println("Multi connection mode!");

  sendCommand("AT+CIPSERVER=1,80");
  Serial.println(espSerial.readString());

  Serial.println("Started server!");
  
  initTextScroll(g_textScrollBuffer, TEXT_SCROLL_BUFFER_SIZE);
  String temp = "Ready!";
  temp.toCharArray(g_textScrollBuffer, TEXT_SCROLL_BUFFER_SIZE);
  updateMessage();
}


uint16_t count = 0;
void loop(){
  if (count++ >= 150) {
    digitalWrite(9, LOW);
    stepDisplayPartial();
    count = 0;
    digitalWrite(9, HIGH);
  }
  flashDisplay();
  if (espSerial.available()) {
    respondToRequest();
  }
}

