#include <Arduino.h>
#include <sys/time.h>
#include <time.h>

#include "config.h"
#include "display.h"
#include "faces.h"
#include "live_data.h"
#include "wifi_setup.h"

namespace {

constexpr uint32_t FRAME_MS = 16;

uint32_t lastFrame = 0;
bool timeLogged = false;

bool readClock(ClockTime &clock) {
  timeval now;
  tm local;
  gettimeofday(&now, nullptr);
  localtime_r(&now.tv_sec, &local);
  if (local.tm_year < 2024 - 1900) return false;
  tm utc;
  gmtime_r(&now.tv_sec, &utc);
  int offset = (local.tm_hour - utc.tm_hour) * 60 + local.tm_min - utc.tm_min;
  if (local.tm_yday != utc.tm_yday) offset += (local.tm_year > utc.tm_year || local.tm_yday > utc.tm_yday) ? 1440 : -1440;
  clock = {local.tm_hour, local.tm_min, local.tm_sec, uint16_t(now.tv_usec / 1000), local.tm_mon, local.tm_yday, offset, uint32_t(now.tv_sec)};
  return true;
}

// Shown until the first time sync: four dashes and a dot scanning along the bottom.
// The dot is blue in setup mode, amber while joining Wi-Fi and green once connected.
void renderWaiting(uint32_t now) {
  displayClear(DAY_BRIGHTNESS);
  for (uint8_t left : DIGIT_COLUMNS) {
    for (uint8_t col = 0; col < DIGIT_WIDTH; col++) setPixel(left + col, 2, GOLD.digit);
  }
  CRGB dot = GOLD.band[0];
  if (wifi_setup::inSetupMode()) dot = CRGB::Blue;
  else if (wifi_setup::isConnected()) dot = CRGB::Green;
  setPixel((now / 120) % COLS, ROWS - 1, dot);
  displayShow();
}

// Shown while choosing the panel layout. Neither letter has any symmetry, so each of a panel's
// eight possible orientations looks different, and only the right one reads correctly.
void renderPanelTest() {
  static constexpr uint8_t LETTER_L[ROWS] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x7E};
  static constexpr uint8_t LETTER_R[ROWS] = {0x7C, 0x66, 0x66, 0x7C, 0x78, 0x6C, 0x66, 0x66};
  displayClear(DAY_BRIGHTNESS);
  for (uint8_t y = 0; y < ROWS; y++) {
    for (uint8_t x = 0; x < 8; x++) {
      if (LETTER_L[y] & (0x80 >> x)) setPixel(x, y, GOLD.band[0]);
      if (LETTER_R[y] & (0x80 >> x)) setPixel(8 + x, y, CRGB(0, 120, 255));
    }
  }
  displayShow();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  displayBegin();
  random16_add_entropy(esp_random());
  facesBegin();
  wifi_setup::begin();
}

void loop() {
  wifi_setup::loop();

  const uint32_t now = millis();
  liveDataLoop(now, wifi_setup::isConnected());
  if (now - lastFrame < FRAME_MS) return;
  lastFrame = now;

  if (wifi_setup::panelTestActive()) {
    renderPanelTest();
    return;
  }

  ClockTime clock;
  if (!readClock(clock)) {
    renderWaiting(now);
    return;
  }
  if (!timeLogged) {
    timeLogged = true;
    Serial.printf("Time synced: %02d:%02d:%02d\n", clock.hour, clock.minute, clock.second);
  }
  facesUpdate(wifi_setup::selectedFace(), wifi_setup::brightnessPercent(), clock, now);
}
