/*
 * wireless-message-board.ino
 * 
 * A sketch for an Arduino Nano to act as a simple web server using an ESP8266
 * (ESP-01) WiFi module.
 * 
 * Serves a web page to any client on the local network who browses to the
 * ESP8266's IP address. This web page contains a text box to send the Arduino a
 * message that is then displayed on a connected LED matrix display.
 * 
 * Requires a valid SSID and password to be defined in a file named
 * WifiCredetials.h in the project directory. Please see the included template
 * WifiCredentialsTemplate.h.
 * 
 * Written by Marc Katzef
 */

#include <stdlib.h>
#include <stdbool.h>
#include <SoftwareSerial.h>
#include <string.h>
#include "TextScroll.h"
#include "WifiCredentials.h"
#include "ESP8266Communication.h"

#define STRING_TIMEOUT_MILLIS 5 // Serial readString timeout
#define BAUD_RATE_PC 9600
#define BAUD_RATE_ESP 9600
#define SERIAL_READ_DELAY 1 // Time to delay between serial read() calls, in milliseconds
#define TEXT_SCROLL_BUFFER_SIZE 100 // Maximum size of message to display, in chars
#define ESP_RX_PIN 16 // Arduino pin from which ESP8266 data is received (connected to ESP8266 Tx)
#define ESP_TX_PIN 17 // Arduino pin to which ESP8266 data is sent (connected to ESP8266 Rx)

SoftwareSerial espSerial(ESP_RX_PIN, ESP_TX_PIN);

// Should always send webPageTop + currentMessage + webPageBottom
const String g_webPageTop = "<html><head><title>Arduino</title></head><meta><link rel=\"icon\" href=\"data:;base64,=\"></meta><h1>Current Message: ";
const String g_webPageBottom = "</h1><form>Message:<br><input type=\"text\" name=\"message\"><input type=\"submit\" value=\"Send\"></form></html>";

uint16_t g_baseLength = g_webPageTop.length() + g_webPageBottom.length();

// Substrings to find a new display message in an HTTP message.
String CONNECTION_START_TERM = "+IPD,";
String MESSAGE_START_TERM = ":GET /";
String MESSAGE_END_TERM = " HTTP/";
String MESSAGE_PREAMBLE = "?message=";

// The current message to display on the LED matrix display.
char g_textScrollBuffer[TEXT_SCROLL_BUFFER_SIZE];


/*
 * Reads and discards data from the ESP8266 serial buffer until no more data is
 * available.
 */
void clearBuffer(void) {
  while (espSerial.available()) {
    espSerial.read();
    delay(SERIAL_READ_DELAY);
  }
}


/* 
 * Parses available data from ESP8266 for connection information and new message to
 * display. If a new message was present, updates scrolling message accordingly,
 * and confirms receipt by sending an updated web page to client.
 */
void respondToRequest (void) {
  char newChar;
  String newMessage;
  bool wasNewMessage;
  uint16_t responseLength;
  String incomingText;
  uint16_t textPointer = 0;

  incomingText = espSerial.readString(); // Retrieve entire data packet from ESP8266
  Serial.print("Received packet:");
  Serial.println(incomingText);

  // Identify start and end indices of connection number.
  // (assigned by ESP8266, should be in [0,4))
  int16_t conStartIndicator = incomingText.indexOf(CONNECTION_START_TERM);
  if (conStartIndicator < 0) { // Not an ESP8266 data packet, ignore
    return;
  }
  conStartIndicator += CONNECTION_START_TERM.length();

  int16_t conEndIndicator = conStartIndicator;
  char conChar = incomingText[conEndIndicator];
  while ((conChar >= '0') && (conChar <= '9')) { // Use variable to avoid indexing twice
    conChar = incomingText[++conEndIndicator];
  }

  // Extract connection number substring
  String connection = incomingText.substring(conStartIndicator, conEndIndicator);

  // Identify start and end indices of new message to display.
  int16_t mesStartIndicator = incomingText.indexOf(MESSAGE_START_TERM);
  if (mesStartIndicator < 0) { // Did not contain HTTP request.
    return;
  }
  mesStartIndicator += MESSAGE_START_TERM.length() + MESSAGE_PREAMBLE.length();

  int16_t mesEndIndicator = incomingText.indexOf(MESSAGE_END_TERM);
  if (mesEndIndicator < 0) { // Did not specify HTTP version.
    return;
  }

  // TODO: check for buffer overrun, convert escaped characters (denoted "%(ASCII in hex)")
  if (mesStartIndicator < mesEndIndicator) { // A message was given
    for (uint16_t index = mesStartIndicator; index < mesEndIndicator; index++) {
      char contender = incomingText[index]; // A character of the display message, or a '+' to represent a space.
      g_textScrollBuffer[index - mesStartIndicator] = contender == '+' ? ' ' : contender;
    }
    g_textScrollBuffer[mesEndIndicator - mesStartIndicator] = 0;
  }

  // Signal to TextScroll that a new message has been written to shared buffer
  updateMessage();

  // Send updated web page to client.
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


/* 
 * Configures serial communication, scrolling text, and the ESP8266. 
 * 
 * Scrolling text set to an initial message.
 * 
 * ESP8266 set to act as a server on the local network defined by
 * WifiCredentials.h. Prints response from each ESP8266 command to Serial.
 */
void setup(){
  Serial.begin(BAUD_RATE_PC);
  Serial.setTimeout(STRING_TIMEOUT_MILLIS);
  espSerial.begin(BAUD_RATE_ESP);
  espSerial.setTimeout(STRING_TIMEOUT_MILLIS);
  
  initESP8266(espSerial, espSerial, 256);

  // Enable access point and station mode.
  sendCommand("AT+CWMODE=3");
  Serial.println(espSerial.readString());
  delay(1000);
  Serial.println("Access point and station active!");

  // Connect to given WiFi network.
  String connectionCommand = "AT+CWJAP=\"" + WIFI_SSID + "\",\"" + WIFI_PASSPHRASE + "\"";
  sendCommand(connectionCommand);
  delay(15000);
  Serial.println(espSerial.readString());

  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  
  Serial.println("Connection info:");
  sendCommand("AT+CIPSTA?");
  Serial.println(espSerial.readString());

  // Enable connections from more than one client.
  sendCommand("AT+CIPMUX=1");
  Serial.println(espSerial.readString());
  Serial.println("Multi connection mode!");

  // Initialize server on port 80.
  sendCommand("AT+CIPSERVER=1,80");
  Serial.println(espSerial.readString());
  Serial.println("Started server!");

  initTextScroll(g_textScrollBuffer, TEXT_SCROLL_BUFFER_SIZE);
  
  // Set initial scrolling text.
  String temp = "Ready!";
  temp.toCharArray(g_textScrollBuffer, TEXT_SCROLL_BUFFER_SIZE);
  updateMessage();
}


/* 
 * Designed to execute quickly and frequently to perform the following tasks: 
 * 
 * * Check for incoming data from ESP8266 before SoftwareSerial buffer fills.
 *   Allows 53 ms (64 bytes * 8 bits / byte / 9600 Baud).
 *   As soon as data is available, should serve client.
 * 
 * * Enable several lines of the LED matrix display, through scanDisplay.
 *   Should be called as frequently as possible to avoid visible flicker.
 * 
 * * Regularly call the function to scroll the letters of the message.
 *   Should be called much less frequently than the above functions. 
 */
void loop(){
  static uint16_t loopCount = 0; // To keep track of number of loops since last letter shift.
  if (loopCount++ >= 150) { // Magic number found through trial and error. Set higher for message to scroll slower. 
    digitalWrite(9, LOW);
    stepDisplayPartial();
    loopCount = 0;
    digitalWrite(9, HIGH);
  }
  scanDisplay();
  if (espSerial.available()) {
    respondToRequest();
  }
}

