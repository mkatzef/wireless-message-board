/* 
 * ESP8266Communication.h
 * 
 * A library to simplify the communication between an Arduino and ESP8266, designed
 * around a single (given size) array for both input and output text.
 * 
 * Written by Marc Katzef
 */

#ifndef ESP8266Communication_H_
#define ESP8266Communication_H_

#define DEFAULT_ESP_TIMEOUT_MILLIS 2000
#define DEFAULT_ESP_MAX_RETRIES 5
#define DEFAULT_RESEND_DELAY_MILLIS 50
#define DEFAULT_DATA_BUFFER_SIZE 256

// Status codes to classify response from ESP8266
enum COMMAND_STATUS {COMMAND_SUCCESS=0, COMMAND_ERROR, COMMAND_BUSY,
                     COMMAND_AWAITING_INPUT, COMMAND_UNRECOGNIZED, COMMAND_TIMEOUT,
                     COMMAND_RESEND_LIM_REACHED};


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
uint8_t sendCommand(String command, int32_t timemoutMillis=DEFAULT_ESP_TIMEOUT_MILLIS);

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
uint8_t sendCommandPersistent(String command, int32_t maxRetries=DEFAULT_ESP_MAX_RETRIES);

/*
 *  Initializes module by storing the given input and output streams* which may be
 *  used to communicate with the ESP8266 module, and allocating a buffer of given
 *  size (as a number of chars) for data storage.
 *  
 *  Returns the error code received from disabling ESP8266 command echo.
 *  
 *  * For example, these may each be a reference to the same SoftwareSerial object.
 */
uint8_t initESP8266(Stream &inputStream, Stream &outputStream, uint32_t dataBufferSize);

#endif /* ESP8266Communication_H_ */

