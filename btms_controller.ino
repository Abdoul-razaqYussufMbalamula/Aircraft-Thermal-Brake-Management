#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <max6675.h>

// -------------------- MAX6675 PINLERI --------------------
const int thermoCLK = 6;   // SoftSPI: SCK
const int thermoCS  = 7;   // CS
const int thermoDO  = 8;   // SO (DO)
MAX6675 thermo(thermoCLK, thermoCS, thermoDO);

// -------------------- FAN (IRF520) --------------------
const int fanPin = 5;      // PWM pin
int fanPWM = 0;
float fanPercent = 0.0;

// -------------------- SD KART --------------------
const int chipSelect = 4;
bool sdOK = false;

// -------------------- LCD --------------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// -------------------- BUZZER --------------------
const int buzzerPin = 9;

// -------------------- KONTROL PARAMETRELERI --------------------
unsigned long lastReadTime = 0;
const unsigned long readInterval = 1000; // 1 saniye

const float T_min = 35.0;   // Fan kapalı
const float T_max = 60.0;   // Fan tam hız
const float T_alarm = 65.0; // Buzzer alarm eşiği

// ------------------------------------------------------------
void setup() {
  Serial.begin(9600);

  // FAN
  pinMode(fanPin, OUTPUT);
  analogWrite(fanPin, 0);

  // BUZZER
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("AKILLI TERMAL");

  lcd.setCursor(0,1);
  lcd.print("FREN YONETIMI");

  delay(2000);   // 2 saniye bekle
  lcd.clear();


  // SD KART BASLATMA (3 kez dener)
  lcd.clear();
  lcd.print("SD kontrol...");
  for (int i = 0; i < 3; i++) {
    if (SD.begin(chipSelect)) {
      sdOK = true;
      break;
    }
    delay(300);
  }

  if (sdOK) {
    lcd.setCursor(0,1);
    lcd.print("SD OK");
    File f = SD.open("log.csv", FILE_WRITE);
    if (f) {
      f.println("time_s,temperature_C,fan_percent");
      f.close();
    }
  } else {
    lcd.setCursor(0,1);
    lcd.print("SD HATA");
  }

  delay(1000);
  lcd.clear();
}

// ------------------------------------------------------------
void loop() {
  unsigned long now = millis();

  if (now - lastReadTime >= readInterval) {
    lastReadTime = now;

    double tempC = thermo.readCelsius();

    Serial.print("T = ");
    Serial.print(tempC);
    Serial.println(" C");

    updateFanPWM(tempC);
    updateBuzzer(tempC);
    updateLCD(tempC);
    logToSD(tempC);
  }
}

// ---------------- FAN HIZI ----------------
void updateFanPWM(float T) {

  if (T <= T_min) {
    fanPWM = 0;
  } 
  else if (T >= T_max) {
    fanPWM = 255;
  } 
  else {
    fanPWM = map((int)(T * 10),
                 (int)(T_min * 10),
                 (int)(T_max * 10),
                 80, 255);
  }

  analogWrite(fanPin, fanPWM);
  fanPercent = (fanPWM / 255.0) * 100.0;
}

// ---------------- BUZZER ----------------
void updateBuzzer(float T) {
  if (T >= T_alarm) {
    tone(buzzerPin, 2000, 300);
  } else {
    noTone(buzzerPin);
  }
}

// ---------------- LCD ----------------
void updateLCD(float T) {
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("T=");
  lcd.print(T,1);
  lcd.print((char)223);
  lcd.print("C");

  lcd.setCursor(0,1);
  lcd.print("FAN:");
  if (fanPWM == 0) lcd.print("OFF ");
  else lcd.print((int)fanPercent), lcd.print("%");

  lcd.print(" SD:");
  lcd.print(sdOK ? "OK" : "NO");
}

// ---------------- SD LOG ----------------
void logToSD(float T) {
  if (!sdOK) return;

  File f = SD.open("log.csv", FILE_WRITE);
  if (f) {
    f.print(millis()/1000);
    f.print(",");
    f.print(T,2);
    f.print(",");
    f.println(fanPercent,1);
    f.close();
  }
}
