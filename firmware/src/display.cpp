#include "display.h"

#include "config.h"

namespace {

constexpr uint8_t PANEL_SIZE = 8;

constexpr uint8_t FONT[10][DIGIT_HEIGHT] = {
    {0b111, 0b101, 0b101, 0b101, 0b111}, {0b010, 0b110, 0b010, 0b010, 0b111},
    {0b111, 0b001, 0b111, 0b100, 0b111}, {0b111, 0b001, 0b111, 0b001, 0b111},
    {0b101, 0b101, 0b111, 0b001, 0b001}, {0b111, 0b100, 0b111, 0b001, 0b111},
    {0b111, 0b100, 0b111, 0b101, 0b111}, {0b111, 0b001, 0b010, 0b010, 0b010},
    {0b111, 0b101, 0b111, 0b101, 0b111}, {0b111, 0b101, 0b111, 0b001, 0b111},
};

CRGB leds[CELLS];

constexpr uint8_t panelBits(const PanelWiring &panel) {
  return (panel.flipX ? 1 : 0) | (panel.flipY ? 2 : 0) | (panel.columns ? 4 : 0);
}

constexpr uint8_t DEFAULT_LAYOUT =
    (LEFT_PANEL_FIRST ? 1 : 0) | (PANEL_SERPENTINE ? 2 : 0) | (panelBits(PANELS[0]) << 2) | (panelBits(PANELS[1]) << 5);

uint8_t layout = DEFAULT_LAYOUT;

uint16_t ledIndex(uint8_t x, uint8_t y) {
  const uint8_t panel = x / PANEL_SIZE;
  const uint8_t wiring = layout >> (2 + 3 * panel);
  const uint8_t chainSlot = (layout & 1) ? panel : 1 - panel;
  const bool columns = wiring & 4;
  uint8_t localX = x % PANEL_SIZE;
  uint8_t localY = y;
  if (wiring & 1) localX = PANEL_SIZE - 1 - localX;
  if (wiring & 2) localY = PANEL_SIZE - 1 - localY;
  const uint8_t line = columns ? localX : localY;
  uint8_t position = columns ? localY : localX;
  if ((layout & 2) && (line & 1)) position = PANEL_SIZE - 1 - position;
  return chainSlot * PANEL_SIZE * PANEL_SIZE + line * PANEL_SIZE + position;
}

}  // namespace

const Palette GOLD = {{CRGB(255, 150, 30), CRGB(255, 214, 110)}, CRGB(255, 240, 214)};
const Palette DUSK = {{CRGB(232, 62, 74), CRGB(255, 150, 70)}, CRGB(255, 222, 200)};
const Palette NIGHT = {{CRGB(92, 12, 8), CRGB(158, 32, 12)}, CRGB(165, 44, 22)};

const Palette &paletteFor(int hour) {
  if (hour >= NIGHT_START_HOUR || hour < DAY_START_HOUR) return NIGHT;
  return hour >= DUSK_START_HOUR ? DUSK : GOLD;
}

void displayBegin() {
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, CELLS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MILLIAMPS);
  FastLED.setDither(DISABLE_DITHER);
  FastLED.clear(true);
}

uint8_t displayDefaultLayout() { return DEFAULT_LAYOUT; }
uint8_t displayLayout() { return layout; }
void displaySetLayout(uint8_t value) { layout = value; }

void displayClear(uint8_t brightness) {
  FastLED.setBrightness(brightness);
  fill_solid(leds, CELLS, CRGB::Black);
}

void displayShow() { FastLED.show(); }

void setPixel(uint8_t x, uint8_t y, CRGB color) {
  if (x < COLS && y < ROWS) leds[ledIndex(x, y)] = color;
}

bool digitPixel(uint8_t digit, uint8_t col, uint8_t row) { return FONT[digit][row] & (0b100 >> col); }

void drawDigit(uint8_t digit, uint8_t left, int top, CRGB color) {
  for (uint8_t row = 0; row < DIGIT_HEIGHT; row++) {
    for (uint8_t col = 0; col < DIGIT_WIDTH; col++) {
      const int y = top + row;
      if (y >= 0 && digitPixel(digit, col, row)) setPixel(left + col, y, color);
    }
  }
}

void drawColon(uint16_t millisecond, const Palette &palette, uint8_t row1, uint8_t row2) {
  // The colon touches the digits on both sides, so it stays dim and sand-coloured to keep them separate.
  CRGB color = palette.band[1];
  color.nscale8(millisecond < 500 ? 102 : 15);
  for (uint8_t x : CHANNEL) {
    setPixel(x, row1, color);
    setPixel(x, row2, color);
  }
}
