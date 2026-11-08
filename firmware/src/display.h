#pragma once

#include <FastLED.h>

constexpr uint8_t COLS = 16;
constexpr uint8_t ROWS = 8;
constexpr uint16_t CELLS = COLS * ROWS;

// HH and MM fill all 16 columns, leaving columns 7 and 8 free across the panel join.
constexpr uint8_t DIGIT_COLUMNS[4] = {0, 4, 9, 13};
constexpr uint8_t CHANNEL[2] = {7, 8};
constexpr uint8_t DIGIT_WIDTH = 3;
constexpr uint8_t DIGIT_HEIGHT = 5;

struct Palette {
  CRGB band[2];
  CRGB digit;
};

struct ClockTime {
  int hour;
  int minute;
  int second;
  uint16_t millisecond;
  int month;  // 0 is January.
  int yearDay;
  int utcOffsetMinutes;
  uint32_t unixTime;
};

extern const Palette GOLD;
extern const Palette DUSK;
extern const Palette NIGHT;

const Palette &paletteFor(int hour);

void displayBegin();

// The panel layout is one byte, so it's easy to save and send to the web page.
// Bit 0: left panel is first in the chain. Bit 1: every other line runs backwards.
// Bits 2-4 are the left panel and bits 5-7 the right: flip X, flip Y, lines run as columns.
uint8_t displayDefaultLayout();
uint8_t displayLayout();
void displaySetLayout(uint8_t layout);

void displayClear(uint8_t brightness);
void displayShow();
void setPixel(uint8_t x, uint8_t y, CRGB color);

bool digitPixel(uint8_t digit, uint8_t col, uint8_t row);
void drawDigit(uint8_t digit, uint8_t left, int top, CRGB color);
void drawColon(uint16_t millisecond, const Palette &palette, uint8_t row1, uint8_t row2);
