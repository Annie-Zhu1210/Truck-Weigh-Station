#include <HX711_ADC.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

const int HX711_dout = 4;
const int HX711_sck = 5;
LiquidCrystal lcd(12, 11, A5, A4, A3, A2);
const int LED_PIN = 7;
const int SERVO_PIN = 9;
Servo myServo;

// Customise your weight thereshold and timing here.
const float WEIGHT_THRESHOLD = 300.0;  // Define 300g as the threshold
const unsigned long DELAY_TIME = 3000;  // 3 seconds

bool isAboveThreshold = false;
bool isServoAt0 = false;
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

  // Initialise LED pin with off status
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialise the servo motor at 90 degrees (free to go)
  myServo.attach(SERVO_PIN);
  myServo.write(90);

  // Initialise the LCD screen
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  LoadCell.begin();
  
  // IMPORTANT:
  // Replace this number by your own calibration value
  float calibrationValue = 1.0;
  // Uncomment this line when you have known your calibration value
  // EEPROM.get(calVal_eepromAdress, calibrationValue);
  
  // Restore tare offset from EEPROM
  long tare_offset = 0;
  EEPROM.get(tareOffsetVal_eepromAdress, tare_offset);
  LoadCell.setTareOffset(tare_offset);
  
  boolean _tare = false;
  
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

  if (LoadCell.update()) newDataReady = true;

  // Get smoothed value from dataset
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float weight = LoadCell.getData();
      
      Serial.print("Weight: ");
      Serial.print(weight, 1);
      Serial.println(" g");

      // Control LED based on the weight threshold (the STREET LIGHT/ INDICATOR LIGHT)
      if (weight > WEIGHT_THRESHOLD) {
        digitalWrite(LED_PIN, HIGH);
      } else {
        digitalWrite(LED_PIN, LOW);
      }

      // Servo motor control (the RISING ARM BARRIER)
      if (weight > WEIGHT_THRESHOLD) {
        // When weight is above the threshold, the rising arm will lower
        if (!isAboveThreshold) {
          isAboveThreshold = true;
          thresholdStartTime = millis();
          Serial.println("Weight > 300g - Starting close timer");
        } else {
          // If the weight has already been above the threshold for 3 seconds, move the servo motor to 0 degree
          if (!isServoAt0 && (millis() - thresholdStartTime >= DELAY_TIME)) {
            myServo.write(0);
            isServoAt0 = true;
            Serial.println("Servo moved to 0 degrees (CLOSED)");
          }
        }
        belowThresholdStartTime = 0;
      } else {
        // When the weight is below or equal to threshold, the rising arm barrier should lift up (move to 90 degrees)
        if (isAboveThreshold) {
          isAboveThreshold = false;
          if (belowThresholdStartTime == 0) {
            belowThresholdStartTime = millis();
            Serial.println("Weight <= 300g - Starting open timer");
          }
        }
        
        // Check if the servo needs to lift up (return to 90 degrees)
        if (isServoAt0 && belowThresholdStartTime > 0 && 
            (millis() - belowThresholdStartTime >= DELAY_TIME)) {
          // if it's below the threshold for 3 seconds 
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
      
      // Show access messages on the screen
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
  // 'r' = start the calibration process
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