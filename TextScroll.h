#include <stdlib.h>
#include <stdbool.h>

//Libraries for Dot Matrix
#include <SPI.h>
#include <DMD.h>
#include <TimerOne.h>

// Font
#include <avr/pgmspace.h>
#include <Arial_Black_16.h> // Font file name
#define FONT_ARRAY(X) pgm_read_byte_near(Arial_Black_16 + X) // Neater indexing

#define WRAP_SPACE_COUNT 1

// Display constants
#define BRIGHTNESS_CYCLES 10 // Number of steps in one PWM period
#define BRIGHTNESS_CYCLE_DURATION 1000 // Microseconds between brightness steps
#define BRIGHTNESS_ON_CYCLES 3 // (desired screen brightness %) * BRIGHTNESS_CYCLES
#define DISPLAY_WIDTH 32
#define DISPLAY_HEIGHT 16

void scanDMD(void);

void clearScreen(void);

void initTextScroll(char *messageBuffer, uint16_t messageBufferSize);

void updateMessage(void);

void stepDisplayPartial(void);

void stepDisplay(void);

void flashDisplay(void);

