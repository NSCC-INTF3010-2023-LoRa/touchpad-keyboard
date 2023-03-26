#include <Keypad.h>
#include "SPI.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <LoRa.h>

#define FREQ 915E6
String textToSend, textToDisplay;

#define TFT_CS 5
#define TFT_DC 4

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define UI_ROW_HEIGHT 40
#define TEXT_OFFSET_X 11
#define TEXT_OFFSET_Y 12

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

const byte ROWS = 4;
const byte COLS = 3;
char controlKey;
int mappedKeyIndex, keyIndexValue;
int currentKeyIndex = -1;
int currentStringLength;
int messagesDisplayed = 0;

char keyMapping[10][4] = {
  { 'a', 'b', 'c', '1' },
  { 'd', 'e', 'f', '2' },
  { 'g', 'h', 'i', '3' },
  { 'j', 'k', 'l', '4' },
  { 'm', 'n', 'o', '5' },
  { 'p', 'q', 'r', '6' },
  { 's', 't', 'u', '7' },
  { 'v', 'w', 'x', '8' },
  { 'y', 'z', '.', '9' },
  { ',', '?', ' ', '0' }
};

char hexaKeys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};

byte rowPins[ROWS] = { A5, A4, A3, A2 };
byte colPins[COLS] = { A1, A0, 3 };

Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

void drawUIGrid() {
  // Draw input border
  tft.drawRect(0, 0, SCREEN_WIDTH, UI_ROW_HEIGHT, ILI9341_WHITE);
  //Draw output border
  tft.drawRect(0, UI_ROW_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT, ILI9341_WHITE);
}

void handledKeypadInput(char keyPressed) {

  switch (keyPressed) {
    case '1':
      mappedKeyIndex = 0;
      break;
    case '2':
      mappedKeyIndex = 1;
      break;
    case '3':
      mappedKeyIndex = 2;
      break;
    case '4':
      mappedKeyIndex = 3;
      break;
    case '5':
      mappedKeyIndex = 4;
      break;
    case '6':
      mappedKeyIndex = 5;
      break;
    case '7':
      mappedKeyIndex = 6;
      break;
    case '8':
      mappedKeyIndex = 7;
      break;
    case '9':
      mappedKeyIndex = 8;
      break;
    case '0':
      mappedKeyIndex = 9;
      break;
    case '*':
      mappedKeyIndex = 10;
      break;
    case '#':
      mappedKeyIndex = 11;
      break;
    default:
      Serial.println("Unhandled keypad press error");
      break;
  }

  if (mappedKeyIndex != currentKeyIndex) {
    if (mappedKeyIndex == 10) deleteCharacter();
    else if (mappedKeyIndex == 11) sendLoRaPacket();
    else {
      currentKeyIndex = mappedKeyIndex;
      keyIndexValue = 0;
      textToSend += keyMapping[currentKeyIndex][keyIndexValue];
    }
  } else {
    if (mappedKeyIndex == 10) deleteCharacter();
    else if (mappedKeyIndex == 11)sendLoRaPacket();
    else {
      currentStringLength = textToSend.length();
      keyIndexValue++;
      if (keyIndexValue > 3) keyIndexValue = 0;
      textToSend.setCharAt(currentStringLength - 1, keyMapping[currentKeyIndex][keyIndexValue]);
    }
  }
  updateInputBox();
}

void updateInputBox() {
  tft.fillRect(0, 0, SCREEN_WIDTH, UI_ROW_HEIGHT, ILI9341_BLACK);
  drawUIGrid();
  tft.setCursor(TEXT_OFFSET_X, TEXT_OFFSET_Y);
  tft.setTextColor(ILI9341_GREEN);
  tft.print(textToSend);
}

void deleteCharacter() {
  textToSend.remove(textToSend.length() - 1, 1);
}

void sendLoRaPacket() {
  Serial.print("Sending packet");
  LoRa.beginPacket();
  LoRa.print(textToSend);
  LoRa.endPacket();
  displayText(textToSend, "Sent");
  textToSend = "";
}

void displayText(String output, String type) {
  messagesDisplayed++;
  tft.fillRect(0, UI_ROW_HEIGHT * messagesDisplayed, SCREEN_WIDTH, UI_ROW_HEIGHT, ILI9341_BLACK);
  tft.setCursor(TEXT_OFFSET_X, UI_ROW_HEIGHT * messagesDisplayed + TEXT_OFFSET_Y);
  tft.setTextColor(ILI9341_GREEN);
  tft.print(type + ": " + output);
  drawUIGrid();
}

void setup() {

  Serial.begin(9600);

  // Using analog pins for digital input so need to set these up explicitly
  digitalRead(A0);
  digitalRead(A1);
  digitalRead(A2);
  digitalRead(A3);
  digitalRead(A4);
  digitalRead(A5);

  Serial.println("Initialzing LoRa");
  if (!LoRa.begin(FREQ)) {
    Serial.println("LoRa initialization failed");
  } else {
    Serial.println("LoRa started");
  }

  tft.begin();

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_GREEN);
  tft.setRotation(3);
  tft.setTextSize(2);
  drawUIGrid();
}

void loop() {

  char keyPressed = keypad.getKey();

  if (keyPressed) {
    handledKeypadInput(keyPressed);
  }

  if (!LoRa.parsePacket()) return;

  Serial.print("Received packet");
  while (LoRa.available()) textToDisplay = LoRa.readString();
  displayText(textToDisplay, "Received");
}