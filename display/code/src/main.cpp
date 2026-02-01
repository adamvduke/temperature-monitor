#include "config.h"
#include "AdafruitIO_WiFi.h"
#include "Adafruit_ThinkInk.h"
#include <Wire.h>

#define SRAM_CS 16
#define EPD_CS 0
#define EPD_DC 15

// Feather ESP8266 default pins (SDA=4, SCL=5)
#define PIN_SDA 4
#define PIN_SCL 5

#define EPD_RESET -1
#define EPD_BUSY -1

// time intervals in milliseconds
#define ONE_SECOND 1000
#define ONE_MINUTE 60 * ONE_SECOND

// action intervals
#define FAST_DISPLAY_UPDATES 0
#if FAST_DISPLAY_UPDATES
#define DISPLAY_UPDATE_INTERVAL (20 * ONE_SECOND)
#else
#define DISPLAY_UPDATE_INTERVAL (5 * ONE_MINUTE)
#endif

#define FEED_GET_INTERVAL (1 * ONE_MINUTE)
#define IO_RUN_INTERVAL (5 * ONE_SECOND)

#define CONNECT_ITERATION_DELAY 2000

#define BORDER_WIDTH 3
#define DEFAULT_DATA_VALUE -50.0  // nearly impossible temp and definitely impossible humidty

// https://www.adafruit.com/product/4777 - 296 x 128
// https://github.com/adafruit/Adafruit_EPD/blob/master/src/panels/ThinkInk_290_Grayscale4_EAAMFGN.h
// https://github.com/adafruit/Adafruit_EPD/blob/master/src/drivers/Adafruit_SSD1680.h
// https://github.com/adafruit/Adafruit_EPD/blob/master/src/drivers/Adafruit_SSD1680.cpp
ThinkInk_290_Grayscale4_EAAMFGN display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

// adafruit connection
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);

// data feeds
AdafruitIO_Feed *tempFeed = io.feed(TEMP_C_FEED);
AdafruitIO_Feed *humidityFeed = io.feed(HUMIDITY_FEED);

// XX.XX C\n
// XX.XX F\n
// XX.XX %
char buffer[32];

struct ProgramState {
  // tracking the last time each of the actions was triggered
  unsigned long previousRunMillis = 0;
  unsigned long previousFeedMillis = 0;
  unsigned long previousUpdateMillis = 0;

  // tracking if a temp or humidity update has been received since the last
  // display update
  bool tempUpdated = false;
  bool humidityUpdated = false;

  // tracking the last temp or humidty reading
  float lastTemp = DEFAULT_DATA_VALUE;
  float lastHum = DEFAULT_DATA_VALUE;
};

ProgramState st;

void updateTemp(AdafruitIO_Data *data);
void updateHumidity(AdafruitIO_Data *data);
void updateDisplay(int borderWidth, uint16_t fillColor, uint16_t drawColor);
float cToF(float celcius);
void drawBorder(int borderWidth, uint16_t color);
void drawPaddedText(int xoff, int yoff, const char *text, int len, uint16_t color);

void setup() {
  Serial.begin(115200);
  display.begin();

  // Initialize I2C
  Wire.begin(PIN_SDA, PIN_SCL);

  // connect to io.adafruit.com
  io.connect();

  // wait for a connection
  int attempts = 0;
  while (io.status() < AIO_CONNECTED) {
    attempts++;
    Serial.printf("Not connected, attempt: %d\n", attempts);
    Serial.printf("io.status(): %d\n", io.status());
    Serial.printf("io.networkStatus(): %d\n", io.networkStatus());
    Serial.printf("io.mqttStatus(): %d\n", io.mqttStatus());
    delay(CONNECT_ITERATION_DELAY);
  }
  Serial.print("Connected: ");
  Serial.println(io.statusText());

  // set up handlers for incoming feed data
  tempFeed->onMessage(updateTemp);
  humidityFeed->onMessage(updateHumidity);
}

void loop() {
  unsigned long currentMillis = millis();

  // trigger io.run() at most every IO_RUN_INTERVAL
  if (currentMillis - st.previousRunMillis >= IO_RUN_INTERVAL) {
    io.run();
    st.previousRunMillis = currentMillis;
  }

  // trigger feed requests at most every FEED_GET_INTERVAL
  if (currentMillis - st.previousFeedMillis >= FEED_GET_INTERVAL) {
    // send /get messages, which triggers the onMessage handler functions
    tempFeed->get();
    humidityFeed->get();
    st.previousFeedMillis = currentMillis;
  }

  // skip the display update on the first loop by returning early to avoid
  // writing values that haven't been populated to the display
  if (st.previousUpdateMillis == 0) {
    st.previousUpdateMillis = currentMillis;
    return;
  }

  // trigger display updates at most every DISPLAY_UPDATE_INTERVAL
  if (currentMillis - st.previousUpdateMillis >= DISPLAY_UPDATE_INTERVAL) {
    // only update if the current values aren't the default and there's an update since the last updateDisplay()
    if ((st.lastTemp != DEFAULT_DATA_VALUE || st.lastHum != DEFAULT_DATA_VALUE) && (st.tempUpdated || st.humidityUpdated)) {
      updateDisplay(BORDER_WIDTH, EPD_WHITE, EPD_BLACK);
      st.tempUpdated = false;
      st.humidityUpdated = false;
    }
    st.previousUpdateMillis = currentMillis;
  }
}

void updateTemp(AdafruitIO_Data *data) {
  st.lastTemp = data->toFloat();
  st.tempUpdated = true;
}

void updateHumidity(AdafruitIO_Data *data) {
  st.lastHum = data->toFloat();
  st.humidityUpdated = true;
}

void updateDisplay(int borderWidth, uint16_t fillColor, uint16_t drawColor) {
  float far = cToF(st.lastTemp);
  int len = snprintf(buffer, sizeof(buffer), "%.2f C\n%.2f F\n%.2f %%", st.lastTemp, far, st.lastHum);

  // only update the display if there's something relevant to show
  if (len > 0) {
    display.fillScreen(fillColor);
    drawBorder(borderWidth, drawColor);
    int xoff = BORDER_WIDTH * 15;  // start to the right a bit
    int yoff = BORDER_WIDTH * 2;   // start down from the border a bit
    drawPaddedText(xoff, yoff, buffer, len, drawColor);
    display.display();
  }
}

float cToF(float celcius) {
  // use floats in division to avoid integer truncation
  return ((9.0 / 5.0) * celcius) + 32;
}

// draws a border around the outside of the display
void drawBorder(int borderWidth, uint16_t color) {
  // top border
  display.fillRect(0, 0, display.width(), borderWidth, color);
  // right border
  display.fillRect(display.width() - borderWidth, borderWidth, borderWidth, display.height() - borderWidth, color);
  // bottom border
  display.fillRect(0, display.height() - borderWidth, display.width() - borderWidth, borderWidth, color);
  // left border
  display.fillRect(0, borderWidth, borderWidth, display.height() - borderWidth, color);
}

void drawPaddedText(int xoff, int yoff, const char *text, int len, uint16_t color) {
  display.setTextWrap(false);
  display.setTextColor(color);
  display.setTextSize(5);
  display.setCursor(xoff, yoff);

  int idx = 0;
  int lineno = 0;
  while (idx < len) {
    // walk the text character at a time
    char c = text[idx];
    if (c == '\n') {
      // if the char is \n, do a newline
      lineno++;
      // set the x offset back, and move down some amount
      //TODO: make 40 some function of the text size
      display.setCursor(xoff, yoff + (40 * lineno));
    } else {
      display.print(c);
    }
    idx++;
  }
}
