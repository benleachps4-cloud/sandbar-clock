#pragma once

#include <stdint.h>

namespace wifi_setup {

// Loads the saved network and either joins it or opens the setup network.
void begin();

// Call on every pass through the main loop.
void loop();

bool inSetupMode();
bool isConnected();
uint8_t selectedFace();
uint8_t brightnessPercent();
bool panelTestActive();

}  // namespace wifi_setup
