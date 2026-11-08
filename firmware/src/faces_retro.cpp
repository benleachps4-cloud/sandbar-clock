#include "faces_retro.h"

#include <math.h>

namespace {

// Seven-segment digits, 3 wide and 7 tall, with the segments named in the usual way.
constexpr uint8_t SEG_A = 1, SEG_B = 2, SEG_C = 4, SEG_D = 8, SEG_E = 16, SEG_F = 32, SEG_G = 64;
constexpr uint8_t SEGMENT_HEIGHT = 7;

constexpr uint8_t DIGIT_SEGMENTS[10] = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
    SEG_B | SEG_C,
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,
    SEG_B | SEG_C | SEG_F | SEG_G,
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,
    SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
    SEG_A | SEG_B | SEG_C,
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,
};

// Which segments each pixel belongs to. Corners are shared, so they light with either neighbour.
constexpr uint8_t PIXEL_SEGMENTS[SEGMENT_HEIGHT][DIGIT_WIDTH] = {
    {SEG_A | SEG_F, SEG_A, SEG_A | SEG_B},
    {SEG_F, 0, SEG_B},
    {SEG_F, 0, SEG_B},
    {SEG_E | SEG_F | SEG_G, SEG_G, SEG_B | SEG_C | SEG_G},
    {SEG_E, 0, SEG_C},
    {SEG_E, 0, SEG_C},
    {SEG_D | SEG_E, SEG_D, SEG_C | SEG_D},
};

void drawSegmentDigit(uint8_t digit, uint8_t left, CRGB on, CRGB off) {
  for (uint8_t y = 0; y < SEGMENT_HEIGHT; y++) {
    for (uint8_t x = 0; x < DIGIT_WIDTH; x++) {
      const uint8_t segments = PIXEL_SEGMENTS[y][x];
      if (segments) setPixel(left + x, y, (segments & DIGIT_SEGMENTS[digit]) ? on : off);
    }
  }
}

void drawBlinkingColon(const ClockTime &clock, CRGB color, uint8_t row1, uint8_t row2) {
  if (clock.millisecond >= 500) return;
  for (uint8_t x : CHANNEL) {
    setPixel(x, row1, color);
    setPixel(x, row2, color);
  }
}

void drawRedLed(const ClockTime &clock, const uint8_t digits[4]) {
  for (uint8_t i = 0; i < 4; i++) drawSegmentDigit(digits[i], DIGIT_COLUMNS[i], CRGB(255, 0, 0), CRGB(12, 0, 0));
  drawBlinkingColon(clock, CRGB(130, 0, 0), 2, 4);
}

void drawVacuumDisplay(const ClockTime &clock, const uint8_t digits[4], uint32_t now) {
  for (uint8_t i = 0; i < 4; i++) {
    CRGB on(0, 255, 170);
    on.nscale8(205 + scale8(sin8(now / 9 + i * 50), 50));  // A gentle shimmer that drifts across the glass.
    drawSegmentDigit(digits[i], DIGIT_COLUMNS[i], on, CRGB(0, 12, 9));
  }
  drawBlinkingColon(clock, CRGB(0, 170, 120), 2, 4);
  setPixel(clock.second * COLS / 60, ROWS - 1, CRGB(0, 110, 85));
}

void drawNixieTubes(const ClockTime &clock, const uint8_t digits[4], uint32_t now) {
  // At each new minute the digits that changed spin through their numerals, as real tubes are made to.
  const bool spinning = clock.second == 0 && clock.millisecond < 700;
  const bool spins[4] = {clock.minute == 0 && clock.hour % 10 == 0, clock.minute == 0, digits[3] == 0, true};
  for (uint8_t i = 0; i < 4; i++) {
    const uint8_t left = DIGIT_COLUMNS[i];
    for (uint8_t x = 0; x < DIGIT_WIDTH; x++) {
      for (uint8_t y = 0; y < ROWS - 1; y++) setPixel(left + x, y, CRGB(20, 6, 1));
      setPixel(left + x, ROWS - 1, CRGB(26, 26, 32));
    }
    // The numerals stacked behind the lit one show faintly, as they do in a real tube.
    drawDigit(8, left, 1, CRGB(44, 14, 2));
    const uint8_t shown = spinning && spins[i] ? (digits[i] + clock.millisecond / 70) % 10 : digits[i];
    CRGB glow(255, 92, 10);
    glow.nscale8(225 + scale8(sin8(now / 3 + i * 90), 30));
    drawDigit(shown, left, 1, glow);
    CRGB ember(120, 38, 0);
    ember.nscale8(150 + scale8(sin8(now / 5 + i * 70), 105));
    setPixel(left + 1, ROWS - 1, ember);
  }
  drawBlinkingColon(clock, CRGB(170, 48, 0), 2, 4);
}

void drawBell(uint8_t left, bool struck) {
  const CRGB body = struck ? CRGB(255, 80, 50) : CRGB(165, 18, 14);
  const CRGB rim = struck ? CRGB(200, 40, 25) : CRGB(80, 8, 6);
  setPixel(left + 1, 0, struck ? CRGB(255, 190, 150) : CRGB(235, 90, 70));  // Shine on the dome.
  setPixel(left + 2, 0, body);
  for (uint8_t x = 0; x < 4; x++) {
    setPixel(left + x, 1, body);
    setPixel(left + x, 2, rim);
  }
}

void drawTwinBell(const ClockTime &clock, const uint8_t digits[4], uint32_t now) {
  // The hammer ticks from side to side each second, then rings for the first 12 seconds of each hour.
  const bool ringing = clock.minute == 0 && clock.second < 12;
  const bool hitsRight = ringing ? (now / 70) & 1 : clock.second & 1;
  drawBell(3, ringing && !hitsRight);
  drawBell(9, ringing && hitsRight);
  const uint8_t hammer = hitsRight ? 8 : 7;
  const CRGB metal = ringing ? CRGB(240, 240, 255) : CRGB(110, 110, 125);
  setPixel(hammer, 0, metal);
  setPixel(hammer, 1, metal);
  setPixel(7, 2, CRGB(45, 45, 55));
  setPixel(8, 2, CRGB(45, 45, 55));
  if (ringing) {
    const uint8_t spark = hitsRight ? 14 : 1;
    setPixel(spark, 0, CRGB(255, 210, 60));
    setPixel(hitsRight ? 15 : 0, 1, CRGB(255, 150, 30));
  }
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], 3, CRGB(255, 232, 185));
  drawBlinkingColon(clock, CRGB(130, 112, 80), 4, 6);
}

void drawClockRadio(const ClockTime &clock, const uint8_t digits[4], uint32_t now) {
  static constexpr uint8_t STATIONS[3] = {3, 8, 13};
  constexpr uint8_t DIAL_TOP = 6;
  constexpr uint32_t SWEEP_MS = 48000;
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], 0, CRGB(255, 140, 18));
  drawBlinkingColon(clock, CRGB(125, 68, 8), 1, 3);

  for (uint8_t x = 0; x < COLS; x++) {
    setPixel(x, DIAL_TOP, x % 4 == 1 ? CRGB(115, 66, 10) : (x % 2 == 1 ? CRGB(48, 27, 4) : CRGB(14, 8, 1)));
    setPixel(x, DIAL_TOP + 1, CRGB(34, 19, 3));
  }

  // The needle drifts along the dial, and the tuning light comes on as it reaches a station.
  const float needle = 7.5f + 7.5f * sinf((now % SWEEP_MS) * TWO_PI / SWEEP_MS);
  const uint8_t left = uint8_t(needle);
  const float fraction = needle - left;
  bool tuned = false;
  for (uint8_t station : STATIONS) {
    if (fabsf(needle - station) < 0.7f) tuned = true;
  }
  for (uint8_t y = DIAL_TOP; y < ROWS; y++) {
    CRGB near(255, 18, 0);
    near.nscale8(uint8_t((1.0f - fraction) * 255));
    if (fraction < 0.8f) setPixel(left, y, near);
    if (fraction > 0.2f && left + 1 < COLS) {
      CRGB far(255, 18, 0);
      far.nscale8(uint8_t(fraction * 255));
      setPixel(left + 1, y, far);
    }
  }
  setPixel(COLS - 1, DIAL_TOP - 1, tuned ? CRGB(0, 220, 40) : CRGB(0, 22, 5));
}

void drawPocketLcd(const ClockTime &clock, const uint8_t digits[4]) {
  const CRGB screen(62, 98, 20);
  for (uint8_t x = 0; x < COLS; x++) {
    for (uint8_t y = 0; y < ROWS; y++) setPixel(x, y, screen);
  }
  for (uint8_t i = 0; i < 4; i++) drawSegmentDigit(digits[i], DIGIT_COLUMNS[i], CRGB::Black, CRGB(56, 89, 18));
  // One pixel wide and softer than the digits, so it doesn't merge with them. It sits next to the
  // minutes because the left edge of those digits is lit less often than the right edge of the hours.
  if (clock.millisecond < 500) {
    setPixel(CHANNEL[1], 2, CRGB(22, 38, 8));
    setPixel(CHANNEL[1], 4, CRGB(22, 38, 8));
  }
  const uint8_t seconds = (clock.second + 1) * COLS / 60;
  for (uint8_t x = 0; x < seconds; x++) setPixel(x, ROWS - 1, CRGB(8, 14, 2));
}

// A quick repeatable random value, for flickers that don't need any saved state.
uint8_t flickerValue(uint32_t step) { return (step * 2654435761UL) >> 24; }

void drawFlipCards(const ClockTime &clock, const uint8_t digits[4]) {
  constexpr uint16_t ROW_MS = 80;
  const uint16_t earlier = (clock.hour * 60 + clock.minute + 1439) % 1440;
  const uint8_t previous[4] = {uint8_t(earlier / 600), uint8_t(earlier / 60 % 10), uint8_t(earlier % 60 / 10), uint8_t(earlier % 10)};
  const bool flipping = clock.second == 0 && clock.millisecond < ROW_MS * SEGMENT_HEIGHT;
  for (uint8_t i = 0; i < 4; i++) {
    const uint8_t left = DIGIT_COLUMNS[i];
    for (uint8_t x = 0; x < DIGIT_WIDTH; x++) {
      for (uint8_t y = 0; y < ROWS; y++) {
        setPixel(left + x, y, y == ROWS - 1 ? CRGB(10, 5, 1) : (y == 3 ? CRGB(6, 3, 1) : CRGB(28, 14, 4)));
      }
    }
    for (uint8_t y = 0; y < SEGMENT_HEIGHT; y++) {
      // The new digit wipes down the card a row at a time, like a card falling past its hinge.
      const uint8_t digit = !flipping || clock.millisecond >= y * ROW_MS ? digits[i] : previous[i];
      for (uint8_t x = 0; x < DIGIT_WIDTH; x++) {
        if (PIXEL_SEGMENTS[y][x] & DIGIT_SEGMENTS[digit]) setPixel(left + x, y, y == 3 ? CRGB(185, 160, 115) : CRGB(255, 226, 170));
      }
    }
  }
  drawBlinkingColon(clock, CRGB(92, 78, 54), 2, 4);
}

void drawNeonSign(const ClockTime &clock, const uint8_t digits[4], uint32_t now) {
  // The tubes struggle to light for the first two seconds of each hour, then hum with an occasional dip.
  const uint8_t chance = flickerValue(now / 70);
  if (clock.minute == 0 && clock.second < 2 && (chance & 3) != 0) {
    for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], 1, CRGB(36, 3, 18));
    return;
  }
  const uint8_t level = (chance & 31) == 0 ? 165 : 225 + scale8(sin8(now / 2), 30);
  CRGB tube(0, 62, 225);
  tube.nscale8(level);
  for (uint8_t x = 1; x < COLS - 1; x++) {
    setPixel(x, 0, tube);
    setPixel(x, ROWS - 1, tube);
  }
  CRGB pink(255, 18, 120);
  pink.nscale8(level);
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], 1, pink);
  drawBlinkingColon(clock, CRGB(120, 8, 56), 2, 4);
}

void drawTestCard(const ClockTime &clock, const uint8_t digits[4]) {
  static constexpr uint8_t BARS[7][3] = {{30, 30, 30}, {30, 30, 0}, {0, 30, 30}, {0, 30, 0}, {30, 0, 30}, {30, 0, 0}, {0, 0, 30}};
  const bool staticBurst = clock.second == 0 && clock.millisecond < 500;
  for (uint8_t x = 0; x < COLS; x++) {
    const uint8_t *bar = BARS[x * 7 / COLS];
    for (uint8_t y = 0; y < ROWS; y++) {
      if (staticBurst) {
        const uint8_t snow = random8(90);
        setPixel(x, y, CRGB(snow, snow, snow));
      } else {
        setPixel(x, y, CRGB(bar[0], bar[1], bar[2]));
      }
    }
  }
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], 1, CRGB::White);
  drawBlinkingColon(clock, CRGB(110, 110, 110), 2, 4);
}

void drawAmberTerminal(const ClockTime &clock, const uint8_t digits[4], uint32_t now) {
  constexpr uint8_t TEXT_ROW = ROWS - 1;
  const uint8_t scanRow = (now / 130) % ROWS;
  for (uint8_t y = 0; y < ROWS; y++) {
    const CRGB glass = y == scanRow ? CRGB(26, 13, 0) : CRGB(4, 2, 0);
    for (uint8_t x = 0; x < COLS; x++) setPixel(x, y, glass);
  }
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t row = 0; row < DIGIT_HEIGHT; row++) {
      for (uint8_t col = 0; col < DIGIT_WIDTH; col++) {
        if (digitPixel(digits[i], col, row)) setPixel(DIGIT_COLUMNS[i] + col, row, row == scanRow ? CRGB(255, 185, 50) : CRGB(225, 125, 0));
      }
    }
  }
  drawBlinkingColon(clock, CRGB(120, 66, 0), 1, 3);

  // A prompt types one character every 6 seconds, with the block cursor following it.
  setPixel(0, TEXT_ROW, CRGB(170, 95, 0));
  setPixel(1, TEXT_ROW, CRGB(170, 95, 0));
  const uint8_t typed = clock.second / 6;
  for (uint8_t n = 0; n < typed; n++) setPixel(3 + n, TEXT_ROW, CRGB(95, 52, 0));
  if (clock.millisecond < 500) {
    for (uint8_t x = 3 + typed; x < 5 + typed; x++) {
      setPixel(x, TEXT_ROW - 1, CRGB(255, 160, 20));
      setPixel(x, TEXT_ROW, CRGB(255, 160, 20));
    }
  }
}

void drawVuMeter(const ClockTime &clock, const uint8_t digits[4], uint32_t now) {
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], 0, CRGB(235, 235, 210));
  drawBlinkingColon(clock, CRGB(110, 110, 98), 1, 3);
  for (uint8_t channel = 0; channel < 2; channel++) {
    const int noise = inoise8(now / 2 + channel * 9000, channel * 5000);
    const bool beat = sin8(now / 4 + channel * 30) > 205;
    const uint8_t level = constrain((noise - 70) * COLS / 110 + (beat ? 3 : 0), 1, COLS);
    for (uint8_t x = 0; x < level; x++) {
      setPixel(x, 6 + channel, x < 10 ? CRGB(0, 200, 30) : (x < 13 ? CRGB(230, 170, 0) : CRGB(255, 25, 0)));
    }
  }
}

void drawBubbleCalculator(const ClockTime &clock, const uint8_t digits[4]) {
  for (uint8_t i = 0; i < 4; i++) {
    const uint8_t left = DIGIT_COLUMNS[i];
    // Each digit sits under its own round lens: dim glass with the corners left dark.
    for (uint8_t y = 0; y < 7; y++) {
      for (uint8_t x = 0; x < DIGIT_WIDTH; x++) {
        if (x == 1 || (y > 0 && y < 6)) setPixel(left + x, y, CRGB(8, 0, 0));
      }
    }
    for (uint8_t row = 0; row < DIGIT_HEIGHT; row++) {
      for (uint8_t col = 0; col < DIGIT_WIDTH; col++) {
        if (digitPixel(digits[i], col, row)) setPixel(left + col, 1 + row, col == 1 ? CRGB(255, 30, 10) : CRGB(175, 8, 4));
      }
    }
  }
  drawBlinkingColon(clock, CRGB(90, 0, 0), 2, 4);
}

}  // namespace

void retroFaceDraw(uint8_t index, const ClockTime &clock, const uint8_t digits[4], uint32_t now) {
  switch (index) {
    case 0: drawRedLed(clock, digits); break;
    case 1: drawVacuumDisplay(clock, digits, now); break;
    case 2: drawNixieTubes(clock, digits, now); break;
    case 3: drawTwinBell(clock, digits, now); break;
    case 4: drawClockRadio(clock, digits, now); break;
    case 5: drawPocketLcd(clock, digits); break;
    case 6: drawFlipCards(clock, digits); break;
    case 7: drawNeonSign(clock, digits, now); break;
    case 8: drawTestCard(clock, digits); break;
    case 9: drawAmberTerminal(clock, digits, now); break;
    case 10: drawVuMeter(clock, digits, now); break;
    default: drawBubbleCalculator(clock, digits); break;
  }
}
