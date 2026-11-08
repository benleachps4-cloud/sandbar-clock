#pragma once

#include "display.h"
#include "faces_complex.h"
#include "faces_retro.h"

// These numbers are saved on the clock and listed in setup_page.h, so add new faces at the end.
enum Face : uint8_t {
  FACE_SANDBAR,
  FACE_DIGIT_SAND,
  FACE_BIG_DIGITS,
  FACE_SPLIT_FLAP,
  FACE_SUN_MOON,
  FACE_ROLLING,
  FACE_RAINDROPS,
  FACE_LAVA,
  FACE_SNOW,
  FACE_CITY,
  FACE_CHASE,
  FACE_SECONDS_BAR,
  FACE_BINARY,
  FACE_RETIRED_TALLY,  // Removed. Its number stays reserved so the faces after it keep theirs.
  FACE_WORDS,
  FACE_GARDENER,
  FACE_CODE_RAIN,
  FACE_CAMPFIRE,
  FACE_AURORA,
  FACE_STARFIELD,
  FACE_RAINBOW,
  FACE_HEARTBEAT,
  FACE_FIREWORKS,
  FACE_OCEAN,
  FACE_AQUARIUM,
  FACE_MARQUEE,
  FACE_COMPLEX_FIRST,
  FACE_RETRO_FIRST = FACE_COMPLEX_FIRST + COMPLEX_FACE_COUNT,
  FACE_COUNT = FACE_RETRO_FIRST + RETRO_FACE_COUNT
};

constexpr uint8_t FACE_SHUFFLE = 255;

// False for numbers beyond the end of the list and for faces that have been removed.
bool faceAvailable(uint8_t face);

void facesBegin();
void facesUpdate(uint8_t selection, uint8_t brightnessPercent, const ClockTime &clock, uint32_t now);
