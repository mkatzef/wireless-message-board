/* 
 * TextScroll.h
 * 
 * A library to draw text on an LED matrix display, and shift the present letters
 * so the given message appears "scrolling".
 * 
 * The characters drawn on the display are read from a given font file. This file 
 * must follow the format used by FontCreator (http://www.apetech.de/fontCreator). 
 * 
 * Written by Marc Katzef
 */
 
#ifndef TextScroll_H_
#define TextScroll_H_

#include <stdlib.h>
#include <stdbool.h>

//Libraries for Dot Matrix
#include <SPI.h>
#include <DMD.h>

// Font
#include <avr/pgmspace.h>
#include <Arial_Black_16.h> // Font file name
#define FONT_ARRAY(X) pgm_read_byte_near(Arial_Black_16 + X) // Neater indexing
#define FONT_WIDTH_INDEX 2
#define FONT_HEIGHT_INDEX 3
#define FONT_FIRST_CHAR_INDEX 4
#define FONT_CHAR_COUNT_INDEX 5
#define FONT_SIZE_ARRAY_INDEX 6

#define WRAP_SPACE_COUNT 1 // Number of spaces between instnces of the message

// Display constants
#define DISPLAY_WIDTH 32
#define DISPLAY_HEIGHT 16

/* 
 * Calls the DMD function responsible for rendering the LEDs written high. Should
 * be called frequently, perhaps from a timer interrupt.
 */
void scanDisplay(void);

/* 
 * Turns all LEDs (of the LED matrix display) off.
 * Currently just calls the DMD function of the same name, but prevents the use of
 * the global DMD object from other modules.
 */
void clearScreen(void);

/* 
 * Initializes module by storing a pointer to the given buffer (which will store
 * the message to display), and the size of the given buffer.
 */
void initTextScroll(char *messageBuffer, uint16_t messageBufferSize);

/* 
 * Formats the (newly-written) message buffer contents, and resets state variables
 * for text-scrolling. Should be called after writing a new message in the message
 * buffer but before the next call to a stepDisplay function.
 */
void updateMessage(void);

/* 
 * Decrements the column from which the first message letter should be written,
 * then writes as many message letters to the display before it is filled (possibly
 * wrapping to the start to achieve this).
 */
void stepDisplay(void);

/* 
 * Writes a single letter to the LED matrix display at a time, decrementing the
 * starting position once each time the display is filled.
 * 
 * Performs the same function as stepDisplay if called enough times to fill
 * display. However, by writing only one letter to the display at a time, the
 * maximum blocking duration is reduced.
 */
void stepDisplayPartial(void);

#endif /* TextScroll_H_ */
