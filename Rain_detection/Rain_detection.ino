#include <Servo.h>

// Pin definitions
#define RAIN_ANALOG_PIN A0    // Analog pin
#define RAIN_DIGITAL_PIN 2    // Digital pin (optional)
#define SERVO_PIN 9
#define RAIN_THRESHOLD 600

Servo myServo;
int rainValue = 0;
int digitalValue = 0;

void setup() {
  Serial.begin(9600);
  pinMode(RAIN_DIGITAL_PIN, INPUT);
  myServo.attach(SERVO_PIN);
  myServo.write(0);
  
  Serial.println("Advanced Rain Detection System");
  Serial.println("================================");
}

void loop() {
  // Read both analog and digital values
  rainValue = analogRead(RAIN_ANALOG_PIN);
  digitalValue = digitalRead(RAIN_DIGITAL_PIN);
  
  // Display readings
  Serial.print("Analog: ");
  Serial.print(rainValue);
  Serial.print(" | Digital: ");
  Serial.print(digitalValue);
  
  // Control logic
  if (rainValue < RAIN_THRESHOLD || digitalValue == LOW) {
    Serial.println(" | ⚠️ RAIN DETECTED!");
    performRainAction();
  } else {
    Serial.println(" | ✅ Clear Sky");
    // Return servo to default position
    myServo.write(0);
  }
  
  delay(300);
}

void performRainAction() {
  // Smooth servo movement
  for (int angle = 0; angle <= 180; angle += 3) {
    myServo.write(angle);
    delay(10);
  }
  
  // Hold position for 2 seconds
  delay(1000);
  
  // Return slowly
  for (int angle = 180; angle >= 0; angle -= 3) {
    myServo.write(angle);
    delay(10);
  }
}