#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "MAX30105.h"
#include "heartRate.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MAX30105 particleSensor;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;

float SpO2 = 0.0;

void setup() {
  Serial.begin(9600);  // Start the serial communication
  delay(250);
  display.begin(0x3C, true); // Address 0x3C default
  display.display();
  delay(2000);
  display.clearDisplay();

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check the connections.");
    while (1);  // Halt execution if the sensor is not found
  } else {
    Serial.println("MAX30105 initialized successfully.");
  }

  particleSensor.setup(); 
  particleSensor.setPulseAmplitudeRed(0x0A); // Low amplitude for Red LED
  particleSensor.setPulseAmplitudeIR(0x0F);  // Higher amplitude for IR LED
  Serial.println("Sensor setup complete. Waiting for finger placement...");
}

void loop() {
  long irValue = particleSensor.getIR();  // Reading IR value
  long redValue = particleSensor.getRed(); // Reading Red value
  
  // Debugging: Output the raw sensor values
  Serial.print("IR Value: "); Serial.println(irValue);
  Serial.print("Red Value: "); Serial.println(redValue);

  // Clear display and set text size for larger text
  display.clearDisplay();
  display.setTextSize(2); // Larger text size
  display.setTextColor(SH110X_WHITE);

  // Display heart rate and SpO2
  display.setCursor(0, 0); // Left align Heart Rate at the top
  display.print("Heart Rate:");
  display.println(beatAvg);
  display.setCursor(0, 30); // Left align SpO2 at the bottom
  display.print("SpO2:");
  display.print(SpO2, 1);
  display.println("%");
  display.display();

  // Check if a finger is detected
  if (irValue > 4000) {
    Serial.println("Finger detected.");

    if (checkForBeat(irValue)) {
      Serial.println("Beat detected!");

      long delta = millis() - lastBeat;
      lastBeat = millis();

      // Debugging: Output the time difference between beats
      Serial.print("Time since last beat: "); Serial.println(delta);

      beatsPerMinute = 60 / (delta / 1000.0);
      Serial.print("Calculated BPM: "); Serial.println(beatsPerMinute);

      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) {
          beatAvg += rates[x];
        }
        beatAvg /= RATE_SIZE;

        // Debugging: Output the average BPM
        Serial.print("Average BPM: "); Serial.println(beatAvg);
      } else {
        Serial.println("BPM out of range, not recorded.");
      }

      // Sound buzzer on beat detection
      tone(3, 1000);
      delay(100);
      noTone(3);
    } else {
      Serial.println("No beat detected.");
    }

    // Calculate SpO2
    float ratio = (float)redValue / (float)irValue;
    SpO2 = 110.0 - 25.0 * ratio;
    if (SpO2 > 100) SpO2 = 100; // Clamp SpO2 at 100%
    else if (SpO2 < 0) SpO2 = 0; // Clamp SpO2 at 0%

    // Debugging: Output the calculated SpO2
    Serial.print("Calculated SpO2: "); Serial.println(SpO2);

    delay(1000);
  } else {
    Serial.println("No finger detected or low IR value.");
    beatAvg = 0;
    SpO2 = 0.0;

    display.clearDisplay();
    display.setTextSize(1); // Slightly larger text for instructions
    display.setCursor((SCREEN_WIDTH - 12 * 6) / 2, 15); // Center "Please place"
    display.println("Please place");
    display.setCursor((SCREEN_WIDTH - 11 * 6) / 2, 35); // Center "your finger"
    display.println("your finger");
    display.display();
    delay(1000);
  }
}
