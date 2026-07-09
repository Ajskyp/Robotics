#define BLYNK_TEMPLATE_ID "TMPL3cFU7O_Jm"
#define BLYNK_TEMPLATE_NAME "Home automation"
#define BLYNK_AUTH_TOKEN "AX0kvtGh5xDQMrDKLBwBy3hcp2PmVbo2"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// WiFi Credentials (replace with your actual WiFi)
char ssid[] = "YourWiFiSSID";
char pass[] = "YourWiFiPassword";

// Pin Definitions
#define SOIL_SENSOR A0
#define RAIN_SENSOR D7
#define PIR_SENSOR D1
#define RELAY_PUMP D0
#define BUZZER D5

int moistureThreshold = 30;
bool pumpManualOverride = false;

BLYNK_WRITE(V3) {
  pumpManualOverride = param.asInt();
  if (pumpManualOverride) {
    digitalWrite(RELAY_PUMP, LOW);   // Pump ON
  } else {
    digitalWrite(RELAY_PUMP, HIGH);  // Pump OFF
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(SOIL_SENSOR, INPUT);
  pinMode(RAIN_SENSOR, INPUT);
  pinMode(PIR_SENSOR, INPUT);
  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  digitalWrite(RELAY_PUMP, HIGH);  // Pump OFF initially
  digitalWrite(BUZZER, LOW);
  
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  delay(2000);
}

void loop() {
  Blynk.run();
  
  // Read soil moisture
  int moisture = analogRead(SOIL_SENSOR);
  int moisturePercent = map(moisture, 350, 850, 100, 0);
  moisturePercent = constrain(moisturePercent, 0, 100);
  Blynk.virtualWrite(V0, moisturePercent);
  
  // Check rain
  bool raining = (digitalRead(RAIN_SENSOR) == LOW);
  Blynk.virtualWrite(V1, raining ? "RAINING" : "DRY");
  
  // Auto-watering logic
  if (!pumpManualOverride && moisturePercent < moistureThreshold && !raining) {
    digitalWrite(RELAY_PUMP, LOW);   // Pump ON
    Blynk.logEvent("watering_started", "Soil moisture low, watering plants");
    delay(5000);  // Water for 5 seconds
    digitalWrite(RELAY_PUMP, HIGH);  // Pump OFF
  }
  
  // Motion detection with buzzer
  if (digitalRead(PIR_SENSOR) == HIGH) {
    digitalWrite(BUZZER, HIGH);
    Blynk.virtualWrite(V2, 1);
    
    // This triggers the notification you configured in Blynk Console
    Blynk.logEvent("motion_detected", "⚠️ Motion detected near your plants!");
    
    Serial.println("Motion detected! Notification sent.");
    delay(3000);
    digitalWrite(BUZZER, LOW);
    Blynk.virtualWrite(V2, 0);
  }
  
  delay(2000);
}