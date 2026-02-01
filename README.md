# Temperature Monitor

A Wippersnapper/Arduino based project for monitoring and displaying temperature and humidity data using Adafruit IO.

## Project Structure

### Display (`/display`)

The display directory contains a PlatformIO project that uses an e-ink display to show temperature and humidity data from Adafruit IO feeds. It fetches temperature and humidity data from Adafruit IO and displays them on a 2.9" grayscale e-ink display.

The e-ink display updates every 5 minutes to conserve power and extend display life.

The display directory also contains CAD files for a 3D printable case that holds the e-ink display, designed to be used as a desk accessory.

### Logger (`/logger`)

The logger directory contains CAD files for a 3D printable case that holds:
- Raspberry Pi [wall adapter](https://www.raspberrypi.com/products/type-c-power-supply/)
- Adafruit [Qt Py ESP32-S2](https://www.adafruit.com/product/5325) 
- Adafruit [AHT20](https://www.adafruit.com/product/4566)
- Adafruit [STEMMA QT Cable](https://www.adafruit.com/product/4399)

The logger uses Adafruit Wippersnapper firmware (no custom code required) to log temperature and humidity data to Adafruit IO, which is then consumed by the display sub-project.

## Display Overview

## Display Hardware Requirements

- **Microcontroller**: Developed on an ESP8266 Feather, likely to work on other feathers with the correct pin configuration
- **Display**: Adafruit 2.9" Grayscale eInk / ePaper Display FeatherWing ([Product #4777](https://www.adafruit.com/product/4777))
  - Resolution: 296 x 128 pixels
  - Driver: SSD1680
- **Sensors**: Temperature/humidity sensor publishing to Adafruit IO (separate device)

## Display Setup

1. **Copy configuration file:**
   ```bash
   cp config.h.example config.h
   ```

2. **Edit `config.h` with your credentials:**
   - `IO_USERNAME`: Your Adafruit IO username
   - `IO_KEY`: Your Adafruit IO key
   - `WIFI_SSID`: Your WiFi network name
   - `WIFI_PASS`: Your WiFi password
   - `TEMP_C_FEED`: Name of your temperature feed (in Celsius)
   - `HUMIDITY_FEED`: Name of your humidity feed

## Configuration Options

### Update Intervals

In `main.cpp`, you can adjust:

- `DISPLAY_UPDATE_INTERVAL`: How often the display refreshes (default: 5 minutes)
- `FEED_GET_INTERVAL`: How often to send the `get()` message to the mqtt topic (default: 1 minute)
- `IO_RUN_INTERVAL`: How often to run `io.run` which maintains/repairs connections to io.adafruit.com (default: 5 seconds)

### Fast Display Mode

For testing, you can enable faster display updates:
```cpp
#define FAST_DISPLAY_UPDATES 1
```
This changes the display update interval to 20 seconds. E-ink displays have a limited lifespan in terms of refreshes. The default is 5 minutes to avoid harming the display life, but that's a long time to wait during development.
