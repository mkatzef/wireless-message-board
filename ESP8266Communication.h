
#ifndef ESP8266Communication_H_
#define ESP8266Communication_H_

#define DEFAULT_ESP_TIMEOUT_MILLIS 2000
#define DEFAULT_ESP_MAX_RETRIES 5

enum COMMAND_STATUS {COMMAND_SUCCESS=0, COMMAND_ERROR, COMMAND_BUSY,
                     COMMAND_AWAITING_INPUT, COMMAND_UNRECOGNIZED, COMMAND_TIMEOUT,
                     COMMAND_RESEND_LIM_REACHED};

enum STREAM_LISTEN_STATUS {STREAM_READY=0, STREAM_LISTEN_TIMEOUT};

uint8_t sendCommand(String command, int32_t timemoutMillis=DEFAULT_ESP_TIMEOUT_MILLIS);

uint8_t sendCommandPersistent(String command, int32_t maxRetries=DEFAULT_ESP_MAX_RETRIES);

uint8_t initESP8266(Stream &inputStream, Stream &outputStream, uint32_t dataBufferSize);

#endif /* #ifndef ESP8266Communication_H_ */
