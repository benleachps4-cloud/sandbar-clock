#pragma once

#include <stdint.h>

// ---- Wi-Fi and time ---------------------------------------------------------

// Your Wi-Fi network and timezone are set from the clock's setup page and saved in flash.
// The clock opens this network when it has no Wi-Fi to join.
constexpr const char *SETUP_NETWORK_NAME = "Sandbar-Clock-Setup";
constexpr const char *SETUP_NETWORK_PASSWORD = "sandbar1234";  // At least 8 characters.

// Once connected, the setup page is at http://sandbar-clock.local
constexpr const char *DEVICE_HOSTNAME = "sandbar-clock";

// Used until a timezone is chosen on the setup page.
constexpr const char *DEFAULT_TIMEZONE = "UTC0";

// ---- LEDs -------------------------------------------------------------------

// LED_PIN is set for each board in platformio.ini.
constexpr uint8_t LED_DATA_PIN = LED_PIN;
constexpr uint32_t MAX_MILLIAMPS = 1500;

// The web page's slider starts here, then remembers your setting.
constexpr uint8_t DEFAULT_BRIGHTNESS_PERCENT = 16;
constexpr uint8_t MIN_BRIGHTNESS_PERCENT = 5;

// Brightness out of 255. Together these set how much the display dims at night.
constexpr uint8_t DAY_BRIGHTNESS = 40;
constexpr uint8_t NIGHT_BRIGHTNESS = 16;

// Hours at which the sand colour changes. Night uses NIGHT_BRIGHTNESS.
constexpr uint8_t DAY_START_HOUR = 7;
constexpr uint8_t DUSK_START_HOUR = 17;
constexpr uint8_t NIGHT_START_HOUR = 22;

// ---- Panel wiring -----------------------------------------------------------
// These are the starting layout only. Panel layout on the clock's page changes it, and the saved choice wins.

constexpr bool LEFT_PANEL_FIRST = true;  // The left panel is the one fed by the ESP32.
constexpr bool PANEL_SERPENTINE = false;  // Every other line of LEDs runs backwards.

struct PanelWiring {
  bool flipX;
  bool flipY;
  bool columns;  // LED lines run top to bottom, not left to right.
};

// Index 0 is the left panel and index 1 is the right panel, which is mounted a quarter-turn from the left one.
constexpr PanelWiring PANELS[2] = {
    {false, false, false},
    {false, true, true},
};
