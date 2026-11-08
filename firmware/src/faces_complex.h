#pragma once

#include "display.h"

constexpr uint8_t COMPLEX_FACE_COUNT = 30;

void complexFaceReset(uint8_t index, const ClockTime &clock, uint32_t now);
void complexFaceDraw(uint8_t index, const ClockTime &clock, const uint8_t digits[4], uint32_t now, const Palette &palette);
