#include "faces_complex.h"

#include <initializer_list>
#include <math.h>
#include <string.h>

#include "live_data.h"

namespace {

constexpr uint8_t MAX_PARTICLES = 24;
constexpr int8_t DX[4] = {1, -1, 0, 0};
constexpr int8_t DY[4] = {0, 0, 1, -1};

struct Particle {
  float x;
  float y;
  float vx;
  float vy;
  uint8_t a;
  uint8_t b;
};

// Only one face runs at a time, so they all share this working memory.
uint8_t grid[CELLS];
uint8_t grid2[CELLS];
float waveNow[CELLS];
float wavePrev[CELLS];
bool mask[CELLS];
Particle parts[MAX_PARTICLES];
int state[8];
float value[4];
uint32_t timer[4];
uint32_t lastFrame = 0;
uint8_t previousDigits[4];
uint8_t path[64];

uint16_t at(int x, int y) { return y * COLS + x; }
bool inB(int x, int y) { return x >= 0 && y >= 0 && x < COLS && y < ROWS; }

float frameDt(uint32_t now) {
  const uint32_t elapsed = min(now - lastFrame, uint32_t(50));
  lastFrame = now;
  return elapsed / 1000.0f;
}

bool every(uint8_t slot, uint32_t ms, uint32_t now) {
  if (now - timer[slot] < ms) return false;
  timer[slot] = now;
  return true;
}

void drawTime(const ClockTime &clock, const uint8_t digits[4], uint8_t top, CRGB color, const Palette &palette) {
  drawColon(clock.millisecond, palette, top + 1, top + 3);
  for (uint8_t i = 0; i < 4; i++) drawDigit(digits[i], DIGIT_COLUMNS[i], top, color);
}

CRGB amber(const Palette &palette) { return &palette == &NIGHT ? CRGB(170, 42, 0) : CRGB(255, 120, 0); }

CRGB scaled(CRGB color, uint8_t amount) {
  color.nscale8(amount);
  return color;
}

void buildMask(const uint8_t digits[4], uint8_t top) {
  memset(mask, 0, sizeof(mask));
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t row = 0; row < DIGIT_HEIGHT; row++) {
      for (uint8_t col = 0; col < DIGIT_WIDTH; col++) {
        if (digitPixel(digits[i], col, row)) mask[at(DIGIT_COLUMNS[i] + col, top + row)] = true;
      }
    }
  }
}

bool digitsChanged(const uint8_t digits[4]) {
  const bool changed = memcmp(previousDigits, digits, 4) != 0;
  return changed;
}

void drawLine(float x0, float y0, float x1, float y1, CRGB color) {
  const float steps = max(fabsf(x1 - x0), fabsf(y1 - y0)) * 2 + 1;
  for (int i = 0; i <= int(steps); i++) {
    const float t = i / steps;
    setPixel(roundf(x0 + (x1 - x0) * t), roundf(y0 + (y1 - y0) * t), color);
  }
}

void randomNeighbour(int x, int y, int &nx, int &ny) {
  const uint8_t d = random8(4);
  nx = (x + DX[d] + COLS) % COLS;
  ny = (y + DY[d] + ROWS) % ROWS;
}

void drawCreatureSprite(int x, int top, bool stepping, CRGB body, CRGB eye) {
  setPixel(x + 1, top, body);
  setPixel(x, top + 1, body);
  setPixel(x + 1, top + 1, body);
  setPixel(x + 2, top + 1, body);
  setPixel(x + 2, top + 1, eye);
  if (stepping) {
    setPixel(x, top + 2, body);
    setPixel(x + 2, top + 2, body);
  } else {
    setPixel(x + 1, top + 2, body);
  }
}

// ---- 0: Ant colony ------------------------------------------------------------------

constexpr uint8_t SOIL_TOP = 5;
constexpr uint8_t SOIL_CELLS = COLS * (ROWS - SOIL_TOP);

void antsReset() {
  memset(grid, 0, sizeof(grid));
  int count = 1;
  grid[at(random8(COLS), SOIL_TOP)] = 1;
  while (count < SOIL_CELLS) {
    int chosen = -1;
    uint8_t seen = 0;
    for (int y = SOIL_TOP; y < ROWS; y++) {
      for (int x = 0; x < COLS; x++) {
        if (grid[at(x, y)]) continue;
        bool touching = false;
        for (uint8_t d = 0; d < 4; d++) {
          const int nx = x + DX[d], ny = y + DY[d];
          if (inB(nx, ny) && ny >= SOIL_TOP && grid[at(nx, ny)]) touching = true;
        }
        if (touching && random8(++seen) == 0) chosen = at(x, y);
      }
    }
    if (chosen < 0) break;
    grid[chosen] = ++count;
  }
  for (uint8_t i = 0; i < 4; i++) {
    parts[i].x = random8(COLS);
    parts[i].y = SOIL_TOP;
  }
}

void ants(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const int dug = 1 + (clock.hour * 60 + clock.minute) * (SOIL_CELLS - 1) / 1440;
  for (int y = SOIL_TOP; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      setPixel(x, y, grid[at(x, y)] && grid[at(x, y)] <= dug ? CRGB::Black : CRGB(48, 22, 4));
    }
  }
  const bool move = every(0, 180, now);
  for (uint8_t i = 0; i < 4; i++) {
    if (move) {
      const uint8_t d = random8(4);
      const int nx = parts[i].x + DX[d], ny = parts[i].y + DY[d];
      if (inB(nx, ny) && ny >= SOIL_TOP && grid[at(nx, ny)] && grid[at(nx, ny)] <= dug) {
        parts[i].x = nx;
        parts[i].y = ny;
      }
    }
    setPixel(parts[i].x, parts[i].y, CRGB(230, 70, 20));
  }
  drawTime(clock, digits, 0, palette.digit, palette);
}

// ---- 1: Predator and prey ---------------------------------------------------------------

void preySeed() {
  for (uint16_t i = 0; i < CELLS; i++) {
    const uint8_t r = random8(10);
    grid[i] = r < 3 ? 1 : r < 4 ? 2 : 0;
  }
}

void prey(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  if (every(0, 220, now)) {
    uint8_t rabbits = 0, foxes = 0;
    for (int n = 0; n < CELLS; n++) {
      const int x = random8(COLS), y = random8(ROWS);
      int nx, ny;
      randomNeighbour(x, y, nx, ny);
      uint8_t &here = grid[at(x, y)];
      uint8_t &there = grid[at(nx, ny)];
      if (here == 1 && there == 0 && random8(3) == 0) {
        there = 1;
      } else if (here == 2) {
        if (there == 1) {
          there = random8(3) == 0 ? 2 : 0;
        } else if (random8(9) == 0) {
          here = 0;
        } else if (there == 0) {
          there = 2;
          here = 0;
        }
      }
    }
    for (uint16_t i = 0; i < CELLS; i++) {
      rabbits += grid[i] == 1;
      foxes += grid[i] == 2;
    }
    if (rabbits == 0 || foxes == 0) preySeed();
  }
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      const uint8_t cell = grid[at(x, y)];
      if (cell) setPixel(x, y, cell == 1 ? CRGB(0, 45, 12) : CRGB(120, 30, 0));
    }
  }
  drawTime(clock, digits, 1, palette.digit, palette);
}

// ---- 2: Falling sand ----------------------------------------------------------------------

void fallingSand(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  buildMask(digits, 2);
  for (uint16_t i = 0; i < CELLS; i++) {
    if (mask[i]) grid[i] = 0;
  }
  if (every(0, 70, now)) {
    if (random8(3) == 0) {
      const uint16_t top = at(random8(COLS), 0);
      if (!grid[top]) grid[top] = random8(2) ? 1 : 2;
    }
    for (int y = ROWS - 1; y >= 0; y--) {
      for (int i = 0; i < COLS; i++) {
        const int x = y % 2 ? i : COLS - 1 - i;
        const uint16_t here = at(x, y);
        const uint8_t kind = grid[here];
        if (!kind) continue;
        if (y == ROWS - 1) {
          if (random8(14) == 0) grid[here] = 0;
          continue;
        }
        const int side = random8(2) ? 1 : -1;
        const int options[5][2] = {{x, y + 1}, {x + side, y + 1}, {x - side, y + 1}, {x + side, y}, {x - side, y}};
        const uint8_t limit = kind == 1 ? 3 : 5;
        for (uint8_t o = 0; o < limit; o++) {
          const int nx = options[o][0], ny = options[o][1];
          if (inB(nx, ny) && !mask[at(nx, ny)] && !grid[at(nx, ny)]) {
            grid[at(nx, ny)] = kind;
            grid[here] = 0;
            break;
          }
        }
      }
    }
  }
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      const uint8_t cell = grid[at(x, y)];
      if (cell) setPixel(x, y, cell == 1 ? CRGB(95, 60, 12) : CRGB(8, 30, 120));
    }
  }
  drawTime(clock, digits, 2, palette.digit, palette);
}

// ---- 3: Life ---------------------------------------------------------------------------------

void lifeSeed(const uint8_t digits[4]) {
  for (uint16_t i = 0; i < CELLS; i++) grid[i] = random8(4) == 0;
  buildMask(digits, 1);
  for (uint16_t i = 0; i < CELLS; i++) {
    if (mask[i]) grid[i] = 1;
  }
  state[0] = 0;
}

void life(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  if (digitsChanged(digits)) lifeSeed(digits);
  if (every(0, 350, now)) {
    uint8_t alive = 0;
    for (int y = 0; y < ROWS; y++) {
      for (int x = 0; x < COLS; x++) {
        uint8_t n = 0;
        for (int dy = -1; dy <= 1; dy++) {
          for (int dx = -1; dx <= 1; dx++) {
            if (dx || dy) n += grid[at((x + dx + COLS) % COLS, (y + dy + ROWS) % ROWS)];
          }
        }
        grid2[at(x, y)] = n == 3 || (n == 2 && grid[at(x, y)]);
        alive += grid2[at(x, y)];
      }
    }
    memcpy(grid, grid2, sizeof(grid));
    if (alive < 5 || ++state[0] > 50) lifeSeed(digits);
  }
  for (uint16_t y = 0; y < ROWS; y++) {
    for (uint16_t x = 0; x < COLS; x++) {
      if (grid[at(x, y)]) setPixel(x, y, CRGB(0, 48, 38));
    }
  }
  drawTime(clock, digits, 1, palette.digit, palette);
}

// ---- 4: Flocking birds ------------------------------------------------------------------------

constexpr uint8_t BIRDS = 12;

void birdsReset() {
  for (uint8_t i = 0; i < BIRDS; i++) {
    parts[i] = {float(random8(COLS)), float(random8(ROWS)), float(random8(40)) / 10.0f - 2, float(random8(40)) / 10.0f - 2, 0, 0};
  }
}

void birds(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const float dt = frameDt(now);
  buildMask(digits, 1);
  float centreX = 0, centreY = 0, averageVx = 0, averageVy = 0;
  for (uint8_t i = 0; i < BIRDS; i++) {
    centreX += parts[i].x / BIRDS;
    centreY += parts[i].y / BIRDS;
    averageVx += parts[i].vx / BIRDS;
    averageVy += parts[i].vy / BIRDS;
  }
  for (uint8_t i = 0; i < BIRDS; i++) {
    Particle &bird = parts[i];
    bird.vx += ((centreX - bird.x) * 0.6f + (averageVx - bird.vx) * 0.5f) * dt * 2;
    bird.vy += ((centreY - bird.y) * 0.6f + (averageVy - bird.vy) * 0.5f) * dt * 2;
    for (uint8_t j = 0; j < BIRDS; j++) {
      if (i != j && fabsf(bird.x - parts[j].x) + fabsf(bird.y - parts[j].y) < 1.5f) {
        bird.vx += (bird.x - parts[j].x) * dt * 6;
        bird.vy += (bird.y - parts[j].y) * dt * 6;
      }
    }
    const float speed = sqrtf(bird.vx * bird.vx + bird.vy * bird.vy) + 0.01f;
    bird.vx = bird.vx / speed * 4.5f;
    bird.vy = bird.vy / speed * 3.0f;
    bird.x += bird.vx * dt;
    bird.y += bird.vy * dt;
    if (bird.x < 0) bird.x += COLS;
    if (bird.x >= COLS) bird.x -= COLS;
    if (bird.y < 0) bird.y += ROWS;
    if (bird.y >= ROWS) bird.y -= ROWS;
    setPixel(bird.x, bird.y, CRGB(40, 70, 150));
  }
  drawTime(clock, digits, 1, amber(palette), palette);
}

// ---- 5: Forest fire ------------------------------------------------------------------------------

void forestFire(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  if (every(0, 130, now)) {
    for (int y = 0; y < ROWS; y++) {
      for (int x = 0; x < COLS; x++) {
        const uint8_t cell = grid[at(x, y)];
        uint8_t next = cell;
        if (cell == 2) {
          next = 0;
        } else if (cell == 1) {
          bool burning = random16(3000) == 0;
          for (uint8_t d = 0; d < 4; d++) {
            const int nx = x + DX[d], ny = y + DY[d];
            if (inB(nx, ny) && grid[at(nx, ny)] == 2 && random8(4) != 0) burning = true;
          }
          if (burning) next = 2;
        } else if (random8(45) == 0) {
          next = 1;
        }
        grid2[at(x, y)] = next;
      }
    }
    memcpy(grid, grid2, sizeof(grid));
  }
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      const uint8_t cell = grid[at(x, y)];
      if (cell) setPixel(x, y, cell == 1 ? CRGB(0, 42, 6) : CRGB(210, 70, 0));
    }
  }
  drawTime(clock, digits, 1, palette.digit, palette);
}

// ---- 6: Ripple tank --------------------------------------------------------------------------------

void ripple(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  buildMask(digits, 1);
  if (state[0] != clock.second) {
    state[0] = clock.second;
    waveNow[at(random8(COLS), random8(ROWS))] = 3.0f;
  }
  if (every(0, 45, now)) {
    for (int y = 0; y < ROWS; y++) {
      for (int x = 0; x < COLS; x++) {
        const float left = x > 0 ? waveNow[at(x - 1, y)] : 0;
        const float right = x < COLS - 1 ? waveNow[at(x + 1, y)] : 0;
        const float up = y > 0 ? waveNow[at(x, y - 1)] : 0;
        const float down = y < ROWS - 1 ? waveNow[at(x, y + 1)] : 0;
        wavePrev[at(x, y)] = mask[at(x, y)] ? 0 : ((left + right + up + down) / 2 - wavePrev[at(x, y)]) * 0.94f;
      }
    }
    memcpy(grid2, grid, 0);
    for (uint16_t i = 0; i < CELLS; i++) {
      const float held = waveNow[i];
      waveNow[i] = wavePrev[i];
      wavePrev[i] = held;
    }
  }
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      const float level = constrain(fabsf(waveNow[at(x, y)]), 0.0f, 1.0f);
      setPixel(x, y, CRGB(0, uint8_t(level * 80), uint8_t(8 + level * 150)));
    }
  }
  drawTime(clock, digits, 1, palette.digit, palette);
}

// ---- 7: Crystal growth ----------------------------------------------------------------------------

void growCrystal() {
  int chosen = -1;
  uint8_t seen = 0;
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      if (mask[at(x, y)] || grid[at(x, y)]) continue;
      bool touching = false;
      for (uint8_t d = 0; d < 4; d++) {
        const int nx = x + DX[d], ny = y + DY[d];
        if (inB(nx, ny) && (mask[at(nx, ny)] || grid[at(nx, ny)])) touching = true;
      }
      if (touching && random8(++seen) == 0) chosen = at(x, y);
    }
  }
  if (chosen >= 0) grid[chosen] = 1;
}

void crystal(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  buildMask(digits, 1);
  if (state[1] != clock.hour) {
    state[1] = clock.hour;
    memset(grid, 0, sizeof(grid));
    state[0] = 0;
  }
  const int target = clock.minute * 2;
  if (state[0] < target && every(0, 200, now)) {
    growCrystal();
    state[0]++;
  }
  for (uint16_t i = 0; i < CELLS; i++) {
    if (grid[i]) setPixel(i % COLS, i / COLS, CRGB(14, 46, 120));
  }
  drawTime(clock, digits, 1, amber(palette), palette);
}

// ---- 8: Paddle duel --------------------------------------------------------------------------------

void paddleDuel(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const float dt = frameDt(now);
  float &ballX = value[0], &ballY = value[1], &vx = value[2], &vy = value[3];
  ballX += vx * dt;
  ballY += vy * dt;
  if (ballY < 5) {
    ballY = 5;
    vy = fabsf(vy);
  }
  if (ballY > 7) {
    ballY = 7;
    vy = -fabsf(vy);
  }
  if (ballX < 1) {
    ballX = 1;
    vx = fabsf(vx);
  }
  if (ballX > COLS - 2) {
    ballX = COLS - 2;
    vx = -fabsf(vx);
  }
  setPixel(0, roundf(ballY), palette.band[1]);
  setPixel(COLS - 1, roundf(ballY), palette.band[1]);
  setPixel(roundf(ballX), roundf(ballY), CRGB(255, 255, 255));
  drawTime(clock, digits, 0, palette.digit, palette);
}

// ---- 9: Brick smasher -------------------------------------------------------------------------------

void brickSmasher(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  buildMask(digits, 0);
  int &ballX = state[0], &ballY = state[1], &vx = state[2], &vy = state[3];
  if (every(0, 75, now)) {
    for (uint16_t i = 0; i < CELLS; i++) {
      if (grid[i]) grid[i]--;
    }
    int nx = ballX + vx, ny = ballY + vy;
    if (nx < 0 || nx >= COLS) {
      vx = -vx;
      nx = ballX + vx;
    }
    if (ny < 0) {
      vy = 1;
      ny = ballY + vy;
    }
    if (ny >= ROWS - 1) {
      vy = -1;
      ny = ballY + vy;
    }
    if (inB(nx, ny) && mask[at(nx, ny)] && grid[at(nx, ny)] == 0) {
      grid[at(nx, ny)] = 180;
      vy = -vy;
      ny = ballY + vy;
    }
    ballX = constrain(nx, 0, COLS - 1);
    ballY = constrain(ny, 0, ROWS - 2);
  }
  drawColon(clock.millisecond, palette, 1, 3);
  for (uint16_t i = 0; i < CELLS; i++) {
    if (mask[i]) setPixel(i % COLS, i / COLS, grid[i] ? scaled(palette.band[0], 70) : palette.digit);
  }
  const int paddle = constrain(ballX - 1, 0, COLS - 3);
  for (int x = paddle; x < paddle + 3; x++) setPixel(x, ROWS - 1, palette.band[1]);
  setPixel(ballX, ballY, CRGB(90, 220, 255));
}

// ---- 10: Runner ---------------------------------------------------------------------------------------

void runner(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const float dt = frameDt(now);
  for (uint8_t x = 0; x < COLS; x++) setPixel(x, ROWS - 1, (x + now / 110) % 4 == 0 ? CRGB(60, 34, 8) : CRGB(30, 16, 4));
  bool jumping = false;
  for (uint8_t i = 0; i < 2; i++) {
    parts[i].x -= 7.0f * dt;
    if (parts[i].x < -1) parts[i].x = COLS + random8(10);
    if (parts[i].x > 3.0f && parts[i].x < 6.5f) jumping = true;
    setPixel(roundf(parts[i].x), ROWS - 2, CRGB(30, 140, 30));
  }
  const uint8_t y = jumping ? ROWS - 3 : ROWS - 2;
  setPixel(3, y, palette.band[1]);
  drawTime(clock, digits, 0, palette.digit, palette);
}

// ---- 11: Space defender ------------------------------------------------------------------------------

void defender(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  uint8_t &alive = *reinterpret_cast<uint8_t *>(&state[0]);
  int &offset = state[1], &bulletX = state[2], &bulletY = state[3], &shipX = state[4], &shipDir = state[5];
  if (alive == 0) alive = 0b11111;
  if (every(0, 420, now)) offset = (offset + 1) % 4;
  if (every(1, 130, now)) {
    shipX += shipDir;
    if (shipX <= 0 || shipX >= COLS - 3) shipDir = -shipDir;
  }
  if (bulletY < 0 && every(2, 600, now)) {
    bulletX = shipX + 1;
    bulletY = ROWS - 2;
  }
  if (bulletY >= 0 && every(3, 55, now)) bulletY--;
  for (uint8_t i = 0; i < 5; i++) {
    if (!(alive & (1 << i))) continue;
    const int x = i * 3 + offset;
    if (bulletY == 5 && bulletX == x) {
      alive &= ~(1 << i);
      bulletY = -1;
    }
    setPixel(x, 5, CRGB(170, 40, 200));
  }
  if (bulletY >= 5) setPixel(bulletX, bulletY, CRGB(255, 240, 120));
  for (int x = shipX; x < shipX + 3; x++) setPixel(x, ROWS - 1, CRGB(40, 170, 90));
  setPixel(shipX + 1, ROWS - 2, CRGB(40, 170, 90));
  drawTime(clock, digits, 0, palette.digit, palette);
}

// ---- 12: Maze solver ------------------------------------------------------------------------------------

constexpr uint8_t MAZE_W = 8;
constexpr uint8_t MAZE_H = 4;
constexpr uint32_t MAZE_SHOW_MS = 9000;
constexpr uint32_t MAZE_CYCLE_MS = 14000;

void mazeGenerate() {
  memset(grid, 1, sizeof(grid));
  uint8_t stack[MAZE_W * MAZE_H];
  uint8_t depth = 0;
  stack[depth++] = 0;
  grid[at(0, 0)] = 0;
  while (depth) {
    const uint8_t cell = stack[depth - 1];
    const int cx = cell % MAZE_W, cy = cell / MAZE_W;
    uint8_t options[4];
    uint8_t count = 0;
    for (uint8_t d = 0; d < 4; d++) {
      const int nx = cx + DX[d], ny = cy + DY[d];
      if (nx >= 0 && ny >= 0 && nx < MAZE_W && ny < MAZE_H && grid[at(nx * 2, ny * 2)]) options[count++] = d;
    }
    if (!count) {
      depth--;
      continue;
    }
    const uint8_t d = options[random8(count)];
    const int nx = cx + DX[d], ny = cy + DY[d];
    grid[at(cx * 2 + DX[d], cy * 2 + DY[d])] = 0;
    grid[at(nx * 2, ny * 2)] = 0;
    stack[depth++] = ny * MAZE_W + nx;
  }

  // Breadth-first search from the top-left corner to the far corner.
  memset(grid2, 255, sizeof(grid2));
  uint8_t queue[CELLS];
  uint16_t head = 0, tail = 0;
  queue[tail++] = at(0, 0);
  grid2[at(0, 0)] = 4;
  const uint16_t goal = at(14, 6);
  while (head < tail && grid2[goal] == 255) {
    const uint16_t here = queue[head++];
    for (uint8_t d = 0; d < 4; d++) {
      const int nx = here % COLS + DX[d], ny = here / COLS + DY[d];
      if (inB(nx, ny) && !grid[at(nx, ny)] && grid2[at(nx, ny)] == 255) {
        grid2[at(nx, ny)] = d;
        queue[tail++] = at(nx, ny);
      }
    }
  }
  uint8_t length = 0;
  int x = 14, y = 6;
  while (grid2[at(x, y)] != 4 && length < sizeof(path)) {
    path[length++] = at(x, y);
    const uint8_t d = grid2[at(x, y)];
    x -= DX[d];
    y -= DY[d];
  }
  state[1] = length;
}

void maze(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const uint32_t phase = now - timer[0];
  if (phase >= MAZE_CYCLE_MS || state[2] == 0) {
    state[2] = 1;
    timer[0] = now;
    mazeGenerate();
    return;
  }
  if (phase >= MAZE_SHOW_MS) {
    drawTime(clock, digits, 1, palette.digit, palette);
    return;
  }
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      if (grid[at(x, y)]) setPixel(x, y, CRGB(10, 18, 70));
    }
  }
  const int revealed = min(uint32_t(state[1]), phase / 110);
  for (int i = 0; i < revealed; i++) {
    const uint8_t cell = path[state[1] - 1 - i];
    setPixel(cell % COLS, cell / COLS, palette.band[0]);
  }
  setPixel(14, 6, CRGB(40, 220, 60));
}

// ---- 13: Grazing worm --------------------------------------------------------------------------------------

constexpr uint8_t WORM_MAX = 22;

void wormReset() {
  state[0] = 3;
  for (uint8_t i = 0; i < WORM_MAX; i++) {
    grid[i] = 4;
    grid2[i] = 3;
  }
  state[1] = random8(COLS);
  state[2] = random8(ROWS);
}

void worm(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  int &length = state[0], &foodX = state[1], &foodY = state[2];
  length = min(WORM_MAX - 1, 3 + clock.minute / 4);
  if (every(0, 130, now)) {
    const int headX = grid[0], headY = grid2[0];
    const int stepX = foodX > headX ? 1 : foodX < headX ? -1 : 0;
    const int stepY = foodY > headY ? 1 : foodY < headY ? -1 : 0;
    const int options[4][2] = {{headX + stepX, headY}, {headX, headY + stepY}, {headX, headY + 1}, {headX + 1, headY}};
    int nx = headX, ny = headY;
    for (const auto &option : options) {
      if (!inB(option[0], option[1]) || (option[0] == headX && option[1] == headY)) continue;
      bool hit = false;
      for (int i = 0; i < length; i++) hit |= grid[i] == option[0] && grid2[i] == option[1];
      if (!hit) {
        nx = option[0];
        ny = option[1];
        break;
      }
    }
    if (nx == headX && ny == headY) {
      wormReset();
    } else {
      for (int i = WORM_MAX - 1; i > 0; i--) {
        grid[i] = grid[i - 1];
        grid2[i] = grid2[i - 1];
      }
      grid[0] = nx;
      grid2[0] = ny;
      if (nx == foodX && ny == foodY) {
        foodX = random8(COLS);
        foodY = random8(ROWS);
      }
    }
  }
  for (int i = length - 1; i >= 0; i--) setPixel(grid[i], grid2[i], i == 0 ? CRGB(60, 190, 70) : CRGB(0, 60, 22));
  setPixel(foodX, foodY, CRGB(170, 20, 30));
  drawTime(clock, digits, 1, palette.digit, palette);
}

// ---- 14: Block builder -----------------------------------------------------------------------------------------

constexpr uint16_t BLOCK_FALL_MS = 260;
constexpr uint16_t BLOCK_STAGGER_MS = 130;

void blockBuilder(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  if (digitsChanged(digits)) {
    timer[0] = now;
    for (uint8_t i = 0; i < 4; i++) state[i] = digits[i] != previousDigits[i];
  }
  drawColon(clock.millisecond, palette, 2, 4);
  const int32_t elapsed = now - timer[0];
  for (uint8_t i = 0; i < 4; i++) {
    for (uint8_t row = 0; row < DIGIT_HEIGHT; row++) {
      int y = 1 + row;
      if (state[i]) {
        const int32_t started = elapsed - int32_t(DIGIT_HEIGHT - 1 - row) * BLOCK_STAGGER_MS;
        if (started < 0) continue;
        if (started < BLOCK_FALL_MS) y = roundf(float(started) / BLOCK_FALL_MS * (y + 1)) - 1;
      }
      if (y < 0) continue;
      for (uint8_t col = 0; col < DIGIT_WIDTH; col++) {
        if (digitPixel(digits[i], col, row)) setPixel(DIGIT_COLUMNS[i] + col, y, palette.digit);
      }
    }
  }
}

// ---- 15: Day-to-night landscape ----------------------------------------------------------------------------------

CRGB skyColor(int minutes) {
  static const struct { int minute; CRGB color; } KEYS[] = {
      {0, CRGB(1, 2, 12)},     {300, CRGB(1, 2, 12)},   {390, CRGB(70, 28, 24)}, {480, CRGB(8, 30, 86)},
      {1020, CRGB(8, 30, 86)}, {1110, CRGB(70, 22, 30)}, {1200, CRGB(1, 2, 12)}, {1440, CRGB(1, 2, 12)},
  };
  for (uint8_t i = 0; i < 7; i++) {
    if (minutes < KEYS[i + 1].minute) {
      const int span = KEYS[i + 1].minute - KEYS[i].minute;
      return blend(KEYS[i].color, KEYS[i + 1].color, (minutes - KEYS[i].minute) * 255 / span);
    }
  }
  return KEYS[0].color;
}

void landscape(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  static constexpr uint8_t HILLS[COLS] = {6, 5, 5, 6, 7, 6, 5, 6, 7, 7, 6, 5, 6, 6, 5, 6};
  const int minutes = clock.hour * 60 + clock.minute;
  const CRGB sky = skyColor(minutes);
  const bool night = minutes < 345 || minutes >= 1155;
  for (uint8_t y = 0; y < ROWS; y++) {
    for (uint8_t x = 0; x < COLS; x++) setPixel(x, y, y >= HILLS[x] ? CRGB(4, 34, 8) : sky);
  }
  if (night) {
    for (uint8_t x : {1, 6, 11, 14}) setPixel(x, HILLS[x] + 1 < ROWS ? HILLS[x] + 1 : ROWS - 1, sin8(now / 8 + x * 40) > 90 ? CRGB(255, 170, 40) : CRGB(40, 24, 6));
  }
  drawTime(clock, digits, 0, palette.digit, palette);
}

// ---- 16: Train journey ------------------------------------------------------------------------------------------------

void train(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const uint8_t hours = clock.hour % 12 == 0 ? 12 : clock.hour % 12;
  const uint32_t scroll = now / 150;
  for (uint8_t x = 0; x < COLS; x++) {
    if ((x + scroll) % 7 == 0) setPixel(x, 5, CRGB(10, 90, 20));
    setPixel(x, ROWS - 1, (x + scroll) % 2 ? CRGB(30, 18, 6) : CRGB(52, 34, 16));
  }
  const int engine = COLS - 2;
  setPixel(engine, 6, CRGB(200, 30, 20));
  setPixel(engine + 1, 6, CRGB(235, 200, 60));
  if ((now / 200) % 2) setPixel(engine, 5, CRGB(60, 60, 70));
  for (uint8_t i = 0; i < hours; i++) {
    setPixel(engine - 1 - i, 6, i % 2 ? CRGB(30, 80, 190) : CRGB(50, 130, 220));
  }
  drawTime(clock, digits, 0, palette.digit, palette);
}

// ---- 17: Space mission ------------------------------------------------------------------------------------------------

void rocket(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  for (uint8_t x = 0; x < COLS; x++) setPixel(x, ROWS - 1, CRGB(16, 30, 12));
  const float minuteProgress = clock.minute + clock.second / 60.0f;
  float rocketX = 7.5f, rocketY = 5.0f;
  bool burning = false;
  if (minuteProgress < 3) {
    rocketY = 6 - minuteProgress / 3 * 6;
    burning = true;
  } else if (minuteProgress > 57) {
    rocketY = (minuteProgress - 57) / 3 * 6;
    burning = true;
  } else {
    const float orbit = (minuteProgress - 3) / 54 * 4;
    const float wrapped = orbit - int(orbit);
    rocketX = (int(orbit) % 2 ? 1 - wrapped : wrapped) * (COLS - 1);
    rocketY = 0;
  }
  drawTime(clock, digits, 2, palette.digit, palette);
  const int x = roundf(rocketX), y = roundf(rocketY);
  setPixel(x, y, CRGB(230, 230, 240));
  if (burning && y + 1 < ROWS - 1 && (now / 90) % 2) setPixel(x, y + 1, CRGB(255, 120, 10));
}

// ---- 18: Seasons -----------------------------------------------------------------------------------------------------

void seasons(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  static const CRGB CANOPY[4] = {CRGB(0, 0, 0), CRGB(210, 70, 120), CRGB(20, 120, 22), CRGB(190, 75, 8)};
  static const CRGB DRIFTING[4] = {CRGB(70, 90, 140), CRGB(120, 40, 70), CRGB(0, 0, 0), CRGB(110, 44, 6)};
  const float dt = frameDt(now);
  const uint8_t season = ((clock.month + 1) % 12) / 3;
  for (uint8_t i = 0; i < 8 && season != 2; i++) {
    parts[i].y += parts[i].vy * dt;
    if (parts[i].y >= ROWS) {
      parts[i].y = -float(random8(8));
      parts[i].x = random8(COLS);
    }
    if (parts[i].y >= 0) setPixel(parts[i].x, parts[i].y, DRIFTING[season]);
  }
  for (uint8_t tree : {1, 7, 13}) {
    setPixel(tree, 6, CRGB(55, 28, 8));
    setPixel(tree, 7, CRGB(55, 28, 8));
    if (season != 0) {
      for (int x = tree - 1; x <= tree + 1; x++) setPixel(x, 5, CANOPY[season]);
    }
  }
  drawTime(clock, digits, 0, palette.digit, palette);
}

// ---- 19: Pixel pet --------------------------------------------------------------------------------------------------

void pixelPet(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const bool asleep = clock.hour >= 22 || clock.hour < 7;
  const bool eating = (clock.hour == 8 || clock.hour == 13 || clock.hour == 18) && clock.minute < 10;
  const CRGB body = palette.band[0];
  float &x = value[0];
  float &direction = value[1];
  if (!asleep && !eating) {
    x += direction * frameDt(now) * 3.0f;
    if (x <= 0 || x >= COLS - 3) {
      x = constrain(x, 0.0f, float(COLS - 3));
      direction = -direction;
    }
  }
  const int petX = roundf(x);
  if (asleep) {
    setPixel(petX, 7, body);
    setPixel(petX + 1, 7, body);
    setPixel(petX + 2, 7, body);
    setPixel(petX + 1, 6, body);
    const uint8_t rise = (now / 500) % 3;
    setPixel(petX + 3 + rise, 6 - rise, CRGB(50, 70, 150));
  } else {
    drawCreatureSprite(petX, 5, int(now / 180) % 2, body, palette.digit);
    if (eating) setPixel(petX + 3, 7, (now / 300) % 2 ? CRGB(200, 40, 20) : CRGB(120, 20, 10));
    else setPixel(COLS - 1 - petX, 7 - abs(int((now / 120) % 4) - 2), CRGB(60, 150, 230));
  }
  drawTime(clock, digits, 0, palette.digit, palette);
}

// ---- 20: Little town ----------------------------------------------------------------------------------------------

void littleTown(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const int minutes = clock.hour * 60 + clock.minute;
  const bool atHome = minutes < 8 * 60 || minutes >= 18 * 60;
  float person = 0;
  if (minutes >= 8 * 60 && minutes < 9 * 60) person = (minutes - 8 * 60) / 60.0f;
  else if (minutes >= 9 * 60 && minutes < 17 * 60) person = 1;
  else if (minutes >= 17 * 60 && minutes < 18 * 60) person = 1 - (minutes - 17 * 60) / 60.0f;
  for (uint8_t y = 5; y < ROWS; y++) {
    for (uint8_t x = 0; x < 3; x++) setPixel(x, y, CRGB(60, 16, 12));
    for (uint8_t x = 13; x < COLS; x++) setPixel(x, y, CRGB(16, 22, 70));
  }
  setPixel(1, 6, atHome ? CRGB(255, 180, 60) : CRGB(30, 16, 4));
  setPixel(14, 6, person == 1 ? CRGB(255, 220, 120) : CRGB(14, 16, 34));
  for (uint8_t x = 3; x < 13; x++) setPixel(x, ROWS - 1, CRGB(14, 14, 16));
  setPixel(3 + (now / 90) % 10, ROWS - 1, CRGB(220, 200, 90));
  if (person > 0 && person < 1) setPixel(3 + person * 9, ROWS - 2, CRGB(255, 200, 150));
  drawTime(clock, digits, 0, palette.digit, palette);
}

// ---- 21: Haunted house --------------------------------------------------------------------------------------------

void haunted(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const float dt = frameDt(now);
  const int minutes = clock.hour * 60 + clock.minute;
  const int fromMidnight = min(minutes, 1440 - minutes);
  const uint8_t ghosts = 1 + (720 - fromMidnight) / 180;
  for (uint8_t i = 0; i < ghosts; i++) {
    parts[i].x += parts[i].vx * dt;
    if (parts[i].x < 0) parts[i].x += COLS;
    if (parts[i].x >= COLS) parts[i].x -= COLS;
    setPixel(parts[i].x, 2 + scale8(sin8(now / 9 + i * 57), 5), CRGB(50, 90, 70));
  }
  for (uint8_t x = 5; x <= 10; x++) {
    for (uint8_t y = 6; y < ROWS; y++) setPixel(x, y, CRGB(20, 8, 34));
    setPixel(x, 5, x > 6 && x < 9 ? CRGB(20, 8, 34) : CRGB::Black);
  }
  setPixel(7, 6, random8(5) ? CRGB(240, 140, 20) : CRGB(40, 20, 4));
  drawTime(clock, digits, 0, palette.digit, palette);
}

// ---- 23: Digit morph --------------------------------------------------------------------------------------------

constexpr uint16_t MORPH_MS = 900;

uint8_t digitPoints(uint8_t digit, uint8_t points[][2]) {
  uint8_t count = 0;
  for (uint8_t row = 0; row < DIGIT_HEIGHT; row++) {
    for (uint8_t col = 0; col < DIGIT_WIDTH; col++) {
      if (digitPixel(digit, col, row)) {
        points[count][0] = col;
        points[count][1] = row;
        count++;
      }
    }
  }
  return count;
}

void morph(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  if (digitsChanged(digits)) {
    timer[0] = now;
    for (uint8_t i = 0; i < 4; i++) state[i] = previousDigits[i];
    state[4] = 1;
  }
  drawColon(clock.millisecond, palette, 2, 4);
  const float t = state[4] ? constrain(float(now - timer[0]) / MORPH_MS, 0.0f, 1.0f) : 1.0f;
  if (t >= 1.0f) state[4] = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if (t >= 1.0f || state[i] == digits[i]) {
      drawDigit(digits[i], DIGIT_COLUMNS[i], 1, palette.digit);
      continue;
    }
    uint8_t oldPoints[15][2], newPoints[15][2];
    const uint8_t oldCount = digitPoints(state[i], oldPoints);
    const uint8_t newCount = digitPoints(digits[i], newPoints);
    const uint8_t count = max(oldCount, newCount);
    const float eased = t * t * (3.0f - 2.0f * t);
    for (uint8_t p = 0; p < count; p++) {
      const uint8_t *from = oldPoints[p * oldCount / count];
      const uint8_t *to = newPoints[p * newCount / count];
      setPixel(roundf(DIGIT_COLUMNS[i] + from[0] + (to[0] - from[0]) * eased), roundf(1 + from[1] + (to[1] - from[1]) * eased), palette.digit);
    }
  }
}

// ---- 24: Pendulum ------------------------------------------------------------------------------------------------

void pendulum(const ClockTime &clock, const uint8_t digits[4], const Palette &palette) {
  const float seconds = clock.second + clock.millisecond / 1000.0f;
  const float swing = sinf(seconds * PI);
  const float bobX = 7.5f + swing * 6.5f;
  drawLine(7.5f, 5.0f, bobX, 6.0f, CRGB(90, 60, 22));
  setPixel(roundf(bobX), ROWS - 1, CRGB(235, 180, 60));
  drawTime(clock, digits, 0, palette.digit, palette);
}

// ---- 27: Weather window -------------------------------------------------------------------------------------------

void weatherParticles(uint32_t now, uint8_t count, float speed, CRGB color, float drift) {
  const float dt = frameDt(now);
  for (uint8_t i = 0; i < count; i++) {
    parts[i].y += speed * dt;
    parts[i].x += drift * dt;
    if (parts[i].x >= COLS) parts[i].x -= COLS;
    if (parts[i].y >= ROWS) {
      parts[i].y = -float(random8(8));
      parts[i].x = random8(COLS);
    }
    if (parts[i].y >= 0) setPixel(parts[i].x, parts[i].y, color);
  }
}

void drawClouds(uint32_t now, CRGB color) {
  for (uint8_t x = 0; x < COLS; x++) {
    for (uint8_t y = 0; y < 3; y++) {
      if (inoise8(x * 40 + now / 12, y * 70) > 130 + y * 14) setPixel(x, y, color);
    }
  }
}

void weather(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  const int code = liveWeatherCode();
  if (code >= 95) {
    drawClouds(now, CRGB(20, 20, 40));
    weatherParticles(now, 10, 13, CRGB(30, 70, 170), 0);
    if ((now / 90) % 37 == 0) {
      for (uint16_t i = 0; i < CELLS; i++) setPixel(i % COLS, i / COLS, CRGB(70, 70, 110));
    }
  } else if (code >= 71 && code <= 77) {
    weatherParticles(now, 12, 2.6f, CRGB(50, 70, 130), 0.4f);
  } else if (code >= 51) {
    drawClouds(now, CRGB(24, 26, 42));
    weatherParticles(now, 10, 11, CRGB(30, 70, 170), 0);
  } else if (code >= 45) {
    for (uint8_t y = 0; y < ROWS; y++) {
      for (uint8_t x = 0; x < COLS; x++) {
        const uint8_t fog = inoise8(x * 26 + now / 20, y * 40);
        setPixel(x, y, CRGB(fog / 8, fog / 8, fog / 7));
      }
    }
  } else if (code >= 1) {
    drawClouds(now, CRGB(44, 48, 62));
  } else if (code == 0) {
    const uint8_t ray = (now / 300) % 2;
    setPixel(COLS - 2, 0, CRGB(235, 170, 30));
    setPixel(COLS - 3 - ray, 1 + ray, CRGB(120, 80, 10));
  }
  drawTime(clock, digits, 1, palette.digit, palette);
}

// ---- 28: Real sun and moon -----------------------------------------------------------------------------------------

constexpr uint8_t SKY_ROWS = 3;
constexpr float SYNODIC_MONTH_DAYS = 29.53059f;
constexpr uint32_t KNOWN_NEW_MOON = 947182440;  // 2000-01-06 18:14 UTC.

void sunTimes(const ClockTime &clock, int &sunrise, int &sunset) {
  sunrise = 360;
  sunset = 1080;
  if (!liveHasLocation()) return;
  const float declination = radians(23.44f) * sinf(TWO_PI * (284 + clock.yearDay) / 365.0f);
  const float latitude = radians(liveLatitude());
  const float cosHour = constrain(-tanf(latitude) * tanf(declination), -1.0f, 1.0f);
  const float halfDayMinutes = degrees(acosf(cosHour)) * 4.0f;
  const float solarNoon = 720.0f - liveLongitude() * 4.0f + clock.utcOffsetMinutes;
  sunrise = solarNoon - halfDayMinutes;
  sunset = solarNoon + halfDayMinutes;
}

void realSunMoon(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  int sunrise, sunset;
  sunTimes(clock, sunrise, sunset);
  const int minutes = clock.hour * 60 + clock.minute;
  const bool day = minutes >= sunrise && minutes < sunset;
  float progress;
  if (day) {
    progress = float(minutes - sunrise) / max(1, sunset - sunrise);
  } else {
    const int nightLength = max(1, 1440 - (sunset - sunrise));
    progress = float((minutes - sunset + 1440) % 1440) / nightLength;
  }
  const int x = roundf(progress * (COLS - 1));
  const int y = roundf((SKY_ROWS - 1) * (1.0f - sinf(progress * PI)));
  if (day) {
    const CRGB sky(0, 10, 34);
    for (uint8_t row = 0; row < SKY_ROWS; row++) {
      for (uint8_t col = 0; col < COLS; col++) setPixel(col, row, sky);
    }
    setPixel(x, y, CRGB(255, 196, 70));
    setPixel(x - 1, y, CRGB(120, 76, 16));
    setPixel(x + 1, y, CRGB(120, 76, 16));
  } else {
    const float age = fmodf(float(clock.unixTime - KNOWN_NEW_MOON) / 86400.0f, SYNODIC_MONTH_DAYS) / SYNODIC_MONTH_DAYS;
    const float lit = 0.5f - 0.5f * cosf(age * TWO_PI);
    setPixel(x, y, CRGB(uint8_t(30 + lit * 200), uint8_t(32 + lit * 205), uint8_t(40 + lit * 210)));
    const int side = age < 0.5f ? 1 : -1;
    setPixel(x + side, y, CRGB(uint8_t(lit * 60), uint8_t(lit * 62), uint8_t(lit * 70)));
  }
  drawTime(clock, digits, SKY_ROWS, palette.digit, palette);
}

// ---- 29: Tide line ---------------------------------------------------------------------------------------------------

void tide(const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  float level = 0.3f;
  bool rising = false;
  if (liveHasTide()) {
    const float here = liveTideLevel(clock.hour);
    const float next = liveTideLevel(clock.hour + 1);
    level = here + (next - here) * clock.minute / 60.0f;
    rising = next > here;
  }
  const float height = 0.5f + level * 2.5f;
  for (uint8_t x = 0; x < COLS; x++) {
    const float wave = (sin8(x * 28 + now / 8) / 255.0f - 0.5f) * 0.6f;
    const int top = ROWS - max(1, int(roundf(height + wave)));
    for (uint8_t y = top; y < ROWS; y++) setPixel(x, y, y == top ? CRGB(20, 90, 200) : CRGB(0, 30, 110));
  }
  if (liveHasTide()) setPixel(COLS - 1, rising ? 5 : 7, CRGB(235, 235, 235));
  drawTime(clock, digits, 0, palette.digit, palette);
}

}  // namespace

void complexFaceReset(uint8_t index, const ClockTime &clock, uint32_t now) {
  memset(grid, 0, sizeof(grid));
  memset(grid2, 0, sizeof(grid2));
  memset(waveNow, 0, sizeof(waveNow));
  memset(wavePrev, 0, sizeof(wavePrev));
  memset(state, 0, sizeof(state));
  memset(value, 0, sizeof(value));
  for (uint32_t &t : timer) t = now;
  lastFrame = now;
  previousDigits[0] = clock.hour / 10;
  previousDigits[1] = clock.hour % 10;
  previousDigits[2] = clock.minute / 10;
  previousDigits[3] = clock.minute % 10;
  for (Particle &p : parts) {
    p = {float(random8(COLS)), -float(random8(ROWS)), (random8(2) ? 1.0f : -1.0f) * (1.0f + random8(10) / 10.0f), 1.5f + random8(20) / 10.0f, 0, 0};
  }
  switch (index) {
    case 0:
      antsReset();
      break;
    case 1:
      preySeed();
      break;
    case 3:
      for (uint16_t i = 0; i < CELLS; i++) grid[i] = random8(4) == 0;
      break;
    case 4:
      birdsReset();
      break;
    case 5:
      for (uint16_t i = 0; i < CELLS; i++) grid[i] = random8(5) != 0;
      break;
    case 7:
      state[1] = clock.hour;
      break;
    case 8:
      value[0] = 8;
      value[1] = 6;
      value[2] = 7;
      value[3] = 5;
      break;
    case 9:
      state[0] = 8;
      state[1] = 6;
      state[2] = 1;
      state[3] = -1;
      break;
    case 10:
      parts[0].x = COLS;
      parts[1].x = COLS + 9;
      break;
    case 11:
      state[3] = -1;
      state[4] = 2;
      state[5] = 1;
      break;
    case 13:
      wormReset();
      break;
    case 19:
      value[0] = 2;
      value[1] = 1;
      break;
    default:
      break;
  }
}

void complexFaceDraw(uint8_t index, const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette) {
  switch (index) {
    case 0: ants(clock, digits, now, palette); break;
    case 1: prey(clock, digits, now, palette); break;
    case 2: fallingSand(clock, digits, now, palette); break;
    case 3: life(clock, digits, now, palette); break;
    case 4: birds(clock, digits, now, palette); break;
    case 5: forestFire(clock, digits, now, palette); break;
    case 6: ripple(clock, digits, now, palette); break;
    case 7: crystal(clock, digits, now, palette); break;
    case 8: paddleDuel(clock, digits, now, palette); break;
    case 9: brickSmasher(clock, digits, now, palette); break;
    case 10: runner(clock, digits, now, palette); break;
    case 11: defender(clock, digits, now, palette); break;
    case 12: maze(clock, digits, now, palette); break;
    case 13: worm(clock, digits, now, palette); break;
    case 14: blockBuilder(clock, digits, now, palette); break;
    case 15: landscape(clock, digits, now, palette); break;
    case 16: train(clock, digits, now, palette); break;
    case 17: rocket(clock, digits, now, palette); break;
    case 18: seasons(clock, digits, now, palette); break;
    case 19: pixelPet(clock, digits, now, palette); break;
    case 20: littleTown(clock, digits, now, palette); break;
    case 21: haunted(clock, digits, now, palette); break;
    // 22, 25 and 26 were removed, and faceAvailable() in faces.cpp keeps them from being chosen.
    case 23: morph(clock, digits, now, palette); break;
    case 24: pendulum(clock, digits, palette); break;
    case 27: weather(clock, digits, now, palette); break;
    case 28: realSunMoon(clock, digits, now, palette); break;
    default: tide(clock, digits, now, palette); break;
  }
  memcpy(previousDigits, digits, 4);
}
