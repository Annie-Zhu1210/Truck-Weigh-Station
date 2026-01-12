#include <HX711_ADC.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

// Load cell pins
const int HX711_dout = 4;
const int HX711_sck = 5;

// LCD pins for RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, A5, A4, A3, A2);

// LED pin
const int LED_PIN = 7;

// Servo pin
const int SERVO_PIN = 9;
Servo myServo;

// Threshold and timing
const float WEIGHT_THRESHOLD = 300.0;  // 300g as the threshold
const unsigned long DELAY_TIME = 3000;  // 3 seconds

// State tracking variables
bool isAboveThreshold = false;
bool isServoAt0 = false;  // Changed: tracking if servo is at 0 degrees (closed)
unsigned long thresholdStartTime = 0;
unsigned long belowThresholdStartTime = 0;

// HX711 constructor
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
const int tareOffsetVal_eepromAdress = 4;
unsigned long t = 0;

void setup() {
  Serial.begin(57600); 
  delay(10);
  Serial.println();
  Serial.println("Starting...");

  // Initialise LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Start with LED off

  // Initialise servo
  myServo.attach(SERVO_PIN);
  myServo.write(90);  // Changed: Start at 90 degrees (open position)

  // Initialise LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  LoadCell.begin();
  
  // Load calibration value from EEPROM
  float calibrationValue = -1319.04;
  EEPROM.get(calVal_eepromAdress, calibrationValue);
  
  // Restore tare offset from EEPROM (remembers zero point)
  long tare_offset = 0;
  EEPROM.get(tareOffsetVal_eepromAdress, tare_offset);
  LoadCell.setTareOffset(tare_offset);
  
  boolean _tare = false; // false because we loaded from EEPROM
  
  // Stabilising time for better accuracy
  unsigned long stabilizingtime = 2000;
  LoadCell.start(stabilizingtime, _tare);
  
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check wiring");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HX711 Error!");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue);
    Serial.println("Startup complete");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ready!");
    delay(1000);
    lcd.clear();
  }
}

void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 200;

  // Check for new data (non-blocking)
  if (LoadCell.update()) newDataReady = true;

  // Get smoothed value from dataset
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float weight = LoadCell.getData();
      
      Serial.print("Weight: ");
      Serial.print(weight, 1);
      Serial.println(" g");

      // Control LED based on weight threshold
      if (weight > WEIGHT_THRESHOLD) {
        digitalWrite(LED_PIN, HIGH);  // Turn LED on
      } else {
        digitalWrite(LED_PIN, LOW);   // Turn LED off
      }

      // Servo control logic - REVERSED
      if (weight > WEIGHT_THRESHOLD) {
        // Weight is above threshold - should CLOSE (move to 0 degrees)
        if (!isAboveThreshold) {
          // Just crossed above threshold - start timer
          isAboveThreshold = true;
          thresholdStartTime = millis();
          Serial.println("Weight > 300g - Starting close timer");
        } else {
          // Already above threshold - check if 3 seconds have passed
          if (!isServoAt0 && (millis() - thresholdStartTime >= DELAY_TIME)) {
            // 3 seconds have passed, move servo to 0 degrees (CLOSED)
            myServo.write(0);
            isServoAt0 = true;
            Serial.println("Servo moved to 0 degrees (CLOSED)");
          }
        }
        // Reset the below-threshold timer
        belowThresholdStartTime = 0;
      } else {
        // Weight is below or equal to threshold - should OPEN (move to 90 degrees)
        if (isAboveThreshold) {
          // Just crossed below threshold - start timer
          isAboveThreshold = false;
          if (belowThresholdStartTime == 0) {
            belowThresholdStartTime = millis();
            Serial.println("Weight <= 300g - Starting open timer");
          }
        }
        
        // Check if we should return servo to 90 degrees
        if (isServoAt0 && belowThresholdStartTime > 0 && 
            (millis() - belowThresholdStartTime >= DELAY_TIME)) {
          // 3 seconds below threshold, move servo back to 90 degrees (OPEN)
          myServo.write(90);
          isServoAt0 = false;
          belowThresholdStartTime = 0;
          Serial.println("Servo returned to 90 degrees (OPEN)");
        }
        
        // Reset the above-threshold timer
        thresholdStartTime = 0;
      }

      // Display on LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(weight, 1);
      lcd.print(" g");
      
      // Show message based on weight - Changed from servo degrees
      lcd.setCursor(0, 1);
      if (weight > WEIGHT_THRESHOLD) {
        lcd.print("Please stop");
      } else {
        lcd.print("Please go through");
      }
      
      newDataReady = 0;
      t = millis();
    }
  }

  // Serial commands:
  // 't' = tare (set current weight as zero)
  // 'r' = start calibration process
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') {
      refreshOffsetValueAndSaveToEEprom();
    }
    else if (inByte == 'r') {
      calibrate();
    }
  }
}

// Tare and save to EEPROM
void refreshOffsetValueAndSaveToEEprom() {
  long _offset = 0;
  Serial.println("Taring...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Taring...");
  
  LoadCell.tare();
  _offset = LoadCell.getTareOffset();
  
  EEPROM.put(tareOffsetVal_eepromAdress, _offset);
  #if defined(ESP8266) || defined(ESP32)
  EEPROM.commit();
  #endif
  
  LoadCell.setTareOffset(_offset);
  Serial.print("New offset: ");
  Serial.println(_offset);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tare complete!");
  delay(1500);
  lcd.clear();
}

// Calibration function
void calibrate() {
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Remove all weight.");
  Serial.println("Send 't' to tare.");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibration");
  lcd.setCursor(0, 1);
  lcd.print("Remove weight");

  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 't') LoadCell.tareNoDelay();
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
      _resume = true;
    }
  }

  Serial.println("Place known mass.");
  Serial.println("Send weight (e.g. 100.0):");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place weight");
  lcd.setCursor(0, 1);
  lcd.print("Enter via Serial");

  float known_mass = 0;
  _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print("Known mass: ");
        Serial.println(known_mass);
        _resume = true;
      }
    }
  }

  LoadCell.refreshDataSet();
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass);

  Serial.print("Calibration value: ");
  Serial.println(newCalibrationValue);
  Serial.print("Save to EEPROM? (y/n): ");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cal: ");
  lcd.print(newCalibrationValue, 1);

  _resume = false;
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
        #if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
        #endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
        #if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
        #endif
        Serial.println("Saved to EEPROM!");
        lcd.setCursor(0, 1);
        lcd.print("Saved!");
        _resume = true;
      }
      else if (inByte == 'n') {
        Serial.println("Not saved");
        lcd.setCursor(0, 1);
        lcd.print("Not saved");
        _resume = true;
      }
    }
  }
  
  delay(2000);
  lcd.clear();
  Serial.println("Calibration complete!");
  Serial.println("***");
}