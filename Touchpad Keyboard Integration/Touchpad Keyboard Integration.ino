#include "SPI.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include <LoRa.h>

#define FREQ 915E6
String textToSend, textToDisplay;
bool touchHandled = false;
uint16_t touchEventX, touchEventY;

/* Hardware SPI:
 * MKR pin  8 to shield pin 11 (MOSI)
 * MKR pin  9 to shield pin 13 (SCK)
 * MKR pin 10 to shield pin 12 (MISO) */
#define TFT_CS 5 // Shield pin 10 
#define TFT_DC 4 // Shield pin 9
#define TS_CS 3  // Shield pin 8

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_STMPE610 ts = Adafruit_STMPE610(TS_CS);

#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

#define KEY_WIDTH 32
#define KEY_HEIGHT 40
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define TEXT_OFFSET_X 11
#define TEXT_OFFSET_Y 12

String keys[4][10] = {
  { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" },
  { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p" },
  { "a", "s", "d", "f", "g", "h", "j", "k", "l", " " },
  { "z", "x", "c", "v", "b", "n", "m", ".", "?", " " }
};

String buffer = String("");

void drawKeyboardGrid() {

  // Draw "Output" row
  tft.drawRect(0, 0, SCREEN_WIDTH, KEY_HEIGHT, ILI9341_WHITE);

  // Draw alpha-numeric key rows
  for (int rows = 1; rows < 5; rows++) {
    for (int cols = 0; cols < 10; cols++) {
      tft.drawRect(cols * KEY_WIDTH, rows * KEY_HEIGHT, KEY_WIDTH, KEY_HEIGHT, ILI9341_WHITE);
    }
  }
  // Draw "cursor control" row
  // "Delete"
  tft.drawRect(0, KEY_HEIGHT * 5, KEY_WIDTH * 2, KEY_HEIGHT, ILI9341_WHITE);
  // "Space"
  tft.drawRect(KEY_WIDTH * 2, KEY_HEIGHT * 5, KEY_WIDTH * 6, KEY_HEIGHT, ILI9341_WHITE);
  // "Send"
  tft.drawRect(KEY_WIDTH * 8, KEY_HEIGHT * 5, KEY_WIDTH * 2, KEY_HEIGHT, ILI9341_WHITE);
}

void labelKeys() {

  // Draw alpha-numeric key rows
  for (int rows = 1; rows < 5; rows++) {
    for (int cols = 0; cols < 10; cols++) {
      tft.setCursor(cols * KEY_WIDTH + TEXT_OFFSET_X, rows * KEY_HEIGHT + TEXT_OFFSET_Y);
      tft.print(keys[rows - 1][cols]);
    }
  }

  // Delete
  tft.setCursor(TEXT_OFFSET_X, 5 * KEY_HEIGHT + TEXT_OFFSET_Y);
  tft.print("DEL");
  // Space
  tft.setCursor(KEY_WIDTH * 4 + TEXT_OFFSET_X, 5 * KEY_HEIGHT + TEXT_OFFSET_Y);
  tft.print("SPACE");
  //Send
  tft.setCursor(KEY_WIDTH * 8 + TEXT_OFFSET_X, 5 * KEY_HEIGHT + TEXT_OFFSET_Y);
  tft.print("SEND");
}


void displayText(String output) {
  tft.fillRect(1, 1, 318, KEY_HEIGHT - 2, ILI9341_BLACK);
  tft.setCursor(TEXT_OFFSET_X, TEXT_OFFSET_Y);
  tft.print(output);
}

void setup() {

  Serial.begin(9600);
  while (!Serial);

  Serial.println("Initialzing LoRa");
  if (!LoRa.begin(FREQ)) {
    Serial.println("LoRa initialization failed");
  } else {
    Serial.println("LoRa started");
  }

  Serial.println("Initialzing Touchscreen");
  tft.begin();
  if (!ts.begin()) {
    Serial.println("Unable to start touchscreen.");
  } else {
    Serial.println("Touchscreen started.");
  }

  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(1);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  drawKeyboardGrid();
  labelKeys();
}

void loop() {
  
  // My hacky way of capturing only one value per touch event...
  if (ts.touched()) {
    touchHandled = false;
    while (!ts.bufferEmpty()) {
      TS_Point p = ts.getPoint();
      touchEventX = map(p.x, TS_MINY, TS_MAXY, 0, tft.height());
      touchEventY = map(p.y, TS_MINX, TS_MAXX, 0, tft.width());
    }
  } else if (!ts.touched() && !touchHandled) {
    // The TFT is rotated, but that doesn't apply to the touch screen.
    // The end result is that we have to do some seemingly unintuitive
    // math to convert from default coords to rotated coords. We also
    // scale down the coords here, so that they represent grid cells
    // instead of pixels.
    int mappedX = touchEventY / KEY_WIDTH;
    int mappedY = 5 - touchEventX / KEY_HEIGHT;

    if (mappedY > 0 && mappedY < 5) {
      String character = keys[mappedY - 1][mappedX];
      if (!character.equals(" ")) {
        buffer.concat(character);
      }
    } else if (mappedY == 5) {
      if (mappedX < 2) { // DEL
        if (buffer.length() > 0) {
          buffer.remove(buffer.length() - 1);
        }
      } else if (mappedX > 7) { // SEND
        LoRa.beginPacket();
        LoRa.print(buffer);
        LoRa.endPacket();

        buffer.remove(0);
      } else { // SPACE
        buffer.concat(" ");
      }
    }

    Serial.print("Buffer: ");
    Serial.println(buffer);
    touchHandled = true;
  }

  // Legacy code to keep LoRa and serial monitor input available
  if (Serial.available()) {

    while (Serial.available()) {
      textToSend = Serial.readString();
    }
    Serial.print("Sending packet");
    LoRa.beginPacket();
    LoRa.print(textToSend);
    LoRa.endPacket();
  }

  if (!LoRa.parsePacket()) return;

  Serial.print("Received packet");
  while (LoRa.available()) textToDisplay = LoRa.readString();
  displayText(textToDisplay);
}