# Sandbar Clock

A 16×8 clock for an ESP32-C3 SuperMini and two 8×8 WS2812B panels. An ESP32-S3 build is also included. The time is always on, and a grain of sand drops through the middle every 2 minutes.

## Wiring

| From | To |
|---|---|
| 5V supply (2A or more) | 5V on both panels |
| Supply ground | Ground on both panels and the ESP32 |
| ESP32 data pin (GPIO 4 on the C3, GPIO 12 on the S3) | 330Ω resistor, then DIN on the first panel |
| DOUT on the first panel | DIN on the second panel |

- These panels work from a 3.3V data signal at 5V. If yours don't, add a 74AHCT125 level shifter before the resistor.
- The `5V` pin on some ESP32-S3 boards isn't connected to USB unless the `IN-OUT` solder jumper is bridged.
- A 1000µF capacitor across the panel power input protects the LEDs at switch-on.
- If you power the ESP32 from USB and the panels from a separate supply, don't also connect the supply's 5V to its 5V pin. Keep the grounds connected.

## Setup

The easiest way to install is the [browser installer](https://benleachps4-cloud.github.io/sandbar-clock/), which needs Chrome or Edge on a computer. The steps below are for building from source.

If your data wire is on a different pin, change `-DLED_PIN` for your board in `platformio.ini`.

Build and upload from the PlatformIO panel in VS Code, or run this in the project folder. It builds for the C3 by default:

```bash
pio run -t upload
```

For the S3, add `-e esp32s3` to that command.

## Connect it to Wi-Fi

1. Power on the clock. With no saved network, it shows four dashes and a blue dot.
2. On your phone or laptop, join the `Sandbar-Clock-Setup` Wi-Fi network. The password is `sandbar1234`.
3. The setup page should open by itself. If it doesn't, go to `http://192.168.4.1`.
4. Choose your network, enter its password, pick your timezone and select **Save and connect**. The network must be 2.4 GHz.

The clock restarts and joins your network. The dot turns green once it's connected, then the time appears.

To change settings later, go to `http://sandbar-clock.local` or the clock's IP address from your router. Some devices don't support `.local` addresses, so use the IP address on those.

If the clock can't join its saved network within 20 seconds of starting up, it opens the setup network again. It retries the saved network after 3 minutes with nobody connected to setup, so it recovers by itself after a router outage.

Change `SETUP_NETWORK_PASSWORD` in `src/config.h` if you don't want to use the default. Like WLED, the settings page has no login, so anyone on your home network can change the clock's Wi-Fi and timezone. The page never shows the saved Wi-Fi password.

## Set the panel layout

Open **Panel layout** on the clock's page. The clock shows an orange L and a blue R, and each button takes effect straight away.

| What you see | Button |
|---|---|
| Letters are jumbled, with rows out of line | **Change row order** |
| Each letter is on the wrong side | **Swap panels** |
| A letter is mirrored, upside down or on its side | **Change left panel** or **Change right panel**, which each have 8 options |

Select **Save layout** when both letters read correctly. If you leave the page without saving, the clock goes back to the saved layout after 5 minutes.

The values in `config.h` set the layout for a clock that has never saved one.

## Clock faces

Open the clock's page and choose a face. The choice takes effect straight away and is saved. The brightness slider on the same page changes the display as you drag it and saves when you let go. The display still dims automatically at night, in proportion to that setting.

| Face | What it does |
|---|---|
| Sandbar | Digits with sand collecting along the bottom |
| Digit sand | Old digits crumble and fall, then new ones build up each minute |
| Big digits | Tall digits using all 8 rows |
| Split-flap | Digits flip over like a departure board |
| Sun and moon | A sun or moon crosses the sky, with day taken as 06:00 to 18:00 |
| Rolling digits | Digits slide up like an odometer |
| Raindrops | Rain falls behind the digits, and the water level shows the minutes |
| Lava time | Icy digits over slowly moving lava |
| Snow globe | Blue snow settles on amber digits and shakes off each minute |
| City at night | The time above a skyline with changing windows |
| Digit chase | A small creature stomps each digit as it changes |
| Seconds bar | Digits with a bar along the bottom that fills every minute |
| Binary columns | Each digit shown as a column of on and off blocks, read as 8, 4, 2, 1 from the top |
| Word clock | Scrolls the time in words, to the nearest 5 minutes |
| Pixel gardener | A small gardener waters each plant in turn, and flowers grow from 06:00 to 18:00 |
| Code rain | Green streams fall behind the digits |
| Campfire | Flickering flames below the time |
| Aurora | Slow green and purple curtains behind the digits |
| Starfield | Stars fly out from the centre behind amber digits |
| Rainbow digits | Colours flow across the digits |
| Heartbeat | A pulse trace runs below the time |
| Fireworks | Bursts go off behind the digits |
| Ocean voyage | A boat crosses the sea once an hour |
| Aquarium | Fish swim below the time as bubbles rise behind it |
| Marquee | Chasing lights along the top and bottom, like a theatre sign |
| Simulations | Ant colony, Predator and prey, Falling sand, Life, Flocking birds, Forest fire, Ripple tank, Crystal growth |
| Self-playing games | Paddle duel, Brick smasher, Runner, Space defender, Maze solver, Grazing worm, Block builder |
| Scenes | Day to night, Train journey, Space mission, Seasons, Pixel pet, Little town, Haunted house |
| Other ways to show time | Digit morph, Pendulum |
| Retro | Red LED alarm, Vacuum display, Nixie tubes, Twin-bell alarm, which rings for the first 12 seconds of each hour, Clock radio, Pocket LCD, Flip-card bedside, Neon sign, TV test card, Amber terminal, Hi-fi VU meter, Bubble LED calculator |
| Live data | Weather window, Real sun and moon, Tide line. Set your location on the page first |
| Shuffle | Picks a different face every hour |

## What it shows

Four dashes and a moving dot along the bottom mean it's waiting for the time. The dot is blue in setup mode, amber while joining Wi-Fi and green once connected.

The sand colour changes at `DAY_START_HOUR`, `DUSK_START_HOUR` and `NIGHT_START_HOUR`, and the display dims at night. `MAX_MILLIAMPS` caps the current draw.
