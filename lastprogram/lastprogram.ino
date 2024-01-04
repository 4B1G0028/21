#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// 硬體引腳定義
const int RST_PIN = 9;
const int SS_PIN = 10;
const int rgbLedRedPin = 2;
const int rgbLedGreenPin = 3;
const int rgbLedBluePin = 4;

// MFRC522 實體和 LCD 實體
MFRC522 mfrc522;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 連續錯誤計數器和最大連續錯誤次數
int consecutiveErrors = 0;
const int maxConsecutiveErrors = 3;
bool isLocked = false;
unsigned long startTime = 0;
const unsigned long waitTime = 5000;  // 5 秒

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.backlight();

  SPI.begin();
  mfrc522.PCD_Init(SS_PIN, RST_PIN);
  Serial.println(F("感應器：初始化完成"));

  pinMode(rgbLedRedPin, OUTPUT);
  pinMode(rgbLedGreenPin, OUTPUT);
  pinMode(rgbLedBluePin, OUTPUT);

  rgbLedBlink(255, 255, 255, 500, 5);  // 初始狀態，閃爍白色
  lcdDisplay("pls push your ","card");
}

void loop() {
  if (isLocked) {
    lcdDisplay("system lock", "pls waiting...");
    rgbLedSetColor(255, 0, 255);  // 紫色燈
    delay(1000);
    isLocked = false;
    lcdDisplay("pls push your ","card");
    rgbLedBlink(255, 255, 255, 500, 3);  // 開始閃白光
  }

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("card HEX UID:"));
    dumpByteArrayToLCD(mfrc522.uid.uidByte, mfrc522.uid.size);

    lcd.setCursor(0, 1);
    lcd.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    lcd.print(mfrc522.PICC_GetTypeName(piccType));

    if (checkUID(mfrc522.uid.uidByte, mfrc522.uid.size)) {
      rgbLedSetColor(0, 255, 0);  // 綠色燈，解鎖成功
      lcdDisplay("ok！","");
      delay(1000);
      consecutiveErrors = 0;  // 重置錯誤計數器
      stopRedLedBlink();      // 停止閃紅燈
    } else {
      rgbLedSetColor(255, 0, 0);  // 紅色燈，解鎖失敗
      lcdDisplay("nono","");
      delay(1000);
      consecutiveErrors++;

      if (consecutiveErrors >= maxConsecutiveErrors) {
        startRedLedBlink();  // 開始閃紅燈
        lcdDisplay("system lock", "pls waiting...");
        isLocked = true; // 鎖定系統
        startTime = millis(); // 記錄開始時間
        while (millis() - startTime < waitTime) {
          // 等待時間結束前一直閃紅燈
          rgbLedSetColor(255, 0, 0);
          delay(500);
          rgbLedSetColor(0, 0, 0);
          delay(500);
        }
        consecutiveErrors = 0; // 重置錯誤計數器
        stopRedLedBlink(); // 停止閃紅燈
        lcdDisplay("pls push your", "card");
        rgbLedBlink(255, 255, 255, 500, 5); // 開始閃白光
      }
    }

    mfrc522.PICC_HaltA();
    lcdDisplay("pls push your", "card");
    rgbLedBlink(255, 255, 255, 500, 5);  // 開始閃白光
    delay(1000);                         // 等待一秒
  }
}

void dumpByteArrayToLCD(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    lcd.print(buffer[i] < 0x10 ? " 0" : " ");
    lcd.print(buffer[i], HEX);
  }
}

bool checkUID(byte *uidBuffer, byte bufferSize) {
  byte expectedUID[] = { 0xD3, 0x08, 0xF4, 0x0F };
  if (bufferSize != sizeof(expectedUID)) {
    return false;
  }
  for (byte i = 0; i < bufferSize; i++) {
    if (uidBuffer[i] != expectedUID[i]) {
      return false;
    }
  }
  return true;
}

void rgbLedSetColor(int red, int green, int blue) {
  analogWrite(rgbLedRedPin, red);
  analogWrite(rgbLedGreenPin, green);
  analogWrite(rgbLedBluePin, blue);
}

void rgbLedBlink(int red, int green, int blue, int duration, int blinkCount) {
  for (int i = 0; i < blinkCount; i++) {
    rgbLedSetColor(red, green, blue);
    delay(duration);
    rgbLedSetColor(0, 0, 0);
    delay(duration);
  }
}

void lcdDisplay(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void startRedLedBlink() {
  rgbLedBlink(255, 0, 0, 500, 10);  // 開始閃紅燈
}

void stopRedLedBlink() {
  rgbLedSetColor(0, 0, 0);  // 停止閃紅燈
}
