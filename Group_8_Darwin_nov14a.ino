#include "arduino_secrets.h"
/*
  Combined Sketch:
  - Arduino MKR IoT Carrier
  - Pulse Sensor (A5)
  - Accelerometer (LSM6DS3)
  - LED Ring G-Force Visualization (color-coded)
  - Buzzer beeps on system state change
*/

#include "thingProperties.h"
#include <Arduino_MKRIoTCarrier.h>
#include <PulseSensorPlayground.h>
#include <Arduino_LSM6DS3.h>   // Accelerometer library

const int PulseSensorPurplePin = A5;
const int LED = LED_BUILTIN;
int Threshold = 580;

int Signal;

MKRIoTCarrier carrier;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  // Initialize Arduino IoT Cloud
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  // Initialize Carrier
  carrier.begin();

  // Startup screen
  carrier.display.fillScreen(ST77XX_BLACK);
  carrier.display.setTextColor(ST77XX_YELLOW);
  carrier.display.setTextSize(2);
  carrier.display.setCursor(20, 60);
  carrier.display.print("");
  carrier.display.setCursor(20, 100);
  carrier.display.print("");

  // Initialize IMU
  if (!IMU.begin()) {
    carrier.display.setTextColor(ST77XX_RED);
    carrier.display.setCursor(20, 160);
    carrier.display.print("IMU FAIL");
    while (1);
  }

  // Wait for cloud connection
  while (!ArduinoCloud.connected()) {
    ArduinoCloud.update();
    delay(500);
  }

  delay(1500);

  // Default system ON
  systemOn = true;
  ArduinoCloud.update();
  onSystemOnChange();
}

void loop() {
  ArduinoCloud.update();

  if (!systemOn)
    return;

  // ---------------- Pulse Sensor ----------------
  Signal = analogRead(PulseSensorPurplePin);
  Serial.println("Signal: " + String(Signal));

  bpm = Signal;
  ArduinoCloud.update();

  if (Signal > Threshold) {
    digitalWrite(LED, HIGH);
    carrier.display.fillCircle(120, 120, 20, ST77XX_RED);
  } else {
    digitalWrite(LED, LOW);
    carrier.display.fillCircle(120, 120, 20, ST77XX_BLACK);
  }

  // ---------------- Accelerometer ----------------
  float x, y, z;

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);

    float magnitude = sqrt(x*x + y*y + z*z) - 1.0;
    if (magnitude < 0) {
      magnitude = 0;
      
    }
    if (magnitude > 3) {
      magnitude = 3;
      carrier.leds.setPixelColor(4, 255, 0, 0);
    }

    float gForceValue = magnitude * 100.0;
    cloudGForce = gForceValue;
    ArduinoCloud.update();

    Serial.println("gForce: " + String(gForceValue));

    // Display g-force on screen
    carrier.display.fillRect(0, 180, 240, 40, ST77XX_BLACK);
    carrier.display.setCursor(20, 190);
    carrier.display.setTextColor(ST77XX_CYAN);
    carrier.display.print("gForce: ");
    carrier.display.print(gForceValue, 1);
    carrier.leds.show();
  }
  delay(20);
}

void onSystemOnChange() {
  // Clear status area
  carrier.display.fillRect(20, 140, 200, 20, ST77XX_BLACK);
  carrier.display.setCursor(20, 140);

  if (systemOn) {
    carrier.display.setTextColor(ST77XX_GREEN);
    carrier.display.print("SYSTEM ON");

    // Single beep
    carrier.Buzzer.sound(1000);
    delay(100);
    carrier.Buzzer.noSound();

    // Remove text after 2 seconds
    delay(2000);
    carrier.display.fillRect(20, 140, 200, 20, ST77XX_BLACK);

  } else {
    carrier.display.setTextColor(ST77XX_RED);
    carrier.display.print("SYSTEM OFF");

    // Two beeps
    for (int i = 0; i < 2; i++) {
      carrier.Buzzer.sound(1000);
      delay(100);
      carrier.Buzzer.noSound();
      delay(100);
    }
  }
}
