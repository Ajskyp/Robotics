#include <Servo.h>

// Pin definitions
const int FLAME_SENSOR_DIGITAL = 2;
const int FLAME_SENSOR_ANALOG = A0;
const int SERVO_PIN = 9;

// Thresholds
const int ANALOG_THRESHOLD = 500;  // Adjust based on your sensor
// Digital: LOW = Flame, HIGH = No flame (for most sensors)

Servo myServo;

void setup() {
  Serial.begin(9600);
  pinMode(FLAME_SENSOR_DIGITAL, INPUT);
  myServo.attach(SERVO_PIN);
  myServo.write(0);
  
  Serial.println("=== Dual Flame Sensor System ===");
  Serial.println("Using both Digital & Analog inputs");
  Serial.println("==================================");
}

void loop() {
  // Read both sensors
  int digitalValue = digitalRead(FLAME_SENSOR_DIGITAL);
  int analogValue = analogRead(FLAME_SENSOR_ANALOG);
  
  // Detection logic using BOTH sensors for reliability
  bool digitalDetected = (digitalValue == LOW);  // Most sensors: LOW = flame
  bool analogDetected = (analogValue < ANALOG_THRESHOLD);
  
  // Print sensor values
  Serial.print("Digital: ");
  Serial.print(digitalValue == LOW ? "LOW 🔥" : "HIGH");
  Serial.print(" | Analog: ");
  Serial.print(analogValue);
  Serial.print(" | ");
  
  // Check flame detection
  if (digitalDetected || analogDetected) {
    Serial.println("FLAME DETECTED!");
    
    // Move servo 0° → 180° → 0°
    for (int angle = 0; angle <= 180; angle += 5) {
      myServo.write(angle);
      delay(30);
    }
    for (int angle = 180; angle >= 0; angle -= 5) {
      myServo.write(angle);
      delay(30);
    }
  } else {
    Serial.println("No flame");
    myServo.write(0);
  }
  
  delay(100);
}