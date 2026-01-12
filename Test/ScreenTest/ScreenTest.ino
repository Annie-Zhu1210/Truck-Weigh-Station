/*
 * Arduino UNO + HC-SR04 Ultrasonic Sensor + 16x2 LCD (Direct Connection)
 * 
 * This code reads distance from HC-SR04 ultrasonic sensor
 * and displays it on a 16x2 LCD with direct pin connections
 * 
 * Connections:
 * LCD:      VSS->GND, VDD->5V, V0->Potentiometer, RS->D12, RW->GND, E->D11
 *           D4->A5, D5->A4, D6->A3, D7->A2, A->5V(+220Ω), K->GND
 * HC-SR04:  VCC->5V, TRIG->D9, ECHO->D10, GND->GND
 */

#include <LiquidCrystal.h>

// LCD pins: RS, E, D4, D5, D6, D7
// LCD D4->A5, D5->A4, D6->A3, D7->A2
LiquidCrystal lcd(12, 11, A5, A4, A3, A2);

// HC-SR04 pins
const int trigPin = 9;
const int echoPin = 10;

// Variables for distance calculation
long duration;
float distanceCm;
float distanceInch;

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(9600);
  
  // Initialize LCD (16 columns, 2 rows)
  lcd.begin(16, 2);
  
  // Display welcome message
  lcd.setCursor(0, 0);
  lcd.print("Distance Sensor");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
  
  // Setup HC-SR04 pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  Serial.println("System Ready!");
}

void loop() {
  // Clear the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Trigger the sensor by setting trigPin HIGH for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate distance in cm and inches
  // Speed of sound = 343 m/s = 0.0343 cm/microsecond
  // Distance = (Time × Speed) / 2  (divided by 2 for round trip)
  distanceCm = duration * 0.0343 / 2;
  distanceInch = distanceCm / 2.54;
  
  // Display on LCD
  lcd.setCursor(0, 0);
  lcd.print("Distance:       "); // Clear line with spaces
  
  lcd.setCursor(0, 1);
  
  // Check if reading is valid (HC-SR04 range: 2cm to 400cm)
  if (distanceCm < 2 || distanceCm > 400) {
    lcd.print("Out of Range    ");
    Serial.println("Distance: Out of Range");
  } else {
    // Display distance in cm
    lcd.print(distanceCm, 1);  // 1 decimal place
    lcd.print(" cm    ");
    
    // Optionally display in inches on same line
    // lcd.setCursor(0, 1);
    // lcd.print(distanceInch, 1);
    // lcd.print(" in    ");
    
    // Print to Serial Monitor for debugging
    Serial.print("Distance: ");
    Serial.print(distanceCm);
    Serial.print(" cm (");
    Serial.print(distanceInch);
    Serial.println(" in)");
  }
  
  // Update every 3000ms
  delay(1000);
}