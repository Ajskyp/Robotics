const int vibrationPin = 2;   // DO pin
const int ledPin = 13;

void setup() {
  pinMode(vibrationPin, INPUT);
  pinMode(ledPin, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  int sensorState = digitalRead(vibrationPin);

  if (sensorState == HIGH) {
    digitalWrite(ledPin, HIGH);
    Serial.println("Vibration Detected!");
  } else {
    digitalWrite(ledPin, LOW);
  }

  delay(50);
}