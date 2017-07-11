/* 
 * ESP8266Communication.cpp
 * 
 * A library to simplify the communication between an Arduino and ESP8266, designed
 * around a single (given size) array for both input and output text.
 * 
 * Written by Marc Katzef
 */
 
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <Arduino.h>

#include "ESP8266Communication.h"

char *g_dataBuffer; // The fundamental data array
uint32_t g_dataBufferSize; // Size of the data array, in chars

Stream *inputStreamESP; // The stream from which ESP8266 data (e.g. HTTP messages) is received
Stream *outputStreamESP; // The stream to which ESP8266 data (e.g. AT commands) must be sent

/*
 * Tests if data is available on the given input stream or timeout. If timeout
 * value is negative, will wait indefinitely. Returns true if data is available,
 * false if timeout was reached.
 */
bool waitForInput(Stream &inputStream, int32_t timeoutMillis) {
  bool canTimeout = timeoutMillis >= 0;
  uint32_t endTime = millis() + timeoutMillis;
  
  while (!inputStream.available()) {
    if (millis() >= endTime) {
      return false;
    }
  }

  return true;
}


/*
 * Sends the given AT command to the connected ESP8266, then waits for a response
 * or timeout. If a negative timeout value is given, will wait for response
 * indefinitely.
 * Returns one of the following status codes:
 * COMMAND_SUCCESS if response ends with "OK".
 * COMMAND_ERROR if response ends with "ERROR".
 * COMMAND_BUSY if response contains "busy".
 * COMMAND_AWAITING_INPUT if response ends with ">".
 * COMMAND_UNRECOGNIZED if response was received but matched none of the above.
 * COMMAND_TIMEOUT if no response was received before timeout.
 */
uint8_t sendCommand(String command, int32_t timeoutMillis=DEFAULT_ESP_TIMEOUT_MILLIS) {
  outputStreamESP->print(command);
  outputStreamESP->print("\r\n"); // Required to terminate command
  
  bool listenStatus = waitForInput(*inputStreamESP, timeoutMillis);
  if (!listenStatus) {
    return COMMAND_TIMEOUT;
  }
  // TODO
  // Read data into buffer (ensure no more data available)
  // Search buffer for the substrings mentioned above
  // Return suitable error codes

  while (inputStreamESP->available()) {
    inputStreamESP->read();
    delay(1);
  }

  return COMMAND_SUCCESS;
}


/*
 * A wrapper for the function "sendCommand" which resends the command while the
 * returned status is one of the following:
 * COMMAND_BUSY - small delay before resending command.
 * COMMAND_AWAITING_INPUT - prints single character to outputStreamESP before
 * resending.
 * COMMAND_TIMEOUT - resend immediately.
 * 
 * Any other status code received from sendCommand is returned immediately. 
 * 
 * After maxRetries resends, COMMAND_RESEND_LIM_REACHED is returned. If maxRetries
 * is negative, function will resend until receiving status codes not listed above.
 */
uint8_t sendCommandPersistent(String command, int32_t maxRetries=DEFAULT_ESP_MAX_RETRIES) {
  bool retriesLimited = maxRetries >= 0;
  uint32_t retryCount = 0;
  uint8_t statusESP;
  while ((retriesLimited && retryCount < maxRetries) || !retriesLimited) {
    statusESP = sendCommand(command);
    retryCount++;

    if (statusESP == COMMAND_BUSY) {
      delay(DEFAULT_RESEND_DELAY_MILLIS);
      continue;
    } else if (statusESP == COMMAND_AWAITING_INPUT) {
      outputStreamESP->print(' ');
      continue;
    } else if (statusESP == COMMAND_TIMEOUT) {
      continue;
    } else {
      return statusESP;
    }
  }
}


/*
 *  Initializes module by storing the given input and output streams* which may be
 *  used to communicate with the ESP8266 module, and allocating a buffer of given
 *  size (as a number of chars) for data storage.
 *  
 *  Returns the error code received from disabling ESP8266 command echo.
 *  
 *  * For example, these may each be a reference to the same SoftwareSerial object.
 */
uint8_t initESP8266(Stream &inputStream, Stream &outputStream, uint32_t dataBufferSize=DEFAULT_DATA_BUFFER_SIZE) {
  inputStreamESP = &inputStream;
  outputStreamESP = &outputStream;
  g_dataBuffer = (char*)malloc(dataBufferSize * sizeof(char));
  g_dataBufferSize = dataBufferSize;
  uint8_t statusESP = sendCommandPersistent("ATE0", -1);

  return statusESP;
}

