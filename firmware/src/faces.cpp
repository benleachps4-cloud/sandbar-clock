#include "faces.h"

#include <initializer_list>
#include <math.h>
#include <string.h>

#include "config.h"

namespace {

constexpr uint8_t GRAINS = 30;
constexpr uint8_t GRAINS_PER_BAND = 5;
constexpr uint8_t MINUTES_PER_GRAIN = 2;
constexpr uint32_t TICK_MS = 33;
constexpr uint32_t DRAIN_MS = 1900;
constexpr uint32_t SHUFFLE_DELAY_SECONDS = 3;  // Lets the Sandbar face finish draining on the hour.
constexpr int8_t EMPTY = -1;

constexpr uint8_t DIGIT_SAND_TOP = 1;
constexpr uint16_t SAND_FALL_MS = 430;
constexpr uint16_t SAND_SINK_AT = 1100;
constexpr uint16_t SAND_SINK_MS = 300;
constexpr uint16_t SAND_BUILD_AT = 1500;
constexpr uint16_t SAND_BUILD_FALL_MS = 280;
constexpr uint16_t SAND_DONE_AT = 2600;
constexpr uint8_t MAX_PARTICLES = 60;

constexpr uint16_t FLAP_MS = 340;
constexpr uint16_t FLAP_STAGGER_MS = 60;
constexpr uint16_t ROLL_MS = 380;
constexpr uint16_t ROLL_STAGGER_MS = 90;
constexpr uint8_t ROLL_TOP = 1;
constexpr uint8_t SKY_ROWS = 3;
constexpr uint8_t SCENE_DIGIT_TOP = 3;
constexpr int DAY_START_MINUTE = 6 * 60;
constexpr int DAY_LENGTH_MINUTES = 12 * 60;

constexpr uint8_t RAIN_DROPS = 9;
constexpr uint8_t WATER_CELLS = 48;
constexpr uint16_t SPLASH_MS = 110;
constexpr uint8_t LAVA_TOP = 1;
constexpr uint8_t SNOW_TOP = 2;
constexpr uint8_t SNOW_FLAKES = 14;
constexpr uint8_t SNOW_FLOOR_ROWS = 2;
constexpr uint8_t SNOW_BURST = 40;
constexpr uint8_t BUILDING_HEIGHTS[COLS] = {2, 3, 3, 1, 0, 2, 3, 2, 1, 3, 3, 0, 2, 1, 3, 2};
constexpr uint8_t CHASE_TOP = 3;
constexpr uint8_t CREATURE_WIDTH = 3;
constexpr uint16_t CHASE_RUN_MS = 700;
constexpr uint16_t CHASE_STOMP_MS = 220;
constexpr uint16_t CHASE_FALL_AT = CHASE_RUN_MS + 150;
constexpr uint16_t CHASE_FALL_MS = 260;
constexpr uint16_t CHASE_RISE_AT = CHASE_RUN_MS + 410;
constexpr uint16_t CHASE_RISE_MS = 300;
constexpr uint16_t CHASE_DONE_AT = CHASE_RISE_AT + CHASE_RISE_MS + 50;
constexpr float CHASE_WALK_PER_SECOND = 4.5f;

constexpr uint8_t SECONDS_BAR_ROW = ROWS - 1;
constexpr uint8_t BINARY_COLUMNS[4] = {1, 4, 9, 12};
constexpr uint8_t BINARY_WIDTH = 2;
constexpr uint8_t BINARY_BIT_ROWS = 2;
constexpr uint8_t TALLY_GROUP_WIDTH = 4;
constexpr uint8_t TALLY_GROUPS_PER_LINE = 2;
constexpr uint8_t TALLY_LINE_HEIGHT = 4;
constexpr uint8_t TALLY_STROKE_HEIGHT = 3;
constexpr uint8_t TALLY_STRIKE_ROWS[4] = {2, 1, 1, 0};
constexpr uint8_t TALLY_LEFT = 8;
constexpr uint8_t TALLY_MINUTES_PER_STROKE = 5;
constexpr uint8_t TALLY_DIGIT_TOP = 1;
constexpr uint8_t TALLY_EXTRA_ROW = ROWS - 1;
constexpr uint8_t WORD_TOP = 1;
constexpr uint16_t WORD_SCROLL_MS = 140;
constexpr uint8_t WORD_BUFFER = 44;
constexpr uint8_t PLANTS = 8;
constexpr uint8_t SOIL_ROW = ROWS - 1;
constexpr int GARDEN_DAY_START = 6 * 60;
constexpr int GARDEN_DAY_END = 18 * 60;
constexpr float GARDENER_WALK_PER_SECOND = 3.0f;
constexpr uint16_t GARDENER_WATER_MS = 1600;

constexpr uint8_t CODE_TRAIL = 6;
constexpr uint8_t FIRE_TOP = 5;
constexpr uint8_t STARS_COUNT = 22;
constexpr uint8_t HEARTBEAT_LENGTH = 16;
constexpr uint8_t HEARTBEAT_TRACE[HEARTBEAT_LENGTH] = {6, 6, 6, 6, 5, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};
constexpr uint16_t HEARTBEAT_STEP_MS = 62;
constexpr uint8_t MAX_FIREWORKS = 3;
constexpr uint16_t FIREWORK_MS = 900;
constexpr float FIREWORK_RADIUS = 4.0f;
constexpr float FIREWORK_DX[8] = {1.0f, 0.7f, 0.0f, -0.7f, -1.0f, -0.7f, 0.0f, 0.7f};
constexpr float FIREWORK_DY[8] = {0.0f, 0.7f, 1.0f, 0.7f, 0.0f, -0.7f, -1.0f, -0.7f};
constexpr uint8_t SEA_TOP = 6;
constexpr uint8_t FISH_COUNT = 3;
constexpr uint8_t BUBBLES = 3;
constexpr uint8_t TANK_TOP = 5;
constexpr uint16_t MARQUEE_STEP_MS = 140;

// M and W need five columns to be readable.
constexpr uint8_t WIDE_LETTER_WIDTH = 5;
constexpr uint8_t WIDE_M[DIGIT_HEIGHT] = {0b10001, 0b11011, 0b10101, 0b10001, 0b10001};
constexpr uint8_t WIDE_W[DIGIT_HEIGHT] = {0b10001, 0b10001, 0b10101, 0b11011, 0b10001};

constexpr uint8_t LETTERS[26][DIGIT_HEIGHT] = {
    {0b010, 0b101, 0b111, 0b101, 0b101}, {0b110, 0b101, 0b110, 0b101, 0b110}, {0b011, 0b100, 0b100, 0b100, 0b011},
    {0b110, 0b101, 0b101, 0b101, 0b110}, {0b111, 0b100, 0b110, 0b100, 0b111}, {0b111, 0b100, 0b110, 0b100, 0b100},
    {0b011, 0b100, 0b101, 0b101, 0b011}, {0b101, 0b101, 0b111, 0b101, 0b101}, {0b111, 0b010, 0b010, 0b010, 0b111},
    {0b001, 0b001, 0b001, 0b101, 0b010}, {0b101, 0b101, 0b110, 0b101, 0b101}, {0b100, 0b100, 0b100, 0b100, 0b111},
    {0b101, 0b111, 0b111, 0b101, 0b101}, {0b110, 0b101, 0b101, 0b101, 0b101}, {0b010, 0b101, 0b101, 0b101, 0b010},
    {0b110, 0b101, 0b110, 0b100, 0b100}, {0b010, 0b101, 0b101, 0b110, 0b011}, {0b110, 0b101, 0b110, 0b101, 0b101},
    {0b011, 0b100, 0b010, 0b001, 0b110}, {0b111, 0b010, 0b010, 0b010, 0b010}, {0b101, 0b101, 0b101, 0b101, 0b111},
    {0b101, 0b101, 0b101, 0b101, 0b010}, {0b101, 0b101, 0b111, 0b111, 0b101}, {0b101, 0b101, 0b010, 0b101, 0b101},
    {0b101, 0b101, 0b010, 0b010, 0b010}, {0b111, 0b001, 0b010, 0b100, 0b111},
};

// Maps the 8 display rows onto the 5 rows of the font.
constexpr uint8_t TALL_ROW[ROWS] = {0, 1, 1, 2, 2, 3, 3, 4};

struct DrainingGrain {
  uint8_t x;
  uint8_t y;
  int8_t grain;
  uint16_t delayMs;
};

struct Particle {
  uint8_t x;
  int8_t startY;
  int8_t endY;
  uint16_t delayMs;
};

struct DigitChange {
  uint8_t digit[4];
  uint8_t previous[4];
  uint32_t startedAt[4];
  bool changing[4];
};

struct Drop {
  uint8_t x;
  float y;
  float speed;
};

struct Flake {
  uint8_t x;
  int8_t y;
  uint32_t nextMove;
  uint16_t stepMs;
};

Drop rain[RAIN_DROPS];
float waterLevel = 0;
int splashX = -1;
int splashY = 0;
uint32_t splashAt = 0;
uint32_t lastAnimation = 0;

Flake flakes[SNOW_FLAKES];
Drop snowBurst[SNOW_BURST];
uint8_t snowBurstCount = 0;
bool snowRest[CELLS];
bool digitMask[CELLS];

struct Star {
  float x;
  float y;
  float vx;
  float vy;
};

struct Firework {
  uint8_t x;
  uint8_t y;
  uint8_t hue;
  uint32_t start;
  bool active;
};

struct Fish {
  float x;
  float speed;
  int8_t direction;
  uint8_t y;
  CRGB color;
};

float codeHead[COLS];
float codeRate[COLS];
Star stars[STARS_COUNT];
Firework fireworks[MAX_FIREWORKS];
uint32_t nextFirework = 0;
Fish fish[FISH_COUNT];
Drop bubbles[BUBBLES];

char wordText[WORD_BUFFER];
int wordWidth = 0;
uint32_t wordStart = 0;

float gardenerX = 0;
uint8_t gardenerTarget = 0;
bool gardenerWatering = false;
uint32_t wateringUntil = 0;

float chaseX = 0;
int8_t chaseDirection = 1;
bool chaseStomping = false;
uint32_t chaseStart = 0;
float chaseFromX = 0;
uint8_t chaseTargetX = 0;

int8_t cells[CELLS];
uint32_t moved[CELLS];
uint8_t jitter[GRAINS];
uint8_t slotX[GRAINS];
uint8_t slotY[GRAINS];
DrainingGrain drainList[GRAINS];
Particle fallingOld[MAX_PARTICLES];
Particle buildingNew[MAX_PARTICLES];
DigitChange change;

uint8_t drainCount = 0;
bool draining = false;
uint32_t drainStart = 0;
uint8_t released = 0;
uint32_t tickCount = 0;
uint32_t lastTick = 0;
int lastHour = -1;

uint8_t fallingCount = 0;
uint8_t buildingCount = 0;
bool digitSandActive = false;
uint32_t digitSandStart = 0;

uint8_t activeFace = FACE_COUNT;
uint8_t lastSelection = FACE_COUNT;
int shuffleHour = -1;

bool inBounds(int x, int y) { return x >= 0 && y >= 0 && x < COLS && y < ROWS; }
uint16_t at(int x, int y) { return y * COLS + x; }
bool isEmpty(int x, int y) { return inBounds(x, y) && cells[at(x, y)] == EMPTY; }

float smoothStep(float t) { return t * t * (3.0f - 2.0f * t); }

void splitDigits(const ClockTime &clock, uint8_t digits[4]) {
  digits[0] = clock.hour / 10;
  digits[1] = clock.hour % 10;
  digits[2] = clock.minute / 10;
  digits[3] = clock.minute % 10;
}

// ---- Sandbar -------------------------------------------------------------------

// Fill order for the pile: bottom row first, centre outwards.
void buildSlots() {
  uint8_t count = 0;
  for (int y = ROWS - 1; y >= 0 && count < GRAINS; y--) {
    for (int offset = 0; offset < COLS / 2 && count < GRAINS; offset++) {
      for (int x : {COLS / 2 - 1 - offset, COLS / 2 + offset}) {
        if (count == GRAINS) break;
        slotX[count] = x;
        slotY[count] = y;
        count++;
      }
    }
  }
}

void resetSand(uint8_t grains) {
  memset(cells, EMPTY, sizeof(cells));
  memset(moved, 0, sizeof(moved));
  tickCount = 0;
  released = grains;
  for (uint8_t i = 0; i < grains; i++) cells[at(slotX[i], slotY[i])] = i;
}

void moveGrain(uint16_t from, uint16_t to) {
  cells[to] = cells[from];
  cells[from] = EMPTY;
  moved[to] = tickCount;
}

bool canFall(int x, int y) { return isEmpty(x, y + 1) || isEmpty(x - 1, y + 1) || isEmpty(x + 1, y + 1); }

// Walk along the grain's row to the nearest spot it could fall from.
int levelDirection(int x, int y) {
  int best = 0;
  int bestDistance = COLS;
  const int first = random8(2) ? 1 : -1;
  for (int dir : {first, -first}) {
    for (int k = 1; k < COLS; k++) {
      const int nextX = x + dir * k;
      if (!isEmpty(nextX, y)) break;
      if (canFall(nextX, y)) {
        if (k < bestDistance) {
          best = dir;
          bestDistance = k;
        }
        break;
      }
    }
  }
  return best;
}

void updateGrain(int x, int y) {
  const uint16_t here = at(x, y);
  if (cells[here] == EMPTY || moved[here] == tickCount) return;
  if (isEmpty(x, y + 1)) {
    moveGrain(here, at(x, y + 1));
    return;
  }
  const int firstSide = random8(2) ? 1 : -1;
  for (int dir : {firstSide, -firstSide}) {
    if (isEmpty(x + dir, y + 1)) {
      moveGrain(here, at(x + dir, y + 1));
      return;
    }
  }
  const int dir = levelDirection(x, y);
  if (dir != 0) moveGrain(here, at(x + dir, y));
}

void tickSand(uint8_t target) {
  tickCount++;
  const bool flip = tickCount & 1;
  for (int y = ROWS - 1; y >= 0; y--) {
    for (int i = 0; i < COLS; i++) updateGrain(flip ? COLS - 1 - i : i, y);
  }
  const uint16_t spawn = at(CHANNEL[random8(2)], 0);
  if (tickCount % 3 == 0 && released < target && cells[spawn] == EMPTY) {
    cells[spawn] = released;
    moved[spawn] = tickCount;
    released++;
  }
}

// On the hour the floor opens and the sand falls away, centre first.
void startDrain(uint32_t now) {
  drainCount = 0;
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      const int8_t grain = cells[at(x, y)];
      if (grain == EMPTY) continue;
      const int fromCentre = abs(2 * x - (COLS - 1));
      drainList[drainCount++] = {uint8_t(x), uint8_t(y), grain, uint16_t(fromCentre * 45 + (ROWS - 1 - y) * 120)};
    }
  }
  resetSand(0);
  draining = true;
  drainStart = now;
}

CRGB grainColor(int8_t grain, const Palette &palette) { return palette.band[(grain / GRAINS_PER_BAND) % 2]; }

void updateSand(const ClockTime &clock, uint32_t now) {
  const uint8_t target = clock.minute / MINUTES_PER_GRAIN;
  if (draining) {
    if (now - drainStart < DRAIN_MS) return;
    draining = false;
    lastTick = now;
  }
  if (clock.hour != lastHour) {
    lastHour = clock.hour;
    startDrain(now);
    return;
  }
  if (target < released || target > released + 2) resetSand(target);  // The clock was corrected.
  if (now - lastTick > 200) lastTick = now - TICK_MS;
  while (now - lastTick >= TICK_MS) {
    tickSand(target);
    lastTick += TICK_MS;
  }
}

void drawSandbar(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  updateSand(clock, now);
  drawColon(clock.millisecond, palette, 1, 3);
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      const int8_t grain = cells[at(x, y)];
      if (grain == EMPTY) continue;
      CRGB color = grainColor(grain, palette);
      color.nscale8(jitter[grain]);
      setPixel(x, y, color);
    }
  }
  if (draining) {
    const int32_t elapsed = now - drainStart;
    for (uint8_t i = 0; i < drainCount; i++) {
      const DrainingGrain &item = drainList[i];
      const float k = max(0.0f, (elapsed - item.delayMs) / 450.0f);
      const float y = item.y + k * k * 8.0f;
      if (y < ROWS) setPixel(item.x, uint8_t(y), grainColor(item.grain, palette));
    }
  }
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], 0, palette.digit);
}

// ---- Tracking which digits changed ------------------------------------------------

void resetDigitChange(const uint8_t digits[4]) {
  memcpy(change.digit, digits, 4);
  memcpy(change.previous, digits, 4);
  memset(change.changing, 0, sizeof(change.changing));
}

// Returns true on the frame when any digit changes. Earlier digits start later when rightFirst is set.
bool trackDigits(const uint8_t digits[4], uint32_t now, uint16_t stagger, bool rightFirst) {
  bool any = false;
  for (uint8_t i = 0; i < 4; i++) {
    if (digits[i] == change.digit[i]) continue;
    change.previous[i] = change.digit[i];
    change.digit[i] = digits[i];
    change.startedAt[i] = now + stagger * (rightFirst ? 3 - i : i);
    change.changing[i] = true;
    any = true;
  }
  return any;
}

// ---- Digit sand --------------------------------------------------------------------

void startDigitSand(uint32_t now) {
  uint8_t height[COLS] = {0};
  fallingCount = 0;
  buildingCount = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if (!change.changing[i]) continue;
    for (int row = DIGIT_HEIGHT - 1; row >= 0; row--) {
      const uint16_t rowDelay = (DIGIT_HEIGHT - 1 - row);
      for (uint8_t col = 0; col < DIGIT_WIDTH; col++) {
        const uint8_t x = DIGIT_COLUMNS[i] + col;
        if (digitPixel(change.previous[i], col, row)) {
          const int8_t endY = ROWS - 1 - height[x]++;
          fallingOld[fallingCount++] = {x, int8_t(DIGIT_SAND_TOP + row), endY, uint16_t(rowDelay * 35 + random8(60))};
        }
        if (digitPixel(change.digit[i], col, row)) {
          buildingNew[buildingCount++] = {x, -1, int8_t(DIGIT_SAND_TOP + row), uint16_t(SAND_BUILD_AT + rowDelay * 70 + random8(40))};
        }
      }
    }
  }
  digitSandActive = true;
  digitSandStart = now;
}

void drawParticle(uint8_t x, float y, CRGB color) {
  if (y > -0.5f && y < ROWS) setPixel(x, uint8_t(max(0.0f, y)), color);
}

void drawDigitSand(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  if (trackDigits(digits, now, 0, false)) startDigitSand(now);
  drawColon(clock.millisecond, palette, DIGIT_SAND_TOP + 1, DIGIT_SAND_TOP + 3);

  if (digitSandActive && now - digitSandStart >= SAND_DONE_AT) {
    digitSandActive = false;
    memset(change.changing, 0, sizeof(change.changing));
  }
  for (uint8_t i = 0; i < 4; i++) {
    if (!change.changing[i]) drawDigit(digits[i], DIGIT_COLUMNS[i], DIGIT_SAND_TOP, palette.digit);
  }
  if (!digitSandActive) return;

  const int32_t elapsed = now - digitSandStart;
  for (uint8_t i = 0; i < fallingCount; i++) {
    const Particle &grain = fallingOld[i];
    const float t = constrain(float(elapsed - grain.delayMs) / SAND_FALL_MS, 0.0f, 1.0f);
    float y = grain.startY + (grain.endY - grain.startY) * t * t;
    if (elapsed > SAND_SINK_AT) {
      const float sink = float(elapsed - SAND_SINK_AT) / SAND_SINK_MS;
      y = grain.endY + sink * sink * ROWS;
    }
    drawParticle(grain.x, y, palette.band[grain.x % 2]);
  }
  for (uint8_t i = 0; i < buildingCount; i++) {
    const Particle &grain = buildingNew[i];
    if (elapsed < grain.delayMs) continue;
    const float t = min(1.0f, float(elapsed - grain.delayMs) / SAND_BUILD_FALL_MS);
    drawParticle(grain.x, grain.startY + (grain.endY - grain.startY) * t * t, palette.digit);
  }
}

// ---- Big digits and split-flap -----------------------------------------------------

void drawTallRow(uint8_t digit, uint8_t left, uint8_t y, uint8_t sourceRow, CRGB color) {
  for (uint8_t col = 0; col < DIGIT_WIDTH; col++) {
    if (digitPixel(digit, col, TALL_ROW[sourceRow])) setPixel(left + col, y, color);
  }
}

void drawBigDigits(const ClockTime &clock, const uint8_t digits[4], const Palette &palette) {
  drawColon(clock.millisecond, palette, 2, 5);
  for (uint8_t y = 0; y < ROWS; y++) {
    const CRGB color = blend(palette.digit, palette.band[0], y * 255 / (ROWS - 1));
    for (uint8_t i = 0; i < 4; i++) drawTallRow(digits[i], DIGIT_COLUMNS[i], y, y, color);
  }
}

CRGB flapColor(uint8_t y, const Palette &palette) {
  CRGB color = palette.digit;
  if (y >= ROWS / 2) color.nscale8(190);  // The lower half is dimmer, like a shaded flap.
  return color;
}

void drawFlapDigit(uint8_t i, uint32_t now, const Palette &palette) {
  const uint8_t left = DIGIT_COLUMNS[i];
  const uint8_t halfRows = ROWS / 2;
  uint8_t sourceDigit[ROWS];
  uint8_t sourceRow[ROWS];
  float t = 1.0f;
  if (change.changing[i]) {
    const int32_t elapsed = now - change.startedAt[i];
    t = elapsed < 0 ? 0.0f : float(elapsed) / FLAP_MS;
    if (t >= 1.0f) {
      change.changing[i] = false;
      t = 1.0f;
    }
  }
  for (uint8_t y = 0; y < ROWS; y++) {
    sourceDigit[y] = y < halfRows ? change.digit[i] : change.previous[i];
    sourceRow[y] = y;
  }
  if (t >= 1.0f) {
    for (uint8_t y = 0; y < ROWS; y++) sourceDigit[y] = change.digit[i];
  } else if (t < 0.5f) {
    // The old top half folds down towards the hinge.
    const uint8_t height = constrain(halfRows - int(t * 2 * halfRows), 1, int(halfRows));
    for (uint8_t r = 0; r < height; r++) {
      const uint8_t y = halfRows - height + r;
      sourceDigit[y] = change.previous[i];
      sourceRow[y] = r * halfRows / height;
    }
  } else {
    // The new bottom half unfolds below the hinge.
    const uint8_t height = constrain(1 + int((t - 0.5f) * 2 * (halfRows - 1)), 1, int(halfRows));
    for (uint8_t r = 0; r < height; r++) {
      const uint8_t y = halfRows + r;
      sourceDigit[y] = change.digit[i];
      sourceRow[y] = halfRows + r * halfRows / height;
    }
  }
  for (uint8_t y = 0; y < ROWS; y++) drawTallRow(sourceDigit[y], left, y, sourceRow[y], flapColor(y, palette));
}

void drawSplitFlap(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  trackDigits(digits, now, FLAP_STAGGER_MS, false);
  drawColon(clock.millisecond, palette, 2, 5);
  for (uint8_t i = 0; i < 4; i++) drawFlapDigit(i, now, palette);
}

// ---- Sun and moon -------------------------------------------------------------------

void drawSkyGlow(int x, int y, CRGB color) {
  if (x >= 0 && x < COLS && y >= 0 && y < SKY_ROWS) setPixel(x, y, color);
}

void drawStars(uint32_t now) {
  static constexpr uint8_t STARS[7][2] = {{1, 0}, {4, 2}, {6, 0}, {9, 1}, {11, 0}, {13, 2}, {15, 1}};
  for (const auto &star : STARS) {
    const uint8_t twinkle = sin8(now / 9 + star[0] * 37);
    setPixel(star[0], star[1], CHSV(160, 40, scale8(twinkle, 150) + 40));
  }
}

void drawSunAndMoon(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const int minutes = clock.hour * 60 + clock.minute;
  const int sinceSunrise = (minutes - DAY_START_MINUTE + 24 * 60) % (24 * 60);
  const bool day = sinceSunrise < DAY_LENGTH_MINUTES;
  const float progress = float(day ? sinceSunrise : sinceSunrise - DAY_LENGTH_MINUTES) / DAY_LENGTH_MINUTES;
  const int bodyX = roundf(progress * (COLS - 1));
  const int bodyY = roundf((SKY_ROWS - 1) * (1.0f - sinf(progress * PI)));

  if (day) {
    const bool lowSun = progress < 0.12f || progress > 0.88f;
    const CRGB sky = lowSun ? CRGB(44, 14, 30) : CRGB(0, 10, 34);
    for (uint8_t y = 0; y < SKY_ROWS; y++) {
      for (uint8_t x = 0; x < COLS; x++) setPixel(x, y, sky);
    }
    const CRGB glow = lowSun ? CRGB(120, 34, 6) : CRGB(120, 76, 16);
    drawSkyGlow(bodyX - 1, bodyY, glow);
    drawSkyGlow(bodyX + 1, bodyY, glow);
    drawSkyGlow(bodyX, bodyY - 1, glow);
    drawSkyGlow(bodyX, bodyY + 1, glow);
    drawSkyGlow(bodyX, bodyY, lowSun ? CRGB(255, 120, 30) : CRGB(255, 196, 70));
  } else {
    drawStars(now);
    drawSkyGlow(bodyX, bodyY, CRGB(210, 218, 235));
  }

  drawColon(clock.millisecond, palette, SCENE_DIGIT_TOP + 1, SCENE_DIGIT_TOP + 3);
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], SCENE_DIGIT_TOP, palette.digit);
}

// ---- Rolling digits -------------------------------------------------------------------

void drawClippedDigit(uint8_t digit, uint8_t left, int top, CRGB color) {
  for (uint8_t row = 0; row < DIGIT_HEIGHT; row++) {
    const int y = top + row;
    if (y < ROLL_TOP || y >= ROLL_TOP + DIGIT_HEIGHT) continue;
    for (uint8_t col = 0; col < DIGIT_WIDTH; col++) {
      if (digitPixel(digit, col, row)) setPixel(left + col, y, color);
    }
  }
}

void drawRolling(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  trackDigits(digits, now, ROLL_STAGGER_MS, true);
  drawColon(clock.millisecond, palette, ROLL_TOP + 1, ROLL_TOP + 3);
  const int travel = DIGIT_HEIGHT + 1;
  for (uint8_t i = 0; i < 4; i++) {
    const uint8_t left = DIGIT_COLUMNS[i];
    if (!change.changing[i]) {
      drawDigit(digits[i], left, ROLL_TOP, palette.digit);
      continue;
    }
    const int32_t elapsed = now - change.startedAt[i];
    const float t = elapsed < 0 ? 0.0f : float(elapsed) / ROLL_MS;
    if (t >= 1.0f) {
      change.changing[i] = false;
      drawDigit(digits[i], left, ROLL_TOP, palette.digit);
      continue;
    }
    const int shift = roundf(smoothStep(t) * travel);
    drawClippedDigit(change.previous[i], left, ROLL_TOP - shift, palette.digit);
    drawClippedDigit(change.digit[i], left, ROLL_TOP + travel - shift, palette.digit);
  }
}

// ---- Shared animation timing ------------------------------------------------------------

float frameSeconds(uint32_t now) {
  const uint32_t elapsed = min(now - lastAnimation, uint32_t(50));
  lastAnimation = now;
  return elapsed / 1000.0f;
}

// ---- Raindrops -----------------------------------------------------------------------

void respawnDrop(Drop &drop) {
  drop.x = random8(COLS);
  drop.y = -float(random8(8)) - 1;
  drop.speed = 9.0f + random8(8);
}

uint8_t waterHeight(uint8_t x) {
  const uint8_t cellsNow = waterLevel;
  const uint8_t fullRows = cellsNow / COLS;
  const uint8_t partial = cellsNow % COLS;
  const bool extra = abs(2 * int(x) - (COLS - 1)) < partial;
  return fullRows + (extra ? 1 : 0);
}

void drawRaindrops(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const float dt = frameSeconds(now);
  const float target = clock.minute * WATER_CELLS / 60.0f;
  const float rate = target < waterLevel ? 30.0f : 4.0f;  // The water drains quickly on the hour.
  waterLevel += constrain(target - waterLevel, -rate * dt, rate * dt);

  for (uint8_t y = 0; y < ROWS; y++) {
    for (uint8_t x = 0; x < COLS; x++) {
      const uint8_t height = waterHeight(x);
      if (y < ROWS - height) continue;
      CRGB water = y == ROWS - height ? CRGB(24, 96, 210) : CRGB(0, 34, 130);
      water.nscale8(150 + scale8(sin8(now / 7 + x * 29 + y * 41), 105));
      setPixel(x, y, water);
    }
  }
  for (Drop &drop : rain) {
    drop.y += drop.speed * dt;
    const int surface = ROWS - waterHeight(drop.x);
    if (drop.y >= surface) {
      splashX = drop.x;
      splashY = surface - 1;
      splashAt = now;
      respawnDrop(drop);
      continue;
    }
    if (drop.y >= 0) setPixel(drop.x, uint8_t(drop.y), CRGB(70, 130, 230));
  }
  if (splashX >= 0 && now - splashAt < SPLASH_MS && splashY >= 0) {
    const CRGB spray(120, 170, 240);
    if (splashX > 0) setPixel(splashX - 1, splashY, spray);
    if (splashX < COLS - 1) setPixel(splashX + 1, splashY, spray);
  }
  drawColon(clock.millisecond, palette, 1, 3);
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], 0, palette.digit);
}

// ---- Lava time ---------------------------------------------------------------------

CRGB lavaColor(uint8_t heat) {
  if (heat < 128) return blend(CRGB(26, 1, 0), CRGB(105, 12, 0), heat * 2);
  return blend(CRGB(105, 12, 0), CRGB(165, 58, 4), (heat - 128) * 2);
}

void drawLava(const ClockTime &clock, const uint8_t digits[4], uint32_t now) {
  for (uint8_t y = 0; y < ROWS; y++) {
    for (uint8_t x = 0; x < COLS; x++) {
      const uint8_t noise = inoise8(x * 36, y * 52, uint16_t(now / 7));
      const uint8_t heat = qadd8(qsub8(noise, 60), scale8(qsub8(noise, 60), 110));
      setPixel(x, y, lavaColor(heat));
    }
  }
  // Icy digits stand out against the dimmed lava.
  const CRGB digitColor(190, 235, 255);
  if (clock.millisecond < 500) {
    for (uint8_t x : CHANNEL) {
      setPixel(x, LAVA_TOP + 1, CRGB(40, 60, 70));
      setPixel(x, LAVA_TOP + 3, CRGB(40, 60, 70));
    }
  }
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], LAVA_TOP, digitColor);
}

// ---- Snow globe --------------------------------------------------------------------

void respawnFlake(Flake &flake, uint32_t now) {
  flake.x = random8(COLS);
  flake.y = -1 - int8_t(random8(6));
  flake.stepMs = 140 + random8(140);
  flake.nextMove = now + flake.stepMs;
}

void buildDigitMask(const uint8_t digits[4], uint8_t top) {
  memset(digitMask, 0, sizeof(digitMask));
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t row = 0; row < DIGIT_HEIGHT; row++) {
      for (uint8_t col = 0; col < DIGIT_WIDTH; col++) {
        if (digitPixel(digits[i], col, row)) digitMask[at(DIGIT_COLUMNS[i] + col, top + row)] = true;
      }
    }
  }
}

bool snowBlocked(int x, int y) { return y >= ROWS || (y >= 0 && (digitMask[at(x, y)] || snowRest[at(x, y)])); }

void moveFlake(Flake &flake, uint32_t now) {
  if (now < flake.nextMove) return;
  flake.nextMove = now + flake.stepMs;
  if (random8(4) == 0 && flake.y >= 0) {
    const int side = flake.x + (random8(2) ? 1 : -1);
    if (side >= 0 && side < COLS && !snowBlocked(side, flake.y)) flake.x = side;
  }
  const int below = flake.y + 1;
  if (!snowBlocked(flake.x, below)) {
    flake.y = below;
    return;
  }
  const bool onFloor = below >= ROWS || (snowRest[at(flake.x, below)] && flake.y >= ROWS - SNOW_FLOOR_ROWS);
  const bool onDigit = below < ROWS && digitMask[at(flake.x, below)];
  if (flake.y >= 0 && (onFloor || onDigit)) snowRest[at(flake.x, flake.y)] = true;
  respawnFlake(flake, now);
}

void shakeOffSnow() {
  snowBurstCount = 0;
  for (uint8_t y = 0; y < ROWS; y++) {
    for (uint8_t x = 0; x < COLS; x++) {
      if (snowRest[at(x, y)] && snowBurstCount < SNOW_BURST) {
        snowBurst[snowBurstCount++] = {x, float(y), 12.0f + random8(9)};
      }
    }
  }
  memset(snowRest, 0, sizeof(snowRest));
}

void drawSnow(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const float dt = frameSeconds(now);
  if (trackDigits(digits, now, 0, false)) shakeOffSnow();
  memset(change.changing, 0, sizeof(change.changing));
  buildDigitMask(digits, SNOW_TOP);
  for (Flake &flake : flakes) moveFlake(flake, now);

  // Dim, cold snow against strong amber digits keeps the time readable, with settled snow the dimmest.
  const bool night = &palette == &NIGHT;
  const CRGB fallingSnow = night ? CRGB(8, 20, 60) : CRGB(30, 70, 190);
  const CRGB settledSnow = night ? CRGB(3, 8, 28) : CRGB(10, 26, 90);
  const CRGB digitColor = night ? CRGB(170, 42, 0) : CRGB(255, 120, 0);
  for (uint8_t y = 0; y < ROWS; y++) {
    for (uint8_t x = 0; x < COLS; x++) {
      if (snowRest[at(x, y)]) setPixel(x, y, settledSnow);
    }
  }
  for (const Flake &flake : flakes) {
    if (flake.y >= 0) setPixel(flake.x, flake.y, fallingSnow);
  }
  for (uint8_t i = 0; i < snowBurstCount; i++) {
    snowBurst[i].y += snowBurst[i].speed * dt;
    if (snowBurst[i].y < ROWS) setPixel(snowBurst[i].x, uint8_t(snowBurst[i].y), fallingSnow);
  }
  drawColon(clock.millisecond, palette, SNOW_TOP + 1, SNOW_TOP + 3);
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], SNOW_TOP, digitColor);
}

// ---- City at night -----------------------------------------------------------------

bool windowLit(uint8_t x, uint8_t y, uint32_t now) {
  const uint32_t period = 1500 + x * 170 + y * 90;
  const uint8_t epoch = now / period;
  const uint8_t hash = sin8(x * 47 + y * 91) ^ uint8_t(epoch * (x + 3) * 29);
  return uint8_t(hash * 167 + 13) < 80;
}

void drawCity(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  // The skyline stays in the three rows below the digits, so nothing competes with the time.
  for (uint8_t x = 0; x < COLS; x++) {
    const uint8_t height = BUILDING_HEIGHTS[x];
    const CRGB wall = x % 4 < 2 ? CRGB(14, 24, 96) : CRGB(40, 16, 92);
    for (uint8_t y = ROWS - height; y < ROWS; y++) {
      setPixel(x, y, height >= 2 && windowLit(x, y, now) ? CRGB(255, 170, 40) : wall);
    }
  }
  drawColon(clock.millisecond, palette, 1, 3);
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], 0, palette.digit);
}

// ---- Digit chase -------------------------------------------------------------------

void drawCreature(int x, int top, bool stepping, int8_t direction, const Palette &palette) {
  const CRGB body = palette.band[0];
  setPixel(x + 1, top, body);
  setPixel(x, top + 1, body);
  setPixel(x + 1, top + 1, body);
  setPixel(x + 2, top + 1, body);
  setPixel(x + (direction > 0 ? 2 : 0), top + 1, palette.digit);
  if (stepping) {
    setPixel(x, top + 2, body);
    setPixel(x + 2, top + 2, body);
  } else {
    setPixel(x + 1, top + 2, body);
  }
}

void startStomp(uint32_t now) {
  chaseStomping = true;
  chaseStart = now;
  chaseFromX = chaseX;
  for (uint8_t i = 0; i < 4; i++) {
    if (change.changing[i]) {
      chaseTargetX = DIGIT_COLUMNS[i];
      chaseDirection = chaseTargetX >= chaseX ? 1 : -1;
      return;
    }
  }
}

void walkCreature(float dt) {
  const float limit = COLS - CREATURE_WIDTH;
  chaseX += chaseDirection * CHASE_WALK_PER_SECOND * dt;
  if (chaseX <= 0 || chaseX >= limit) {
    chaseX = constrain(chaseX, 0.0f, limit);
    chaseDirection = -chaseDirection;
  }
}

void drawChase(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const float dt = frameSeconds(now);
  if (trackDigits(digits, now, 0, false)) startStomp(now);
  drawColon(clock.millisecond, palette, CHASE_TOP + 1, CHASE_TOP + 3);

  int creatureTop = 0;
  uint32_t elapsed = now - chaseStart;
  if (chaseStomping && elapsed >= CHASE_DONE_AT) {
    chaseStomping = false;
    memset(change.changing, 0, sizeof(change.changing));
  }
  if (chaseStomping) {
    chaseX = chaseFromX + (chaseTargetX - chaseFromX) * smoothStep(min(1.0f, float(elapsed) / CHASE_RUN_MS));
    if (elapsed >= CHASE_RUN_MS && elapsed < uint32_t(CHASE_RUN_MS + CHASE_STOMP_MS)) creatureTop = 1;
  } else {
    walkCreature(dt);
  }

  for (uint8_t i = 0; i < 4; i++) {
    const uint8_t left = DIGIT_COLUMNS[i];
    if (!change.changing[i]) {
      drawDigit(digits[i], left, CHASE_TOP, palette.digit);
    } else if (elapsed < CHASE_FALL_AT) {
      drawDigit(change.previous[i], left, CHASE_TOP, palette.digit);
    } else if (elapsed < CHASE_RISE_AT) {
      const float t = float(elapsed - CHASE_FALL_AT) / CHASE_FALL_MS;
      drawDigit(change.previous[i], left, CHASE_TOP + int(t * t * ROWS), palette.digit);
    } else {
      const float t = min(1.0f, float(elapsed - CHASE_RISE_AT) / CHASE_RISE_MS);
      drawDigit(digits[i], left, CHASE_TOP + roundf((1.0f - smoothStep(t)) * (ROWS - CHASE_TOP)), palette.digit);
    }
  }
  drawCreature(roundf(chaseX), creatureTop, int(chaseX * 2) & 1, chaseDirection, palette);
}

// ---- Seconds bar -------------------------------------------------------------------

void drawSecondsBar(const ClockTime &clock, const uint8_t digits[4], const Palette &palette) {
  drawColon(clock.millisecond, palette, 2, 4);
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], 1, palette.digit);

  const float progress = (clock.second + clock.millisecond / 1000.0f) * COLS / 60.0f;
  const uint8_t full = progress;
  CRGB bar = palette.band[0];
  bar.nscale8(150);
  for (uint8_t x = 0; x < full; x++) setPixel(x, SECONDS_BAR_ROW, bar);
  if (full < COLS) {
    CRGB tip = palette.band[1];
    tip.nscale8(40 + uint8_t((progress - full) * 215));
    setPixel(full, SECONDS_BAR_ROW, tip);
  }
}

// ---- Binary columns ----------------------------------------------------------------

void drawBinary(const ClockTime &clock, const uint8_t digits[4], const Palette &palette) {
  drawColon(clock.millisecond, palette, 2, 5);
  const CRGB off(14, 11, 8);
  for (uint8_t i = 0; i < 4; i++) {
    const CRGB on = i < 2 ? palette.digit : palette.band[0];
    for (uint8_t bit = 0; bit < 4; bit++) {
      const CRGB color = digits[i] & (1 << bit) ? on : off;
      const uint8_t top = (3 - bit) * BINARY_BIT_ROWS;
      for (uint8_t y = top; y < top + BINARY_BIT_ROWS; y++) {
        for (uint8_t x = 0; x < BINARY_WIDTH; x++) setPixel(BINARY_COLUMNS[i] + x, y, color);
      }
    }
  }
}

// ---- Tally marks -------------------------------------------------------------------

// The hour stays as normal digits. Each stroke is 5 minutes, and dots along the bottom count the rest.
void drawTally(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  drawDigit(digits[0], DIGIT_COLUMNS[0], TALLY_DIGIT_TOP, palette.digit);
  drawDigit(digits[1], DIGIT_COLUMNS[1], TALLY_DIGIT_TOP, palette.digit);

  const uint8_t strokes = clock.minute / TALLY_MINUTES_PER_STROKE;
  for (uint8_t s = 0; s < strokes; s++) {
    const uint8_t group = s / 5;
    const uint8_t stroke = s % 5;
    const uint8_t left = TALLY_LEFT + (group % TALLY_GROUPS_PER_LINE) * TALLY_GROUP_WIDTH;
    const uint8_t top = (group / TALLY_GROUPS_PER_LINE) * TALLY_LINE_HEIGHT;
    CRGB color = stroke == 4 ? palette.digit : palette.band[group % 2];  // Groups touch, so they alternate colour.
    if (s == strokes - 1) color.nscale8(120 + scale8(sin8(now / 4), 135));  // The newest stroke pulses.
    if (stroke < 4) {
      for (uint8_t y = 0; y < TALLY_STROKE_HEIGHT; y++) setPixel(left + stroke, top + y, color);
    } else {
      for (uint8_t k = 0; k < 4; k++) setPixel(left + k, top + TALLY_STRIKE_ROWS[k], color);
    }
  }
  const uint8_t extra = clock.minute % TALLY_MINUTES_PER_STROKE;
  for (uint8_t i = 0; i < extra; i++) setPixel(TALLY_LEFT + i * 2, TALLY_EXTRA_ROW, palette.band[1]);
}

// ---- Word clock --------------------------------------------------------------------

uint8_t letterWidth(char letter) { return letter == 'M' || letter == 'W' ? WIDE_LETTER_WIDTH : DIGIT_WIDTH; }

void buildWords(const ClockTime &clock) {
  static const char *const HOURS[12] = {"TWELVE", "ONE", "TWO", "THREE", "FOUR", "FIVE", "SIX", "SEVEN", "EIGHT", "NINE", "TEN", "ELEVEN"};
  static const char *const LEAD[12] = {"", "FIVE PAST ", "TEN PAST ", "QUARTER PAST ", "TWENTY PAST ", "TWENTY FIVE PAST ", "HALF PAST ",
                                       "TWENTY FIVE TO ", "TWENTY TO ", "QUARTER TO ", "TEN TO ", "FIVE TO "};
  const int nearest = (clock.minute + 2) / 5;
  const uint8_t step = nearest % 12;
  const int hourOffset = nearest == 12 || step > 6 ? 1 : 0;
  const char *hour = HOURS[(clock.hour + hourOffset) % 12];
  snprintf(wordText, sizeof(wordText), step == 0 ? "%s%s OCLOCK" : "%s%s", LEAD[step], hour);
  wordWidth = 0;
  for (uint8_t i = 0; wordText[i] != 0; i++) wordWidth += letterWidth(wordText[i]) + 1;
}

void drawLetter(char letter, int left, CRGB color) {
  if (letter < 'A' || letter > 'Z') return;
  const uint8_t width = letterWidth(letter);
  const uint8_t *glyph = letter == 'M' ? WIDE_M : letter == 'W' ? WIDE_W : LETTERS[letter - 'A'];
  for (uint8_t row = 0; row < DIGIT_HEIGHT; row++) {
    for (uint8_t col = 0; col < width; col++) {
      const int x = left + col;
      if (x >= 0 && x < COLS && (glyph[row] & (1 << (width - 1 - col)))) setPixel(x, WORD_TOP + row, color);
    }
  }
}

void drawWords(const ClockTime &clock, uint32_t now, const Palette &palette) {
  int offset = (now - wordStart) / WORD_SCROLL_MS;
  if (wordWidth == 0 || offset > wordWidth + COLS) {
    buildWords(clock);
    wordStart = now;
    offset = 0;
  }
  int left = COLS - offset;
  for (uint8_t i = 0; wordText[i] != 0; i++) {
    drawLetter(wordText[i], left, palette.digit);
    left += letterWidth(wordText[i]) + 1;
  }
}

// ---- Pixel gardener -----------------------------------------------------------------

uint8_t plantColumn(uint8_t plant) { return 1 + plant * 2; }

void drawGardener(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const float dt = frameSeconds(now);
  drawColon(clock.millisecond, palette, 1, 3);
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], 0, palette.digit);

  const int minutes = clock.hour * 60 + clock.minute;
  const bool daytime = minutes >= GARDEN_DAY_START && minutes < GARDEN_DAY_END;
  const float growth = daytime ? float(minutes - GARDEN_DAY_START) / (GARDEN_DAY_END - GARDEN_DAY_START) : 1.0f;

  for (uint8_t x = 0; x < COLS; x++) setPixel(x, SOIL_ROW, CRGB(46, 20, 4));
  uint8_t grown = 0;
  for (uint8_t i = 0; i < PLANTS; i++) {
    const float progress = growth * PLANTS - i;
    const uint8_t x = plantColumn(i);
    if (progress <= 0) {
      setPixel(x, SOIL_ROW - 1, CRGB(70, 34, 8));  // A seed waiting its turn.
      continue;
    }
    grown++;
    setPixel(x, SOIL_ROW - 1, CRGB(20, 130, 20));
    if (progress >= 0.67f) {
      setPixel(x, SOIL_ROW - 2, daytime ? CRGB(235, 60, 120) : CRGB(50, 14, 34));  // Flowers close at night.
    } else if (progress >= 0.34f) {
      setPixel(x, SOIL_ROW - 2, CRGB(20, 130, 20));
    }
  }
  if (!daytime || grown == 0) return;  // The gardener goes home at night.

  // Walk to the plant, water it, then move on to the next one.
  const uint8_t target = gardenerTarget % grown;
  const float stopX = plantColumn(target) - 1;
  if (now >= wateringUntil) {
    if (gardenerWatering) {
      gardenerWatering = false;
      gardenerTarget = (target + 1) % grown;
    } else {
      const float step = GARDENER_WALK_PER_SECOND * dt;
      gardenerX = abs(stopX - gardenerX) <= step ? stopX : gardenerX + (stopX > gardenerX ? step : -step);
      if (gardenerX == stopX) {
        gardenerWatering = true;
        wateringUntil = now + GARDENER_WATER_MS;
      }
    }
  }
  if (gardenerWatering) setPixel(plantColumn(target), SOIL_ROW - 2 + (now / 140) % 2, CRGB(40, 120, 255));
  const uint8_t x = roundf(gardenerX);
  setPixel(x, SOIL_ROW - 1, CRGB(50, 100, 230));
  setPixel(x, SOIL_ROW - 2, CRGB(255, 200, 150));
}

// ---- Shared by the scene faces -------------------------------------------------------

void drawTime(const ClockTime &clock, const uint8_t digits[4], uint8_t top, CRGB color, const Palette &palette) {
  drawColon(clock.millisecond, palette, top + 1, top + 3);
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], top, color);
}

CRGB amberFor(const Palette &palette) { return &palette == &NIGHT ? CRGB(170, 42, 0) : CRGB(255, 120, 0); }

// ---- Code rain -------------------------------------------------------------------------

void respawnStream(uint8_t x) {
  codeHead[x] = -float(random8(10));
  codeRate[x] = 4.0f + random8(7);
}

void drawCodeRain(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const float dt = frameSeconds(now);
  for (uint8_t x = 0; x < COLS; x++) {
    codeHead[x] += codeRate[x] * dt;
    if (codeHead[x] > ROWS + CODE_TRAIL) respawnStream(x);
    for (uint8_t y = 0; y < ROWS; y++) {
      const float distance = codeHead[x] - y;
      if (distance < 0 || distance >= CODE_TRAIL) continue;
      const float strength = 1.0f - distance / CODE_TRAIL;
      setPixel(x, y, CRGB(0, uint8_t(12 + strength * 90), uint8_t(strength * 18)));
    }
  }
  drawTime(clock, digits, 1, palette.digit, palette);
}

// ---- Campfire --------------------------------------------------------------------------

void drawCampfire(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  static constexpr uint8_t ROW_HEAT[3] = {130, 205, 255};
  for (uint8_t y = FIRE_TOP; y < ROWS; y++) {
    for (uint8_t x = 0; x < COLS; x++) {
      const uint8_t noise = inoise8(x * 48, y * 74 + uint16_t(now / 2), uint16_t(now / 5));
      const uint8_t stretched = qadd8(qsub8(noise, 50), qsub8(noise, 50));
      setPixel(x, y, ColorFromPalette(HeatColors_p, scale8(scale8(stretched, ROW_HEAT[y - FIRE_TOP]), 225)));
    }
  }
  drawTime(clock, digits, 0, palette.digit, palette);
}

// ---- Aurora ------------------------------------------------------------------------------

void drawAurora(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  for (uint8_t x = 0; x < COLS; x++) {
    const uint8_t noise = inoise8(x * 30, uint16_t(now / 6));
    const int centre = noise * ROWS / 255;
    for (uint8_t y = 0; y < ROWS; y++) {
      const int distance = abs(int(y) - centre);
      if (distance > 3) continue;
      setPixel(x, y, CHSV(96 + scale8(noise, 100), 200, (3 - distance) * 20 + 10));
    }
  }
  drawTime(clock, digits, 1, palette.digit, palette);
}

// ---- Starfield ---------------------------------------------------------------------------

void respawnStar(Star &star) {
  const float angle = random8() * TWO_PI / 256.0f;
  const float speed = 2.0f + random8(60) / 10.0f;
  star.x = (COLS - 1) / 2.0f;
  star.y = (ROWS - 1) / 2.0f;
  star.vx = cosf(angle) * speed;
  star.vy = sinf(angle) * speed;
}

void drawStarfield(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const float dt = frameSeconds(now);
  const float centreX = (COLS - 1) / 2.0f;
  const float centreY = (ROWS - 1) / 2.0f;
  for (Star &star : stars) {
    const float distance = fabsf(star.x - centreX) + fabsf(star.y - centreY);
    star.x += star.vx * dt * (1.0f + distance * 0.35f);
    star.y += star.vy * dt * (1.0f + distance * 0.35f);
    if (star.x < 0 || star.x >= COLS || star.y < 0 || star.y >= ROWS) {
      respawnStar(star);
      continue;
    }
    const uint8_t level = min(110.0f, 18.0f + distance * 11.0f);
    setPixel(star.x, star.y, CRGB(level * 2 / 3, level * 3 / 4, level));
  }
  drawTime(clock, digits, 1, amberFor(palette), palette);
}

// ---- Rainbow digits ------------------------------------------------------------------------

void drawRainbow(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  drawColon(clock.millisecond, palette, 2, 4);
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t row = 0; row < DIGIT_HEIGHT; row++) {
      for (uint8_t col = 0; col < DIGIT_WIDTH; col++) {
        if (!digitPixel(digits[i], col, row)) continue;
        const uint8_t x = DIGIT_COLUMNS[i] + col;
        setPixel(x, 1 + row, CHSV(uint8_t(now / 14) + x * 14, 240, 255));
      }
    }
  }
}

// ---- Heartbeat -----------------------------------------------------------------------------

void drawHeartbeat(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const uint32_t step = now / HEARTBEAT_STEP_MS;
  for (uint8_t x = 0; x < COLS; x++) {
    const uint8_t y = HEARTBEAT_TRACE[(x + step) % HEARTBEAT_LENGTH];
    const uint8_t level = 40 + x * 13;
    setPixel(x, y, CRGB(level, level / 8, level / 5));
  }
  drawTime(clock, digits, 0, palette.digit, palette);
}

// ---- Fireworks -----------------------------------------------------------------------------

void launchFirework(uint32_t now) {
  nextFirework = now + 600 + random16(1400);
  for (Firework &firework : fireworks) {
    if (firework.active) continue;
    firework = {uint8_t(2 + random8(12)), uint8_t(1 + random8(5)), random8(), now, true};
    return;
  }
}

void drawFireworks(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  if (int32_t(now - nextFirework) >= 0) launchFirework(now);
  for (Firework &firework : fireworks) {
    if (!firework.active) continue;
    const float t = float(now - firework.start) / FIREWORK_MS;
    if (t >= 1.0f) {
      firework.active = false;
      continue;
    }
    const float radius = t * FIREWORK_RADIUS;
    const CRGB spark = CHSV(firework.hue, 200, uint8_t((1.0f - t) * 170));
    for (uint8_t i = 0; i < 8; i++) {
      setPixel(roundf(firework.x + FIREWORK_DX[i] * radius), roundf(firework.y + FIREWORK_DY[i] * radius), spark);
    }
  }
  drawTime(clock, digits, 1, palette.digit, palette);
}

// ---- Ocean voyage --------------------------------------------------------------------------

// The boat crosses the sea once an hour.
void drawOcean(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  for (uint8_t x = 0; x < COLS; x++) {
    const bool crest = sin8(x * 26 + now / 9) > 150;
    if (crest) setPixel(x, SEA_TOP - 1, CRGB(30, 90, 190));
    for (uint8_t y = SEA_TOP; y < ROWS; y++) setPixel(x, y, y == SEA_TOP ? CRGB(10, 50, 150) : CRGB(0, 20, 90));
  }
  const uint8_t boat = clock.minute * (COLS - 3) / 59;
  for (uint8_t i = 0; i < 3; i++) setPixel(boat + i, SEA_TOP, CRGB(150, 70, 16));
  setPixel(boat + 1, SEA_TOP - 1, CRGB(235, 235, 225));
  drawTime(clock, digits, 0, palette.digit, palette);
}

// ---- Aquarium -------------------------------------------------------------------------------

void respawnBubble(Drop &bubble) {
  bubble.x = random8(COLS);
  bubble.y = ROWS + random8(8);
  bubble.speed = 2.0f + random8(30) / 10.0f;
}

void drawAquarium(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const float dt = frameSeconds(now);
  for (uint8_t y = TANK_TOP; y < ROWS; y++) {
    for (uint8_t x = 0; x < COLS; x++) setPixel(x, y, CRGB(0, 7, 28));
  }
  for (uint8_t x : {2, 13}) {
    setPixel(x, ROWS - 1, CRGB(10, 100, 24));
    setPixel(x + (sin8(now / 6 + x * 40) > 128 ? 1 : 0), ROWS - 2, CRGB(10, 100, 24));
  }
  for (Drop &bubble : bubbles) {
    bubble.y -= bubble.speed * dt;
    if (bubble.y < 0) {
      respawnBubble(bubble);
    } else if (bubble.y < ROWS) {
      setPixel(bubble.x, uint8_t(bubble.y), CRGB(20, 60, 120));
    }
  }
  drawTime(clock, digits, 0, palette.digit, palette);
  for (Fish &swimmer : fish) {
    swimmer.x += swimmer.direction * swimmer.speed * dt;
    if (swimmer.x < 1 || swimmer.x > COLS - 2) {
      swimmer.x = constrain(swimmer.x, 1.0f, float(COLS - 2));
      swimmer.direction = -swimmer.direction;
    }
    const int x = roundf(swimmer.x);
    CRGB tail = swimmer.color;
    tail.nscale8(120);
    setPixel(x, swimmer.y, swimmer.color);
    setPixel(x - swimmer.direction, swimmer.y, tail);
  }
}

// ---- Marquee --------------------------------------------------------------------------------

void drawMarquee(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const uint32_t phase = now / MARQUEE_STEP_MS;
  CRGB dim = palette.band[0];
  dim.nscale8(30);
  for (uint8_t x = 0; x < COLS; x++) {
    setPixel(x, 0, (x + phase) % 3 == 0 ? palette.band[1] : dim);
    setPixel(x, ROWS - 1, (COLS - 1 - x + phase) % 3 == 0 ? palette.band[1] : dim);
  }
  drawTime(clock, digits, 1, palette.digit, palette);
}

// ---- Choosing a face -----------------------------------------------------------------

void resetSceneFaces(uint32_t now) {
  for (uint8_t x = 0; x < COLS; x++) respawnStream(x);
  for (Star &star : stars) {
    respawnStar(star);
    const float head = random8(100) / 100.0f;
    star.x += star.vx * head;
    star.y += star.vy * head;
  }
  for (Firework &firework : fireworks) firework.active = false;
  nextFirework = now;
  fish[0] = {3.0f, 2.6f, 1, 5, CRGB(255, 90, 0)};
  fish[1] = {10.0f, 1.8f, -1, 6, CRGB(255, 200, 30)};
  fish[2] = {6.0f, 3.4f, 1, 7, CRGB(255, 50, 90)};
  for (Drop &bubble : bubbles) respawnBubble(bubble);
}

void resetAnimations(const ClockTime &clock, uint32_t now) {
  lastAnimation = now;
  resetSceneFaces(now);
  wordWidth = 0;
  wordText[0] = 0;
  gardenerX = 0;
  gardenerTarget = 0;
  gardenerWatering = false;
  wateringUntil = 0;
  waterLevel = clock.minute * WATER_CELLS / 60.0f;
  splashX = -1;
  for (Drop &drop : rain) respawnDrop(drop);
  memset(snowRest, 0, sizeof(snowRest));
  snowBurstCount = 0;
  for (Flake &flake : flakes) respawnFlake(flake, now);
  chaseStomping = false;
  chaseX = 0;
  chaseDirection = 1;
}

void activateFace(uint8_t face, const ClockTime &clock, const uint8_t digits[4], uint32_t now) {
  activeFace = face;
  resetDigitChange(digits);
  resetAnimations(clock, now);
  if (face >= FACE_COMPLEX_FIRST) complexFaceReset(face - FACE_COMPLEX_FIRST, clock, now);
  digitSandActive = false;
  draining = false;
  resetSand(clock.minute / MINUTES_PER_GRAIN);
  lastHour = clock.hour;
  lastTick = now;
}

uint8_t pickDifferentFace(uint8_t current) {
  uint8_t face = random8(FACE_COUNT - 1);
  return face >= current ? face + 1 : face;
}

void chooseFace(uint8_t selection, const ClockTime &clock, const uint8_t digits[4], uint32_t now) {
  if (selection != lastSelection) {
    lastSelection = selection;
    shuffleHour = clock.hour;
    const uint8_t face = selection == FACE_SHUFFLE ? (activeFace < FACE_COUNT ? pickDifferentFace(activeFace) : random8(FACE_COUNT)) : selection;
    activateFace(face, clock, digits, now);
    return;
  }
  const bool settled = clock.minute > 0 || clock.second >= SHUFFLE_DELAY_SECONDS;
  if (selection == FACE_SHUFFLE && clock.hour != shuffleHour && settled) {
    shuffleHour = clock.hour;
    activateFace(pickDifferentFace(activeFace), clock, digits, now);
  }
}

}  // namespace

void facesBegin() {
  buildSlots();
  resetSand(0);
  for (uint8_t &value : jitter) value = random8(219, 255);
}

void facesUpdate(uint8_t selection, uint8_t brightnessPercent, const ClockTime &clock, uint32_t now) {
  uint8_t digits[4];
  splitDigits(clock, digits);
  chooseFace(selection < FACE_COUNT || selection == FACE_SHUFFLE ? selection : FACE_SANDBAR, clock, digits, now);

  const Palette &palette = paletteFor(clock.hour);
  // Night keeps the same proportion of the daytime brightness as the two defaults in config.h.
  const uint8_t dayLevel = brightnessPercent * 255 / 100;
  const uint8_t nightLevel = max(3, dayLevel * NIGHT_BRIGHTNESS / DAY_BRIGHTNESS);
  displayClear(&palette == &NIGHT ? nightLevel : dayLevel);
  switch (activeFace) {
    case FACE_DIGIT_SAND:
      drawDigitSand(clock, digits, now, palette);
      break;
    case FACE_BIG_DIGITS:
      resetDigitChange(digits);
      drawBigDigits(clock, digits, palette);
      break;
    case FACE_SPLIT_FLAP:
      drawSplitFlap(clock, digits, now, palette);
      break;
    case FACE_SUN_MOON:
      resetDigitChange(digits);
      drawSunAndMoon(clock, digits, now, palette);
      break;
    case FACE_ROLLING:
      drawRolling(clock, digits, now, palette);
      break;
    case FACE_RAINDROPS:
      resetDigitChange(digits);
      drawRaindrops(clock, digits, now, palette);
      break;
    case FACE_LAVA:
      resetDigitChange(digits);
      drawLava(clock, digits, now);
      break;
    case FACE_SNOW:
      drawSnow(clock, digits, now, palette);
      break;
    case FACE_CITY:
      resetDigitChange(digits);
      drawCity(clock, digits, now, palette);
      break;
    case FACE_CHASE:
      drawChase(clock, digits, now, palette);
      break;
    case FACE_SECONDS_BAR:
      resetDigitChange(digits);
      drawSecondsBar(clock, digits, palette);
      break;
    case FACE_BINARY:
      resetDigitChange(digits);
      drawBinary(clock, digits, palette);
      break;
    case FACE_TALLY:
      resetDigitChange(digits);
      drawTally(clock, digits, now, palette);
      break;
    case FACE_WORDS:
      resetDigitChange(digits);
      drawWords(clock, now, palette);
      break;
    case FACE_GARDENER:
      resetDigitChange(digits);
      drawGardener(clock, digits, now, palette);
      break;
    case FACE_CODE_RAIN:
      resetDigitChange(digits);
      drawCodeRain(clock, digits, now, palette);
      break;
    case FACE_CAMPFIRE:
      resetDigitChange(digits);
      drawCampfire(clock, digits, now, palette);
      break;
    case FACE_AURORA:
      resetDigitChange(digits);
      drawAurora(clock, digits, now, palette);
      break;
    case FACE_STARFIELD:
      resetDigitChange(digits);
      drawStarfield(clock, digits, now, palette);
      break;
    case FACE_RAINBOW:
      resetDigitChange(digits);
      drawRainbow(clock, digits, now, palette);
      break;
    case FACE_HEARTBEAT:
      resetDigitChange(digits);
      drawHeartbeat(clock, digits, now, palette);
      break;
    case FACE_FIREWORKS:
      resetDigitChange(digits);
      drawFireworks(clock, digits, now, palette);
      break;
    case FACE_OCEAN:
      resetDigitChange(digits);
      drawOcean(clock, digits, now, palette);
      break;
    case FACE_AQUARIUM:
      resetDigitChange(digits);
      drawAquarium(clock, digits, now, palette);
      break;
    case FACE_MARQUEE:
      resetDigitChange(digits);
      drawMarquee(clock, digits, now, palette);
      break;
    default:
      resetDigitChange(digits);
      if (activeFace >= FACE_COMPLEX_FIRST) {
        complexFaceDraw(activeFace - FACE_COMPLEX_FIRST, clock, digits, now, palette);
      } else {
        drawSandbar(clock, digits, now, palette);
      }
      break;
  }
  displayShow();
}
