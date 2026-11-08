#pragma once

#include "display.h"

constexpr uint8_t RETRO_FACE_COUNT = 12;

void retroFaceDraw(uint8_t index, const ClockTime &clock, const uint8_t digits[4], uint32_t now);
