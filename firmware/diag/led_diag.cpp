// Standalone LED test: drives the panels through the ESP32 RMT peripheral, without FastLED or Wi-Fi.
// If this lights the panels and the clock firmware doesn't, the fault is in the firmware.
// If the panels stay dark here, look at the power and data wiring.

#include <Arduino.h>

#include "../src/config.h"

namespace {

constexpr uint16_t LED_COUNT = 128;
constexpr uint8_t BITS_PER_LED = 24;
constexpr uint8_t LEVEL = 30;
constexpr uint32_t COLOUR_MS = 1000;

// Dev boards usually have one WS2812 on one of these pins, which proves the signal timing is valid.
constexpr uint8_t ONBOARD_RGB_PINS[2] = {48, 38};

// At 10 MHz each RMT tick is 100 ns.
constexpr uint32_t RMT_FREQUENCY_HZ = 10000000;
constexpr uint8_t T0H = 4;
constexpr uint8_t T0L = 8;
constexpr uint8_t T1H = 8;
constexpr uint8_t T1L = 4;

rmt_data_t symbols[LED_COUNT * BITS_PER_LED];
bool rmtReady = false;
uint32_t frameCount = 0;

void encode(uint8_t red, uint8_t green, uint8_t blue) {
  const uint32_t grb = (uint32_t(green) << 16) | (uint32_t(red) << 8) | blue;
  size_t index = 0;
  for (uint16_t led = 0; led < LED_COUNT; led++) {
    for (int bit = BITS_PER_LED - 1; bit >= 0; bit--) {
      const bool one = grb & (1UL << bit);
      symbols[index].level0 = 1;
      symbols[index].duration0 = one ? T1H : T0H;
      symbols[index].level1 = 0;
      symbols[index].duration1 = one ? T1L : T0L;
      index++;
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  // The S3 has four TX memory blocks to share between the three pins.
  rmtReady = rmtInit(LED_DATA_PIN, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_2, RMT_FREQUENCY_HZ);
  for (uint8_t pin : ONBOARD_RGB_PINS) rmtInit(pin, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, RMT_FREQUENCY_HZ);
}

void loop() {
  static const char *const NAMES[3] = {"red", "green", "blue"};
  const uint8_t colour = frameCount % 3;
  encode(colour == 0 ? LEVEL : 0, colour == 1 ? LEVEL : 0, colour == 2 ? LEVEL : 0);
  const bool written = rmtReady && rmtWrite(LED_DATA_PIN, symbols, LED_COUNT * BITS_PER_LED, RMT_WAIT_FOR_EVER);
  for (uint8_t pin : ONBOARD_RGB_PINS) rmtWrite(pin, symbols, BITS_PER_LED, RMT_WAIT_FOR_EVER);
  Serial.printf("LED test: pin %u, rmtInit %s, write %s, colour %s, frame %lu\n", LED_DATA_PIN, rmtReady ? "ok" : "FAILED",
                written ? "ok" : "FAILED", NAMES[colour], (unsigned long)frameCount);
  frameCount++;
  delay(COLOUR_MS);
}
