const int ldrPin = A0;      // LDR analog pin
const int ledPin = 13;       // LED digital pin
const int threshold = 800;  // Adjust based on testing (0-1023)

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  int ldrValue = analogRead(ldrPin);
  Serial.println(ldrValue);  // for calibrating threshold

  if (ldrValue > threshold) {
    // Dark -> blink LED
    digitalWrite(ledPin, HIGH);
    delay(300);
    digitalWrite(ledPin, LOW);
    delay(300);
  } else {
    // Bright -> LED off
    digitalWrite(ledPin, LOW);
  }
}