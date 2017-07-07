
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

//Libraries for Dot Matrix
#include <SPI.h>
#include <DMD.h>
#include <TimerOne.h>

// Font
#include <avr/pgmspace.h>
#include <Arial_Black_16.h> // Font file name
#define FONT_ARRAY(X) pgm_read_byte_near(Arial_Black_16 + X) // Neater indexing

#define WRAP_SPACE_COUNT 1 // Number of spaces between instnces of the message

// Display constants
#define BRIGHTNESS_CYCLES 10 // Number of steps in one PWM period
#define BRIGHTNESS_CYCLE_DURATION 1000 // Microseconds between brightness steps
#define BRIGHTNESS_ON_CYCLES 3 // (desired screen brightness %) * BRIGHTNESS_CYCLES
#define DISPLAY_WIDTH 32
#define DISPLAY_HEIGHT 16

uint8_t g_fontTargetWidth;
uint8_t g_fontHeight;
char g_fontFirstChar;
uint8_t g_fontCharCount;
uint16_t g_fontSizeIndex;
uint16_t g_fontDataIndex;

uint8_t g_currentBCycle = 0; // The current brightness step

DMD dmd(1, 1);

void scanDMD(void){
  dmd.scanDisplayBySPI();
  g_currentBCycle++;
  if(g_currentBCycle < BRIGHTNESS_ON_CYCLES){
    digitalWrite(9, HIGH);
  } else {
    digitalWrite(9, LOW);
  }
  if (g_currentBCycle >= BRIGHTNESS_CYCLES) {
    g_currentBCycle = 0;
  }
}


void clearScreen(void) {
    dmd.clearScreen(true);
}


void flashDisplay(void) {
  dmd.scanDisplayBySPI();
}

void initDMD(void) {
  Timer1.initialize(BRIGHTNESS_CYCLE_DURATION);
  Timer1.attachInterrupt(scanDMD);   //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()
  clearScreen();     //clear/init the DMD pixels held in RAM
}


void initFont(void) {
  g_fontTargetWidth = FONT_ARRAY(2);
  g_fontHeight = FONT_ARRAY(3);
  g_fontFirstChar = FONT_ARRAY(4);
  g_fontCharCount = FONT_ARRAY(5);
  g_fontSizeIndex = 6;
  g_fontDataIndex = g_fontSizeIndex + g_fontCharCount;
}


char *g_messageBuffer;
uint16_t g_messageBufferSize;
uint16_t messageLength;
uint16_t startLetterIndex; // index of left-most character of message to display
int16_t startColumn; // column to start writing (can be negative)

// size must be >= wrap_space_count
void initTextScroll(char *messageBuffer, uint16_t messageBufferSize) {
  g_messageBuffer = messageBuffer;
  g_messageBufferSize = messageBufferSize;
  initFont();
}


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
  dmd.clearScreen(true);
}


void clearColumn(uint8_t column) {
  for (uint8_t row = 0; row < DISPLAY_HEIGHT; row++) {
    dmd.writePixel(column, row, GRAPHICS_NORMAL, 0);  
  }
}

// Take start index, and letter. Return number of columns used.
uint16_t writeLetter(char letter, int16_t startCol, int16_t startRow) {
  uint8_t writtenCols = 0; // Number of columns affected
  uint8_t widthCols; // Width of the given character
  
  uint16_t widthPos; // Where to find the width data
  uint16_t startPos; // Where to find the column data
  
  if (letter == ' ') {
    widthCols = g_fontTargetWidth - 1;  
  } else {
    widthPos = letter - g_fontFirstChar + g_fontSizeIndex;
    widthCols = FONT_ARRAY(widthPos);
    startPos = 0;
    for (uint16_t i = g_fontSizeIndex; i < widthPos; i++) {
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


// An incrememental version of step display - more suitable for weaving tasks but must be called more frequently.
// Breaks task by limiting function to one call to writeLetter per call of this function.
int16_t partialStartColumn;
uint16_t partialLetterIndex;

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

