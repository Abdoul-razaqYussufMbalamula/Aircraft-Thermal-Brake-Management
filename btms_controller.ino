#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <max6675.h>

// -------------------- MAX6675 PINS (Software SPI isolation) --------------------
const int thermoCLK = 6;   // SoftSPI: SCK
const int thermoCS  = 7;   // CS
const int thermoDO  = 8;   // SO (DO)
MAX6675 thermo(thermoCLK, thermoCS, thermoDO);

// -------------------- FAN (IRF520 MOSFET) --------------------
const int fanPin = 5;      // PWM pin
int fanPWM = 0;
float fanPercent = 0.0;

// -------------------- SD CARD MODULE --------------------
const int chipSelect = 4;
bool sdOK = false;

// -------------------- 16x2 LCD DISPLAY --------------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// -------------------- ACTIVE BUZZER --------------------
const int buzzerPin = 9;

// -------------------- CONTROL THRESHOLDS & PARAMETERS --------------------
unsigned long lastReadTime = 0;
const unsigned long readInterval = 1000; // 1Hz Sampling Rate (1 second)

const float T_min = 35.0;   // Idle: Fan OFF
const float T_max = 60.0;   // Max Cooling: Fan 100%
const float T_alarm = 65.0; // Critical condition: Buzzer activated

// ------------------------------------------------------------
void setup() {
  Serial.begin(9600);

  // Initialize Fan (Default OFF)
  pinMode(fanPin, OUTPUT);
  analogWrite(fanPin, 0);

  // Initialize Buzzer (Default OFF)
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  // Initialize LCD Boot Sequence
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("AKILLI TERMAL");
  lcd.setCursor(0,1);
  lcd.print("FREN YONETIMI");

  delay(2000);   // Allow components to stabilize
  lcd.clear();

  // Initialize SD Card Data Logger (Attempts 3 connections)
  lcd.print("SD kontrol...");
  for (int i = 0; i < 3; i++) {
    if (SD.begin(chipSelect)) {
      sdOK = true;
      break;
    }
    delay(300);
  }

  // Generate CSV Headers if SD connection successful
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

  // 1Hz Polling Routine (Non-Blocking)
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

// ---------------- FAN SPEED MAPPING ----------------
void updateFanPWM(float T) {
  if (T <= T_min) {
    fanPWM = 0; // Zone 1
  } 
  else if (T >= T_max) {
    fanPWM = 255; // Zone 3
  } 
  else {
    // Zone 2: Proportional mapping. 
    // Minimum PWM is 80 to overcome motor stall torque at low voltages.
    fanPWM = map((int)(T * 10),
                 (int)(T_min * 10),
                 (int)(T_max * 10),
                 80, 255);
  }

  analogWrite(fanPin, fanPWM);
  fanPercent = (fanPWM / 255.0) * 100.0;
}

// ---------------- BUZZER ALARM ----------------
void updateBuzzer(float T) {
  if (T >= T_alarm) {
    tone(buzzerPin, 2000, 300); // 2kHz warning tone
  } else {
    noTone(buzzerPin);
  }
}

// ---------------- UI UPDATE (LCD) ----------------
void updateLCD(float T) {
  lcd.clear();

  // Display Temperature
  lcd.setCursor(0,0);
  lcd.print("T=");
  lcd.print(T,1);
  lcd.print((char)223); // Degree symbol
  lcd.print("C");

  // Display Fan Percentage & SD Status
  lcd.setCursor(0,1);
  lcd.print("FAN:");
  if (fanPWM == 0) lcd.print("OFF ");
  else lcd.print((int)fanPercent), lcd.print("%");

  lcd.print(" SD:");
  lcd.print(sdOK ? "OK" : "NO");
}

// ---------------- TELEMETRY LOGGING (SD) ----------------
void logToSD(float T) {
  if (!sdOK) return;

  File f = SD.open("log.csv", FILE_WRITE);
  if (f) {
    f.print(millis()/1000); // Record timestamp in seconds
    f.print(",");
    f.print(T,2);
    f.print(",");
    f.println(fanPercent,1);
    f.close();
  }
}
