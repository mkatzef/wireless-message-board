/* 
 * TextScroll.cpp
 * 
 * A library to draw text on an LED matrix display, and shift the present letters
 * so the given message appears "scrolling".
 * 
 * The characters drawn on the display are read from a given font file. This file 
 * must follow the format used by FontCreator (http://www.apetech.de/fontCreator). 
 * 
 * Written by Marc Katzef
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "TextScroll.h"

//Libraries for Dot Matrix
#include <SPI.h>
#include <DMD.h>

// Font characteristics
uint8_t g_fontTargetWidth; // The font array's stated width
uint8_t g_fontHeight; // The font array's stated height
char g_fontFirstChar; // The first character described by the font array
uint8_t g_fontCharCount; // The number of characters described by the font array
uint16_t g_fontDataIndex; // The starting index of the font array's character data

// Scrolling text variables
char *g_messageBuffer; // A pointer to the given message buffer
uint16_t g_messageBufferSize; // Maximum size of given buffer
uint16_t messageLength; // Current message's length 
uint16_t startLetterIndex; // Index of left-most character currently on display
int16_t startColumn; // Left-most display column occupied by start letter (can be negative)

// Additional scrolling text variables, for incremental updates
uint16_t partialLetterIndex; // Index of the next letter to be written to the display
int16_t partialStartColumn; // The left-most column to be used by the next letter

DMD dmd(1, 1);


/* 
 * Calls the DMD function responsible for rendering the LEDs written high. Should
 * be called frequently, perhaps from a timer interrupt.
 */
void scanDisplay(void) {
  dmd.scanDisplayBySPI();
}


/* 
 * Turns all LEDs (of the LED matrix display) off.
 * Currently just calls the DMD function of the same name, but prevents the use of
 * the global DMD object from other modules.
 */
void clearScreen(void) {
    dmd.clearScreen(true);
}


/* 
 * Parses the chosen font file which must be possible to index through the macro
 * FONT_ARRAY. Sets the global font-characterisation variables to the values
 * specific to the given file.
 */
void initFont(void) {
  g_fontTargetWidth = FONT_ARRAY(FONT_WIDTH_INDEX);
  g_fontHeight = FONT_ARRAY(FONT_HEIGHT_INDEX);
  g_fontFirstChar = FONT_ARRAY(FONT_FIRST_CHAR_INDEX);
  g_fontCharCount = FONT_ARRAY(FONT_CHAR_COUNT_INDEX);
  g_fontDataIndex = FONT_SIZE_ARRAY_INDEX + g_fontCharCount;
}


/* 
 * Initializes module by storing a pointer to the given buffer (which will store
 * the message to display), and the size of the given buffer.
 */
void initTextScroll(char *messageBuffer, uint16_t messageBufferSize) {
  g_messageBuffer = messageBuffer;
  g_messageBufferSize = messageBufferSize;
  initFont();
}


/* 
 * Formats the (newly-written) message buffer contents, and resets state variables
 * for text-scrolling. Should be called after writing a new message in the message
 * buffer but before the next call to a stepDisplay function.
 */
void updateMessage(void) {
  messageLength = min(g_messageBufferSize - WRAP_SPACE_COUNT - 1, strlen(g_messageBuffer));
  uint16_t i;
  
  for (i = 0; i < WRAP_SPACE_COUNT; i++) {
    g_messageBuffer[messageLength + i] = ' ';
  }

  g_messageBuffer[messageLength + WRAP_SPACE_COUNT] = 0;
  messageLength += WRAP_SPACE_COUNT;
  
  startLetterIndex = 0;
  startColumn = 0;
  clearScreen();
}


/* 
 * Turns off all LEDs in a given column.
 */
void clearColumn(uint8_t column) {
  for (uint8_t row = 0; row < DISPLAY_HEIGHT; row++) {
    dmd.writePixel(column, row, GRAPHICS_NORMAL, 0);  
  }
}


/* 
 * Draws the given symbol (described by the global font file) on the LED matrix,
 * with bottom-left corner at the given column and row.
 * Returns the number of columns which were affected in the drawing process.
 */
uint16_t writeLetter(char letter, int16_t startCol, int16_t startRow) {
  uint8_t writtenCols = 0; // Number of columns affected
  uint8_t widthCols; // Width of the given character
  
  uint16_t widthPos; // Where to find the width data
  uint16_t startPos; // Where to find the column data
  
  if (letter == ' ') {
    widthCols = g_fontTargetWidth - 1;  
  } else {
    widthPos = letter - g_fontFirstChar + FONT_SIZE_ARRAY_INDEX;
    widthCols = FONT_ARRAY(widthPos);
    startPos = 0;
    for (uint16_t i = FONT_SIZE_ARRAY_INDEX; i < widthPos; i++) {
      startPos += FONT_ARRAY(i);
    }
    startPos *= 2;
    startPos += g_fontDataIndex;
  }
  
  for (uint8_t deltaCol = 0; deltaCol < widthCols; deltaCol++) {
    uint8_t displayCol = startCol + deltaCol;
    if ((displayCol >= 0) && (displayCol < DISPLAY_WIDTH)) {
      writtenCols++;
    } else {
      continue;
    }
    uint16_t colData; // State of the pixels for the current row - 16 bit flags.

    if (letter == ' ') {
      colData = 0;
    } else {
      uint16_t colDataPos = startPos + deltaCol;
      colData = FONT_ARRAY(colDataPos + widthCols);
      colData <<= 8;
      colData |= FONT_ARRAY(colDataPos);
    }
    
    for (uint8_t deltaRow = 0; deltaRow < g_fontHeight; deltaRow++) {
      uint8_t displayRow = startRow + deltaRow;
      bool displayVal = colData & (1 << deltaRow);
      dmd.writePixel(displayCol, displayRow, GRAPHICS_NORMAL, displayVal);  
    }
  }

  uint8_t separatorCol = startCol + widthCols;
  if ((separatorCol >= 0) && (separatorCol < DISPLAY_WIDTH)) {
    writtenCols++;
    clearColumn(separatorCol);
  }
  
  return writtenCols;
}


/* 
 * Decrements the column from which the first message letter should be written,
 * then writes as many message letters to the display before it is filled (possibly
 * wrapping to the start to achieve this).
 */
void stepDisplay(void) {
  uint16_t letterIndex = startLetterIndex;
  startColumn--;

  uint16_t columnsWritten = writeLetter(g_messageBuffer[letterIndex], startColumn, 0);
  letterIndex++;
  if (columnsWritten == 0) {
    startLetterIndex++;
    startColumn = 0;
    if (startLetterIndex == messageLength) { // Last letter scrolled off display
      startLetterIndex = 0;
    }
  }

  char focusChar;
  while (columnsWritten < DISPLAY_WIDTH) {
   
    if (letterIndex >= messageLength) {
      letterIndex = 0;
    }
    focusChar = g_messageBuffer[letterIndex++];
    
    columnsWritten += writeLetter(focusChar, columnsWritten, 0);
  }
}


/* 
 * Writes a single letter to the LED matrix display at a time, decrementing the
 * starting position once each time the display is filled.
 * 
 * Performs the same function as stepDisplay if called enough times to fill
 * display. However, by writing only one letter to the display at a time, the
 * maximum blocking duration is reduced.
 */
void stepDisplayPartial(void) {
  if (partialStartColumn > 0) {
    partialStartColumn += writeLetter(g_messageBuffer[partialLetterIndex], partialStartColumn, 0);
  } else {
    partialStartColumn = writeLetter(g_messageBuffer[partialLetterIndex], partialStartColumn, 0);
  }
  partialLetterIndex++;
  if (partialLetterIndex >= messageLength) {
    partialLetterIndex = 0;
  }
  if (partialStartColumn >= DISPLAY_WIDTH) {
    startColumn--;
    partialStartColumn = startColumn;
    partialLetterIndex = startLetterIndex;
  } else if (partialStartColumn <= 0) {
    startLetterIndex++;
    startColumn = 0;
    if (startLetterIndex >= messageLength) { // Last letter scrolled off display
      startLetterIndex = 0;
    }
  }
}

