const int soilPin = 2;      // Soil moisture sensor digital pin
const int ledPin = 13;       // LED digital pin

void setup() {
  pinMode(soilPin, INPUT);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  int soilState = digitalRead(soilPin);

  if (soilState == HIGH) {
    // Dry soil -> blink LED
    digitalWrite(ledPin, HIGH);
    delay(300);
    digitalWrite(ledPin, LOW);
    delay(300);
  } else {
    // Wet soil -> LED off
    digitalWrite(ledPin, LOW);
  }
}