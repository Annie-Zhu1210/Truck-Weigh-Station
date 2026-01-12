#include "HX711.h"

#define DOUT 4
#define CLK  5

HX711 scale;

void setup() {
  Serial.begin(9600);
  Serial.println("HX711 Test");

  scale.begin(DOUT, CLK);
}

void loop() {
  if (scale.is_ready()) {
    long reading = scale.read();
    long corrected = -reading;  // flip the sign

    Serial.print("Raw reading: ");
    Serial.print(reading);
    Serial.print(" | Corrected: ");
    Serial.println(corrected);
  } else {
    Serial.println("HX711 not found.");
  }

  delay(200);
}
