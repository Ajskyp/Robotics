/*
  SMART IRRIGATION SYSTEM - NodeMCU ESP8266
  Complete working sketch for the HTML dashboard
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ===== 🔧 CHANGE THESE =====
const char* ssid     = "mechanical Dept";  // Your WiFi name
const char* password = "2026@mechanical";  // Your WiFi password
// ===========================

// ===== Pin Definitions =====
#define PIR_PIN      D1   // GPIO5 - Motion Sensor
#define RAIN_PIN     D2   // GPIO4 - Rain Sensor (Digital)
#define SOIL_PIN     A0   // Analog - Soil Moisture
#define PUMP_RELAY   D5   // GPIO14 - Water Pump Relay
#define BUZZER_PIN   D6   // GPIO12 - Buzzer

// ===== Configuration =====
int soilDryThreshold = 600;  // Calibrate this value
bool rainActiveLow = true;   // Most rain modules pull LOW when wet
const unsigned long motionAlertMs = 5000;

ESP8266WebServer server(80);

bool motionDetected = false;
bool rainDetected = false;
int soilValue = 0;
bool pumpState = false;
bool autoMode = true;
unsigned long lastMotionTime = 0;

// ============================================================
//  HTML PAGE (Same as your Smart_irrigation.html)
//  Note: You already have this file, so we'll just serve it
// ============================================================

// ===== HANDLERS =====
void handleRoot() {
  // Serve the HTML file (your Smart_irrigation.html content)
  server.send(200, "text/html", "<!DOCTYPE html><html><head><title>Smart Irrigation</title></head><body><h1>Loading...</h1><script>window.location.href='/dashboard.html';</script></body></html>");
}

void handleData() {
  String json = "{";
  json += "\"motion\":" + String(motionDetected ? "true" : "false") + ",";
  json += "\"rain\":" + String(rainDetected ? "true" : "false") + ",";
  json += "\"soil\":" + String(soilValue) + ",";
  json += "\"soilStatus\":\"" + String(soilValue > soilDryThreshold ? "Dry" : "Wet") + "\",";
  json += "\"pump\":" + String(pumpState ? "true" : "false") + ",";
  json += "\"auto\":" + String(autoMode ? "true" : "false");
  json += "}";
  
  // Add CORS headers
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200, "application/json", json);
}

void handlePumpOn() {
  autoMode = false;
  pumpState = true;
  digitalWrite(PUMP_RELAY, HIGH);
  server.send(200, "text/plain", "OK");
}

void handlePumpOff() {
  autoMode = false;
  pumpState = false;
  digitalWrite(PUMP_RELAY, LOW);
  server.send(200, "text/plain", "OK");
}

void handleAutoOn() {
  autoMode = true;
  server.send(200, "text/plain", "OK");
}

void handleOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200);
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("  🌱 SMART IRRIGATION SYSTEM");
  Serial.println("========================================");

  // Setup pins
  pinMode(PIR_PIN, INPUT);
  pinMode(RAIN_PIN, INPUT);
  pinMode(PUMP_RELAY, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  digitalWrite(PUMP_RELAY, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.print("📡 Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✅ WiFi Connected!");
    Serial.print("📡 IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("⚠️ WiFi connection failed!");
    Serial.println("📡 Starting Access Point mode...");
    
    WiFi.softAP("SmartIrrigation", "12345678");
    Serial.print("📡 AP IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("📱 Connect to 'SmartIrrigation' WiFi");
    Serial.println("📱 Password: 12345678");
    Serial.println("📱 Then use IP: 192.168.4.1");
  }

  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.on("/pump/on", HTTP_GET, handlePumpOn);
  server.on("/pump/off", HTTP_GET, handlePumpOff);
  server.on("/auto/on", HTTP_GET, handleAutoOn);
  server.on("/dashboard.html", HTTP_GET, []() {
    // Serve your HTML file
    server.send(200, "text/html", R"=====(
      <!-- PASTE YOUR ENTIRE Smart_irrigation.html CONTENT HERE -->
      <!-- Or serve it from SPIFFS -->
    )=====");
  });
  
  // Handle CORS
  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
      handleOptions();
    } else {
      server.send(404, "text/plain", "Not Found");
    }
  });

  server.begin();
  
  Serial.println("🌐 HTTP Server Started!");
  Serial.println("========================================");
  Serial.println();
  Serial.println("📱 OPEN THIS IN YOUR BROWSER:");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("   http://");
    Serial.println(WiFi.localIP());
    Serial.println();
    Serial.println("📱 OR USE THE DASHBOARD:");
    Serial.print("   http://");
    Serial.print(WiFi.localIP());
    Serial.println("/dashboard.html");
  } else {
    Serial.println("   http://192.168.4.1");
  }
  Serial.println("========================================");
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  server.handleClient();

  // ---- PIR Motion Sensor ----
  if (digitalRead(PIR_PIN) == HIGH) {
    motionDetected = true;
    lastMotionTime = millis();
    digitalWrite(BUZZER_PIN, HIGH);
  }
  if (motionDetected && millis() - lastMotionTime > motionAlertMs) {
    motionDetected = false;
    digitalWrite(BUZZER_PIN, LOW);
  }

  // ---- Rain Sensor ----
  int rainRaw = digitalRead(RAIN_PIN);
  rainDetected = rainActiveLow ? (rainRaw == LOW) : (rainRaw == HIGH);

  // ---- Soil Moisture ----
  soilValue = analogRead(SOIL_PIN);
  
  // ---- Auto Irrigation Logic ----
  if (autoMode) {
    if (rainDetected) {
      pumpState = false;  // Don't water when raining
    } else if (soilValue > soilDryThreshold) {
      pumpState = true;   // Soil dry → water
    } else {
      pumpState = false;  // Soil wet → stop
    }
    digitalWrite(PUMP_RELAY, pumpState ? HIGH : LOW);
  }

  delay(200);
}