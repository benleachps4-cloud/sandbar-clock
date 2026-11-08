#include "live_data.h"

#include <Arduino.h>
#include <WiFi.h>

namespace {

constexpr uint32_t FETCH_INTERVAL_MS = 30UL * 60UL * 1000UL;
constexpr uint32_t RETRY_INTERVAL_MS = 2UL * 60UL * 1000UL;
constexpr uint32_t RESPONSE_TIMEOUT_MS = 8000;
constexpr uint8_t TIDE_HOURS = 24;

bool locationSet = false;
float lat = 0;
float lon = 0;
volatile int weatherCode = -1;
volatile bool tideValid = false;
float tideLevels[TIDE_HOURS];
volatile bool fetching = false;
volatile bool lastFetchOk = false;
volatile bool fetchRequested = false;
uint32_t lastFetch = 0;
bool fetchedOnce = false;

String httpGet(const char *host, const String &path) {
  WiFiClient client;
  String body;
  if (!client.connect(host, 80, 5000)) return body;
  client.print("GET " + path + " HTTP/1.0\r\nHost: " + host + "\r\nUser-Agent: sandbar-clock\r\nConnection: close\r\n\r\n");
  const uint32_t started = millis();
  while ((client.connected() || client.available()) && millis() - started < RESPONSE_TIMEOUT_MS) {
    if (client.available()) {
      body += char(client.read());
    } else {
      delay(5);
    }
  }
  client.stop();
  const int split = body.indexOf("\r\n\r\n");
  return split < 0 ? String() : body.substring(split + 4);
}

bool fetchWeather() {
  const String path = "/v1/forecast?latitude=" + String(lat, 3) + "&longitude=" + String(lon, 3) + "&current=weather_code&timezone=auto";
  const String body = httpGet("api.open-meteo.com", path);
  const int current = body.indexOf("\"current\":{");
  if (current < 0) return false;
  const char *key = "\"weather_code\":";
  const int found = body.indexOf(key, current);
  if (found < 0) return false;
  weatherCode = body.substring(found + strlen(key)).toInt();
  return true;
}

bool fetchTide() {
  const String path = "/v1/marine?latitude=" + String(lat, 3) + "&longitude=" + String(lon, 3) + "&hourly=sea_level_height_msl&forecast_days=1&timezone=auto";
  const String body = httpGet("marine-api.open-meteo.com", path);
  const int hourly = body.indexOf("\"hourly\":{");
  if (hourly < 0) return false;
  const char *key = "\"sea_level_height_msl\":[";
  int position = body.indexOf(key, hourly);
  if (position < 0) return false;
  position += strlen(key);

  float values[TIDE_HOURS];
  float lowest = 1000;
  float highest = -1000;
  for (uint8_t i = 0; i < TIDE_HOURS; i++) {
    if (body.startsWith("null", position)) return false;  // Inland locations have no tide data.
    values[i] = body.substring(position, position + 12).toFloat();
    lowest = min(lowest, values[i]);
    highest = max(highest, values[i]);
    position = body.indexOf(',', position);
    if (position < 0 && i < TIDE_HOURS - 1) return false;
    position++;
  }
  if (highest - lowest < 0.01f) return false;
  for (uint8_t i = 0; i < TIDE_HOURS; i++) tideLevels[i] = (values[i] - lowest) / (highest - lowest);
  return true;
}

void fetchTask(void *) {
  const bool weatherOk = fetchWeather();
  tideValid = fetchTide();
  lastFetchOk = weatherOk;
  fetching = false;
  vTaskDelete(nullptr);
}

}  // namespace

void liveDataBegin(bool hasLocation, float latitude, float longitude) {
  locationSet = hasLocation;
  lat = latitude;
  lon = longitude;
}

void liveDataSetLocation(float latitude, float longitude) {
  locationSet = true;
  lat = latitude;
  lon = longitude;
  fetchRequested = true;
}

void liveDataLoop(uint32_t now, bool connected) {
  if (!locationSet || !connected || fetching) return;
  const uint32_t interval = lastFetchOk ? FETCH_INTERVAL_MS : RETRY_INTERVAL_MS;
  if (!fetchRequested && fetchedOnce && now - lastFetch < interval) return;
  fetchRequested = false;
  fetchedOnce = true;
  lastFetch = now;
  fetching = true;
  if (xTaskCreate(fetchTask, "live-data", 7168, nullptr, 1, nullptr) != pdPASS) fetching = false;
}

bool liveHasLocation() { return locationSet; }
float liveLatitude() { return lat; }
float liveLongitude() { return lon; }
int liveWeatherCode() { return weatherCode; }
bool liveHasTide() { return tideValid; }
float liveTideLevel(int hour) { return tideLevels[hour % TIDE_HOURS]; }
