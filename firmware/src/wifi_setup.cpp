#include "wifi_setup.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"
#include "display.h"
#include "faces.h"
#include "live_data.h"
#include "setup_page.h"

namespace {

constexpr uint32_t CONNECT_TIMEOUT_MS = 20000;
constexpr uint32_t SETUP_RETRY_MS = 180000;  // Retry the saved network after this long with nobody on setup.
constexpr uint32_t RESTART_DELAY_MS = 1500;
constexpr uint32_t PANEL_TEST_TIMEOUT_MS = 300000;  // Back to the clock if the page is left open or closed.
constexpr uint16_t DNS_PORT = 53;
constexpr size_t MAX_SSID_LENGTH = 32;
constexpr size_t MIN_PASSWORD_LENGTH = 8;
constexpr size_t MAX_PASSWORD_LENGTH = 63;
constexpr size_t MAX_TIMEZONE_LENGTH = 63;
constexpr const char *PREFS_NAMESPACE = "sandbar";

enum class Mode { Connecting, Connected, Setup };

WebServer server(80);
DNSServer dns;

String savedSsid;
String savedPassword;
String savedTimezone;
uint8_t savedFace = FACE_SANDBAR;
uint8_t brightness = DEFAULT_BRIGHTNESS_PERCENT;
uint8_t savedLayout = 0;
bool panelTest = false;
uint32_t panelTestSeen = 0;
Mode mode = Mode::Connecting;
uint32_t modeStart = 0;
uint32_t lastStationSeen = 0;
uint32_t restartRequestedAt = 0;
bool restartRequested = false;
bool mdnsStarted = false;

void loadSettings() {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  savedSsid = prefs.getString("ssid", "");
  savedPassword = prefs.getString("password", "");
  savedTimezone = prefs.getString("timezone", DEFAULT_TIMEZONE);
  savedFace = prefs.getUChar("face", FACE_SANDBAR);
  brightness = prefs.getUChar("brightness", DEFAULT_BRIGHTNESS_PERCENT);
  savedLayout = prefs.getUChar("layout", displayDefaultLayout());
  displaySetLayout(savedLayout);
  const bool hasLocation = prefs.getBool("hasLocation", false);
  const float latitude = prefs.getFloat("latitude", 0);
  const float longitude = prefs.getFloat("longitude", 0);
  prefs.end();
  liveDataBegin(hasLocation, latitude, longitude);
}

void saveLocation(float latitude, float longitude) {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putBool("hasLocation", true);
  prefs.putFloat("latitude", latitude);
  prefs.putFloat("longitude", longitude);
  prefs.end();
}

void saveBrightness(uint8_t percent) {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putUChar("brightness", percent);
  prefs.end();
}

void saveLayout(uint8_t layout) {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putUChar("layout", layout);
  prefs.end();
}

void saveFace(uint8_t face) {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putUChar("face", face);
  prefs.end();
}

void saveSettings(const String &ssid, const String &password, const String &timezone) {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putString("ssid", ssid);
  prefs.putString("password", password);
  prefs.putString("timezone", timezone);
  prefs.end();
}

void forgetNetwork() {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.remove("ssid");
  prefs.remove("password");
  prefs.end();
}

void requestRestart() {
  restartRequested = true;
  restartRequestedAt = millis();
}

String jsonString(const String &value) {
  String out = "\"";
  for (size_t i = 0; i < value.length(); i++) {
    const char c = value[i];
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (uint8_t(c) < 0x20) {
      char escaped[7];
      snprintf(escaped, sizeof(escaped), "\\u%04x", c);
      out += escaped;
    } else {
      out += c;
    }
  }
  return out + "\"";
}

// Many C3 SuperMini boards can't hold a Wi-Fi connection at full transmit power.
void limitTxPower() {
#if CONFIG_IDF_TARGET_ESP32C3
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
#endif
}

void startConnecting() {
  Serial.printf("Joining %s\n", savedSsid.c_str());
  WiFi.mode(WIFI_STA);
  limitTxPower();
  WiFi.setAutoReconnect(true);
  WiFi.begin(savedSsid.c_str(), savedPassword.c_str());
  mode = Mode::Connecting;
  modeStart = millis();
}

void startSetupMode() {
  Serial.printf("Opening setup network %s\n", SETUP_NETWORK_NAME);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(SETUP_NETWORK_NAME, SETUP_NETWORK_PASSWORD);
  limitTxPower();
  dns.start(DNS_PORT, "*", WiFi.softAPIP());
  mode = Mode::Setup;
  modeStart = millis();
  lastStationSeen = modeStart;
}

void stopSetupMode() {
  dns.stop();
  WiFi.softAPdisconnect(true);
}

void onConnected() {
  mode = Mode::Connected;
  Serial.printf("Wi-Fi connected: %s\n", WiFi.localIP().toString().c_str());
  if (!mdnsStarted && MDNS.begin(DEVICE_HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    mdnsStarted = true;
  }
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", SETUP_PAGE);
}

// The saved password is never sent back to the page.
void handleStatus() {
  const bool connected = wifi_setup::isConnected();
  const IPAddress ip = connected ? WiFi.localIP() : WiFi.softAPIP();
  String json = "{\"setup\":";
  json += mode == Mode::Setup ? "true" : "false";
  json += ",\"connected\":";
  json += connected ? "true" : "false";
  json += ",\"ssid\":" + jsonString(savedSsid);
  json += ",\"ip\":" + jsonString(ip.toString());
  json += ",\"timezone\":" + jsonString(savedTimezone);
  json += ",\"setupName\":" + jsonString(SETUP_NETWORK_NAME);
  json += ",\"host\":" + jsonString(DEVICE_HOSTNAME);
  json += ",\"face\":" + String(savedFace);
  json += ",\"brightness\":" + String(brightness);
  json += ",\"minBrightness\":" + String(MIN_BRIGHTNESS_PERCENT);
  json += ",\"layout\":" + String(displayLayout());
  json += ",\"panelTest\":";
  json += panelTest ? "true" : "false";
  json += ",\"hasLocation\":";
  json += liveHasLocation() ? "true" : "false";
  json += ",\"latitude\":" + String(liveLatitude(), 3);
  json += ",\"longitude\":" + String(liveLongitude(), 3);
  json += ",\"weatherCode\":" + String(liveWeatherCode());
  json += ",\"hasTide\":";
  json += liveHasTide() ? "true" : "false";
  json += "}";
  server.send(200, "application/json", json);
}

void handleScan() {
  const int16_t count = WiFi.scanComplete();
  if (count == WIFI_SCAN_FAILED) WiFi.scanNetworks(true);
  if (count < 0) {
    server.send(200, "application/json", "{\"scanning\":true}");
    return;
  }
  String json = "{\"networks\":[";
  for (int16_t i = 0; i < count; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":" + jsonString(WiFi.SSID(i));
    json += ",\"rssi\":" + String(WiFi.RSSI(i));
    json += ",\"open\":";
    json += WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "true" : "false";
    json += "}";
  }
  json += "]}";
  WiFi.scanDelete();
  server.send(200, "application/json", json);
}

void handleSave() {
  const String ssid = server.arg("ssid");
  String password = server.arg("password");
  String timezone = server.arg("timezone");

  if (ssid.isEmpty() || ssid.length() > MAX_SSID_LENGTH) {
    server.send(400, "text/plain", "Enter a network name of up to 32 characters.");
    return;
  }
  if (password.isEmpty() && ssid == savedSsid) password = savedPassword;
  if (!password.isEmpty() && (password.length() < MIN_PASSWORD_LENGTH || password.length() > MAX_PASSWORD_LENGTH)) {
    server.send(400, "text/plain", "Wi-Fi passwords must be 8 to 63 characters long.");
    return;
  }
  if (timezone.isEmpty()) timezone = savedTimezone;
  if (timezone.length() > MAX_TIMEZONE_LENGTH) {
    server.send(400, "text/plain", "That timezone is too long.");
    return;
  }

  saveSettings(ssid, password, timezone);
  server.send(200, "text/plain", "Saved");
  requestRestart();
}

void handleFace() {
  const String value = server.arg("face");
  const long face = value.toInt();
  if (value.isEmpty() || !isDigit(value[0]) || face > 255 || (!faceAvailable(face) && face != FACE_SHUFFLE)) {
    server.send(400, "text/plain", "That clock face doesn't exist.");
    return;
  }
  savedFace = face;
  saveFace(savedFace);
  server.send(200, "text/plain", "Saved");
}

// The slider sends live values while dragging, and adds "save" when released to spare the flash.
void handleBrightness() {
  const String value = server.arg("value");
  const long percent = value.toInt();
  if (value.isEmpty() || !isDigit(value[0]) || percent < MIN_BRIGHTNESS_PERCENT || percent > 100) {
    server.send(400, "text/plain", "Brightness must be between 5 and 100.");
    return;
  }
  brightness = percent;
  if (server.hasArg("save")) saveBrightness(brightness);
  server.send(200, "text/plain", "Saved");
}

// Each change shows on the test pattern straight away. Nothing is kept until "save" arrives.
void handleLayout() {
  if (server.hasArg("cancel")) {
    panelTest = false;
    displaySetLayout(savedLayout);
    server.send(200, "text/plain", "Cancelled");
    return;
  }
  const String value = server.arg("value");
  const long layout = value.toInt();
  if (value.isEmpty() || !isDigit(value[0]) || layout > 255) {
    server.send(400, "text/plain", "That panel layout isn't valid.");
    return;
  }
  displaySetLayout(layout);
  if (server.hasArg("save")) {
    savedLayout = layout;
    saveLayout(savedLayout);
    panelTest = false;
  } else {
    panelTest = true;
    panelTestSeen = millis();
  }
  server.send(200, "text/plain", "Saved");
}

void handleLocation() {
  const float latitude = server.arg("latitude").toFloat();
  const float longitude = server.arg("longitude").toFloat();
  if (!server.hasArg("latitude") || !server.hasArg("longitude") || latitude < -90 || latitude > 90 || longitude < -180 || longitude > 180) {
    server.send(400, "text/plain", "Enter a latitude from -90 to 90 and a longitude from -180 to 180.");
    return;
  }
  saveLocation(latitude, longitude);
  liveDataSetLocation(latitude, longitude);
  server.send(200, "text/plain", "Saved");
}

void handleForget() {
  forgetNetwork();
  server.send(200, "text/plain", "Forgotten");
  requestRestart();
}

// Phones and laptops probe a known address to detect a captive portal. Redirecting those opens the page.
void handleNotFound() {
  if (mode != Mode::Setup) {
    server.send(404, "text/plain", "Not found");
    return;
  }
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/");
  server.send(302, "text/plain", "");
}

void startServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/face", HTTP_POST, handleFace);
  server.on("/brightness", HTTP_POST, handleBrightness);
  server.on("/layout", HTTP_POST, handleLayout);
  server.on("/location", HTTP_POST, handleLocation);
  server.on("/forget", HTTP_POST, handleForget);
  server.onNotFound(handleNotFound);
  server.begin();
}

}  // namespace

namespace wifi_setup {

bool inSetupMode() { return mode == Mode::Setup; }
bool isConnected() { return WiFi.status() == WL_CONNECTED; }
uint8_t selectedFace() { return savedFace; }
uint8_t brightnessPercent() { return brightness; }
bool panelTestActive() { return panelTest; }

void begin() {
  loadSettings();
  WiFi.persistent(false);  // Preferences already holds the network details.
  WiFi.setHostname(DEVICE_HOSTNAME);
  WiFi.mode(WIFI_STA);
  configTzTime(savedTimezone.c_str(), "pool.ntp.org", "time.google.com");
  startServer();
  if (savedSsid.isEmpty()) {
    startSetupMode();
  } else {
    startConnecting();
  }
}

void loop() {
  server.handleClient();
  const uint32_t now = millis();

  if (restartRequested && now - restartRequestedAt >= RESTART_DELAY_MS) ESP.restart();

  if (panelTest && now - panelTestSeen >= PANEL_TEST_TIMEOUT_MS) {
    panelTest = false;
    displaySetLayout(savedLayout);
  }

  switch (mode) {
    case Mode::Connecting:
      if (isConnected()) {
        onConnected();
      } else if (now - modeStart >= CONNECT_TIMEOUT_MS) {
        startSetupMode();
      }
      break;
    case Mode::Connected:
      break;  // The Wi-Fi stack reconnects by itself, and the clock keeps time meanwhile.
    case Mode::Setup:
      dns.processNextRequest();
      if (WiFi.softAPgetStationNum() > 0) lastStationSeen = now;
      if (!savedSsid.isEmpty() && now - lastStationSeen >= SETUP_RETRY_MS) {
        stopSetupMode();
        startConnecting();
      }
      break;
  }
}

}  // namespace wifi_setup
