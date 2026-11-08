#pragma once

#include <pgmspace.h>

// The page uses system fonts only, because a phone on the setup network has no internet.
const char SETUP_PAGE[] PROGMEM = R"html(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Sandbar Clock setup</title>
<style>
:root { color-scheme: dark; --wall: #0f2228; --panel: #16303a; --line: #2b4b57; --ink: #e9efe9; --muted: #9db4b6; --sand: #f2b65a; --on-sand: #1d1405; --bad: #ff9a8a; }
* { box-sizing: border-box; }
[hidden] { display: none !important; }
body { margin: 0; padding: 24px 16px 40px; background: var(--wall); color: var(--ink); font: 16px/1.5 system-ui, -apple-system, "Segoe UI", sans-serif; }
main { max-width: 440px; margin: 0 auto; display: grid; gap: 20px; }
h1 { margin: 0; font-size: 1.7rem; line-height: 1.15; color: var(--sand); }
#state { margin: 4px 0 0; color: var(--muted); }
form, #done { display: grid; gap: 16px; }
.field { display: grid; gap: 6px; }
label, h2 { margin: 0; font-size: 0.82rem; font-weight: 600; color: var(--muted); }
.head { display: flex; justify-content: space-between; align-items: center; gap: 12px; }
input, select { width: 100%; padding: 12px; font: inherit; color: var(--ink); background: var(--panel); border: 1px solid var(--line); border-radius: 4px; }
button { padding: 12px 14px; font: inherit; font-weight: 600; color: var(--ink); background: transparent; border: 1px solid var(--line); border-radius: 4px; cursor: pointer; }
button.small { padding: 5px 10px; font-size: 0.85rem; }
button.main { background: var(--sand); border-color: var(--sand); color: var(--on-sand); }
button:disabled { opacity: 0.6; }
:focus-visible { outline: 2px solid var(--sand); outline-offset: 2px; }
#networks { display: grid; gap: 6px; max-height: 260px; overflow-y: auto; }
.net { display: flex; justify-content: space-between; gap: 12px; width: 100%; text-align: left; font-weight: 400; background: var(--panel); border-color: transparent; }
.net[aria-pressed="true"] { border-color: var(--sand); }
.net span:first-child { min-width: 0; overflow-wrap: anywhere; }
.net span:last-child { flex: none; color: var(--muted); font-size: 0.85rem; }
.pass { display: grid; grid-template-columns: minmax(0, 1fr) auto; gap: 6px; }
.hint { margin: 0; font-size: 0.85rem; color: var(--muted); }
#error, #faceError, #panelError { margin: 0; color: var(--bad); }
#panelBox { display: grid; gap: 8px; margin-top: 16px; }
#faceList { display: grid; gap: 8px; }
.faceGrid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 6px; margin-top: 8px; }
.faceGrid .face:last-child:nth-child(odd) { grid-column: 1 / -1; }
.faceGroup { padding: 10px 12px; background: var(--panel); border-radius: 4px; }
.faceGroup .face { background: var(--wall); }
.face { font-weight: 400; background: var(--panel); border-color: transparent; }
input[type="range"] { padding: 0; accent-color: var(--sand); }
.face[aria-pressed="true"] { background: var(--sand); color: var(--on-sand); font-weight: 600; }
details { display: grid; gap: 16px; }
summary { cursor: pointer; font-weight: 600; color: var(--muted); }
details form { margin-top: 16px; }
#forget { color: var(--muted); }
code { color: var(--sand); }
</style>
</head>
<body>
<main>
  <header>
    <h1>Sandbar Clock</h1>
    <p id="state">Checking the clock&hellip;</p>
  </header>

  <section class="field" id="facesBox" hidden>
    <div class="head"><label for="brightness">Brightness</label><span class="hint" id="brightnessValue"></span></div>
    <input id="brightness" type="range" max="100">
    <h2>Clock face</h2>
    <div id="faceList"></div>
    <p class="hint">Shuffle picks a different face every hour.</p>
    <p id="faceError" role="alert" hidden></p>
    <h2>Location for Weather, Sun and moon, and Tide</h2>
    <div class="pass">
      <input id="latitude" type="number" step="any" min="-90" max="90" placeholder="Latitude" aria-label="Latitude">
      <input id="longitude" type="number" step="any" min="-180" max="180" placeholder="Longitude" aria-label="Longitude">
    </div>
    <button type="button" id="saveLocation">Save location</button>
    <p class="hint" id="locationHint">Your location is sent to Open-Meteo to get weather and tides. A rough location is fine.</p>
  </section>

  <details id="panels">
  <summary>Panel layout</summary>
  <div id="panelBox">
    <p class="hint">The clock now shows an orange L and a blue R. Use the buttons until the L reads correctly on the left panel and the R on the right. Panels are wired differently, so this is normal on a new build.</p>
    <button type="button" id="zigzag">Letters jumbled? Change row order</button>
    <button type="button" id="swap">Letters on the wrong sides? Swap panels</button>
    <button type="button" id="turnLeft"></button>
    <button type="button" id="turnRight"></button>
    <p id="panelError" role="alert" hidden></p>
    <button type="button" class="main" id="saveLayout">Save layout</button>
    <button type="button" id="cancelLayout">Cancel</button>
  </div>
  </details>

  <details id="wifi">
  <summary>Wi-Fi and timezone</summary>
  <form id="form">
    <div class="field">
      <div class="head"><h2>Nearby networks</h2><button type="button" class="small" id="rescan">Scan again</button></div>
      <div id="networks"><p class="hint">Looking for networks&hellip;</p></div>
    </div>
    <div class="field">
      <label for="ssid">Network name</label>
      <input id="ssid" maxlength="32" autocomplete="off" autocapitalize="none" spellcheck="false" required>
    </div>
    <div class="field">
      <label for="password">Password</label>
      <div class="pass">
        <input id="password" type="password" maxlength="63" autocomplete="off" autocapitalize="none" spellcheck="false">
        <button type="button" id="show">Show</button>
      </div>
      <p class="hint" id="passwordHint">Leave blank for an open network.</p>
    </div>
    <div class="field">
      <label for="zone">Timezone</label>
      <select id="zone"></select>
    </div>
    <div class="field" id="customField" hidden>
      <label for="custom">POSIX timezone</label>
      <input id="custom" maxlength="63" autocomplete="off" autocapitalize="none" spellcheck="false">
    </div>
    <p id="error" role="alert" hidden></p>
    <button class="main" id="save">Save and connect</button>
    <button type="button" id="forget" hidden>Forget this network</button>
  </form>
  </details>

  <section id="done" hidden>
    <h2>Saved</h2>
    <p id="doneText"></p>
  </section>
</main>
<script>
const $ = id => document.getElementById(id);
const ZONES = [
  ["UK and Ireland", "GMT0BST,M3.5.0/1,M10.5.0"],
  ["Central Europe", "CET-1CEST,M3.5.0,M10.5.0/3"],
  ["Eastern Europe", "EET-2EEST,M3.5.0/3,M10.5.0/4"],
  ["US Eastern", "EST5EDT,M3.2.0,M11.1.0"],
  ["US Central", "CST6CDT,M3.2.0,M11.1.0"],
  ["US Mountain", "MST7MDT,M3.2.0,M11.1.0"],
  ["US Pacific", "PST8PDT,M3.2.0,M11.1.0"],
  ["India", "IST-5:30"],
  ["Japan", "JST-9"],
  ["Sydney and Melbourne", "AEST-10AEDT,M10.1.0,M4.1.0/3"],
  ["New Zealand", "NZST-12NZDT,M9.5.0,M4.1.0/3"],
  ["UTC", "UTC0"],
];
const CUSTOM = "custom";
// Each number must match the Face enum in faces.h, where complex faces start at 26.
const SHUFFLE = ["Shuffle", 255];
const FACE_GROUPS = [
  ["Simple and clear", [["Sandbar", 0], ["Big digits", 2], ["Seconds bar", 11], ["Rainbow digits", 20], ["Marquee", 25], ["Word clock", 14]]],
  ["Animated digits", [["Digit sand", 1], ["Split-flap", 3], ["Rolling digits", 5], ["Digit morph", 49], ["Block builder", 40], ["Digit chase", 10]]],
  ["Nature and weather", [["Raindrops", 6], ["Snow globe", 8], ["Aurora", 18], ["Lava time", 7], ["Campfire", 17], ["Ocean voyage", 23], ["Aquarium", 24], ["Seasons", 44], ["Day to night", 41], ["Sun and moon", 4]]],
  ["Scenes and characters", [["City at night", 9], ["Pixel gardener", 15], ["Train journey", 42], ["Space mission", 43], ["Pixel pet", 45], ["Little town", 46], ["Haunted house", 47], ["Fireworks", 22], ["Starfield", 19], ["Code rain", 16], ["Heartbeat", 21]]],
  ["Simulations", [["Ant colony", 26], ["Predator and prey", 27], ["Falling sand", 28], ["Life", 29], ["Flocking birds", 30], ["Forest fire", 31], ["Ripple tank", 32], ["Crystal growth", 33]]],
  ["Self-playing games", [["Paddle duel", 34], ["Brick smasher", 35], ["Runner", 36], ["Space defender", 37], ["Maze solver", 38], ["Grazing worm", 39]]],
  ["Other ways to tell time", [["Analogue clock", 48], ["Pendulum", 50], ["Gears", 52], ["Marble run", 51], ["Binary columns", 12], ["Tally marks", 13]]],
  ["Live data", [["Weather window", 53], ["Real sun and moon", 54], ["Tide line", 55]]],
];
let status = {};
let scanId = 0;
let scanned = false;

const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));

function showError(text) {
  $("error").textContent = text;
  $("error").hidden = !text;
}

function signalLabel(net) {
  const strength = net.rssi > -60 ? "Strong" : net.rssi > -75 ? "Good" : "Weak";
  return net.open ? `${strength}, open` : strength;
}

function updatePasswordHint() {
  const saved = status.ssid && $("ssid").value === status.ssid;
  $("passwordHint").textContent = saved ? "Leave blank to keep the saved password." : "Leave blank for an open network.";
}

function chooseNetwork(net) {
  $("ssid").value = net.ssid;
  $("password").value = "";
  updatePasswordHint();
  document.querySelectorAll(".net").forEach(b => b.setAttribute("aria-pressed", b.dataset.ssid === net.ssid));
  if (!net.open) $("password").focus();
}

function renderNetworks(list) {
  const strongest = new Map();
  for (const net of list) {
    if (net.ssid && (!strongest.has(net.ssid) || strongest.get(net.ssid).rssi < net.rssi)) strongest.set(net.ssid, net);
  }
  const box = $("networks");
  box.replaceChildren();
  const sorted = [...strongest.values()].sort((a, b) => b.rssi - a.rssi);
  if (!sorted.length) {
    const none = document.createElement("p");
    none.className = "hint";
    none.textContent = "No networks found. You can type the name below.";
    box.append(none);
  }
  for (const net of sorted) {
    const b = document.createElement("button");
    b.type = "button";
    b.className = "net";
    b.dataset.ssid = net.ssid;
    b.setAttribute("aria-pressed", net.ssid === $("ssid").value);
    const name = document.createElement("span");
    name.textContent = net.ssid;
    const signal = document.createElement("span");
    signal.textContent = signalLabel(net);
    b.append(name, signal);
    b.addEventListener("click", () => chooseNetwork(net));
    box.append(b);
  }
}

async function scan() {
  const id = ++scanId;
  $("networks").innerHTML = '<p class="hint">Looking for networks&hellip;</p>';
  for (let attempt = 0; attempt < 20 && id === scanId; attempt++) {
    try {
      const data = await (await fetch("/scan")).json();
      if (id !== scanId) return;
      if (data.networks) { renderNetworks(data.networks); return; }
    } catch (e) { /* The radio can stall requests during a scan, so try again. */ }
    await sleep(1000);
  }
  if (id === scanId) renderNetworks([]);
}

function selectedTimezone() {
  return $("zone").value === CUSTOM ? $("custom").value.trim() : $("zone").value;
}

function fillZones(current) {
  const options = [...ZONES, ["Other (POSIX string)", CUSTOM]];
  $("zone").replaceChildren(...options.map(([label, value]) => new Option(label, value)));
  const known = ZONES.some(([, value]) => value === current);
  $("zone").value = known ? current : CUSTOM;
  $("custom").value = known ? "" : current;
  $("customField").hidden = known;
}

function describeStatus() {
  if (status.connected) return `Connected to ${status.ssid} at ${status.ip}.`;
  if (status.setup && status.ssid) return `Couldn't join ${status.ssid}. Check the details below.`;
  if (status.setup) return "Choose your Wi-Fi network to get started.";
  return status.ssid ? `Joining ${status.ssid}…` : "Not connected.";
}

function markFace(id) {
  document.querySelectorAll(".face").forEach(b => b.setAttribute("aria-pressed", b.dataset.face === String(id)));
}

async function chooseFace(id) {
  $("faceError").hidden = true;
  try {
    const response = await post("/face", { face: id });
    if (!response.ok) throw new Error(await response.text());
    markFace(id);
  } catch (e) {
    $("faceError").textContent = "The clock didn't change. Check you're still on its network, then try again.";
    $("faceError").hidden = false;
  }
}

function faceButton([name, id]) {
  const b = document.createElement("button");
  b.type = "button";
  b.className = "face";
  b.dataset.face = id;
  b.textContent = name;
  b.addEventListener("click", () => chooseFace(id));
  return b;
}

function faceGroup([title, faces]) {
  const group = document.createElement("details");
  group.className = "faceGroup";
  group.open = faces.some(([, id]) => id === status.face);
  const summary = document.createElement("summary");
  summary.textContent = `${title} (${faces.length})`;
  const list = document.createElement("div");
  list.className = "faceGrid";
  list.append(...faces.map(faceButton));
  group.append(summary, list);
  return group;
}

function renderFaces() {
  $("faceList").replaceChildren(faceButton(SHUFFLE), ...FACE_GROUPS.map(faceGroup));
  markFace(status.face);
  $("brightness").min = status.minBrightness;
  $("brightness").value = status.brightness;
  showBrightness();
  if (status.hasLocation) {
    $("latitude").value = status.latitude;
    $("longitude").value = status.longitude;
  }
  $("facesBox").hidden = false;
}

function showBrightness() {
  $("brightnessValue").textContent = `${$("brightness").value}%`;
}

let lastBrightnessSend = 0;

function sendBrightness(save) {
  const fields = { value: $("brightness").value };
  if (save) fields.save = 1;
  return post("/brightness", fields).catch(() => {
    $("faceError").textContent = "The clock didn't change. Check you're still on its network, then try again.";
    $("faceError").hidden = false;
  });
}

async function loadStatus() {
  try {
    status = await (await fetch("/status")).json();
    $("state").textContent = describeStatus();
    $("ssid").value = status.ssid;
    $("forget").hidden = !status.ssid;
    fillZones(status.timezone);
    updatePasswordHint();
    if (status.connected) renderFaces();
  } catch (e) {
    $("state").textContent = "Can't reach the clock. Check you're still on its network.";
  }
}

async function post(path, fields) {
  return fetch(path, { method: "POST", body: new URLSearchParams(fields) });
}

// The layout is one byte: bit 0 panel order, bit 1 row order, bits 2-4 left panel, bits 5-7 right panel.
let layout = 0;
let testingPanels = false;

function showPanelButtons() {
  $("turnLeft").textContent = `Change left panel (option ${((layout >> 2) & 7) + 1} of 8)`;
  $("turnRight").textContent = `Change right panel (option ${((layout >> 5) & 7) + 1} of 8)`;
}

async function sendLayout(fields) {
  $("panelError").hidden = true;
  try {
    const response = await post("/layout", fields);
    if (!response.ok) throw new Error(await response.text());
    return true;
  } catch (e) {
    $("panelError").textContent = "The clock didn't respond. Check you're still on its network, then try again.";
    $("panelError").hidden = false;
    return false;
  }
}

function changeLayout(next) {
  layout = next & 255;
  showPanelButtons();
  sendLayout({ value: layout });
}

function nextOption(shift) {
  const option = (((layout >> shift) & 7) + 1) & 7;
  changeLayout((layout & ~(7 << shift)) | (option << shift));
}

function showDone(text) {
  $("panels").hidden = true;
  $("wifi").hidden = true;
  $("facesBox").hidden = true;
  $("state").textContent = "Restarting…";
  $("doneText").textContent = text;
  $("done").hidden = false;
}

$("form").addEventListener("submit", async event => {
  event.preventDefault();
  showError("");
  const ssid = $("ssid").value;
  const timezone = selectedTimezone();
  if ($("zone").value === CUSTOM && !timezone) { showError("Enter a POSIX timezone, or choose one from the list."); return; }
  $("save").disabled = true;
  try {
    const response = await post("/save", { ssid, password: $("password").value, timezone });
    if (!response.ok) { showError(await response.text()); return; }
    showDone(`The clock is restarting and joining ${ssid}. Once it connects, you can reach this page at http://${status.host}.local. If it can't connect, the ${status.setupName} network comes back in about 30 seconds.`);
  } catch (e) {
    showError("The clock didn't respond. Check you're still on its network, then try again.");
  } finally {
    $("save").disabled = false;
  }
});

$("forget").addEventListener("click", async () => {
  showError("");
  try {
    const response = await post("/forget", {});
    if (!response.ok) { showError("The clock couldn't forget this network. Try again."); return; }
    showDone(`The saved network is gone. Join the ${status.setupName} network to set up a new one.`);
  } catch (e) {
    showError("The clock didn't respond. Check you're still on its network, then try again.");
  }
});

$("show").addEventListener("click", () => {
  const hidden = $("password").type === "password";
  $("password").type = hidden ? "text" : "password";
  $("show").textContent = hidden ? "Hide" : "Show";
});
$("zone").addEventListener("change", () => { $("customField").hidden = $("zone").value !== CUSTOM; });
$("ssid").addEventListener("input", updatePasswordHint);
$("rescan").addEventListener("click", scan);
$("brightness").addEventListener("input", () => {
  showBrightness();
  if (Date.now() - lastBrightnessSend < 150) return;
  lastBrightnessSend = Date.now();
  sendBrightness(false);
});
$("brightness").addEventListener("change", () => sendBrightness(true));
$("saveLocation").addEventListener("click", async () => {
  const latitude = $("latitude").value;
  const longitude = $("longitude").value;
  const hint = $("locationHint");
  if (!latitude || !longitude) { hint.textContent = "Enter both a latitude and a longitude."; return; }
  try {
    const response = await post("/location", { latitude, longitude });
    hint.textContent = response.ok ? "Location saved. Live faces update within a minute." : await response.text();
  } catch (e) {
    hint.textContent = "The clock didn't respond. Check you're still on its network, then try again.";
  }
});
$("zigzag").addEventListener("click", () => changeLayout(layout ^ 2));
$("swap").addEventListener("click", () => changeLayout(layout ^ 1));
$("turnLeft").addEventListener("click", () => nextOption(2));
$("turnRight").addEventListener("click", () => nextOption(5));
$("saveLayout").addEventListener("click", async () => {
  if (!await sendLayout({ value: layout, save: 1 })) return;
  status.layout = layout;
  testingPanels = false;
  $("panels").open = false;
});
$("cancelLayout").addEventListener("click", () => { $("panels").open = false; });
// Opening the section starts the test pattern. Closing it without saving goes back to the saved layout.
$("panels").addEventListener("toggle", () => {
  if ($("panels").open) {
    layout = status.layout;
    testingPanels = true;
    changeLayout(layout);
  } else if (testingPanels) {
    testingPanels = false;
    sendLayout({ cancel: 1 });
  }
});
// Scanning interrupts Wi-Fi briefly, so it only runs when the Wi-Fi section is open.
$("wifi").addEventListener("toggle", () => {
  if ($("wifi").open && !scanned) {
    scanned = true;
    scan();
  }
});

loadStatus().then(() => { if (!status.connected) $("wifi").open = true; });
</script>
</body>
</html>
)html";
