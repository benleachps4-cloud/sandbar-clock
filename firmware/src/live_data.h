#pragma once

#include <stdint.h>

// Fetches weather and tide data over plain HTTP from Open-Meteo, in the background.
void liveDataBegin(bool hasLocation, float latitude, float longitude);
void liveDataSetLocation(float latitude, float longitude);
void liveDataLoop(uint32_t now, bool connected);

bool liveHasLocation();
float liveLatitude();
float liveLongitude();

// -1 until the first successful fetch.
int liveWeatherCode();
bool liveHasTide();
float liveTideLevel(int hour);  // 0 is the day's lowest tide and 1 the highest.
