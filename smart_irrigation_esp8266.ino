/*
  SMART IRRIGATION SYSTEM - NodeMCU ESP8266
  Sensors : PIR (intruder), Raindrop (digital), Soil Moisture (analog)
  Actuator: Relay -> Water Pump, Buzzer -> Intruder Alert
  Web App : Served directly from ESP8266 (HTML/CSS/JS embedded below)

  WIRING
  --------------------------------------------------
  PIR OUT        -> D1  (GPIO5)
  Raindrop D0    -> D2  (GPIO4)   (digital output of rain module)
  Soil Moisture  -> A0             (analog, 0-1023)
  Relay IN       -> D5  (GPIO14)  -> controls pump
  Buzzer +       -> D6  (GPIO12)
  All GND common with ESP8266 GND
  --------------------------------------------------
  NOTE: Calibrate soilDryThreshold for your sensor.
        Most rain modules pull output LOW when wet -> adjust if inverted.
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ---------------- WiFi Credentials ----------------
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ---------------- Pin Definitions ----------------
#define PIR_PIN      D1   // GPIO5
#define RAIN_PIN     D2   // GPIO4
#define SOIL_PIN     A0
#define PUMP_RELAY   D5   // GPIO14
#define BUZZER_PIN   D6   // GPIO12

// ---------------- Config ----------------
int  soilDryThreshold      = 600;   // > this value = DRY (calibrate!)
bool rainActiveLow         = true;  // module pulls LOW when wet, set false if inverted
const unsigned long motionAlertMs = 5000;

ESP8266WebServer server(80);

bool motionDetected = false;
bool rainDetected   = false;
int  soilValue      = 0;
bool pumpState       = false;
bool autoMode        = true;
unsigned long lastMotionTime = 0;

// ================= WEB PAGE (PROGMEM) =================
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Smart Irrigation Dashboard</title>
<style>
  * { box-sizing:border-box; margin:0; padding:0; font-family:'Segoe UI',Arial,sans-serif; }
  body { background:linear-gradient(135deg,#0f2027,#203a43,#2c5364); min-height:100vh; color:#eaf6f6; padding:20px; }
  h1 { text-align:center; margin-bottom:25px; font-size:1.8rem; letter-spacing:1px; }
  .grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(220px,1fr)); gap:18px; max-width:1000px; margin:0 auto; }
  .card { background:rgba(255,255,255,0.07); backdrop-filter:blur(6px); border:1px solid rgba(255,255,255,0.15);
          border-radius:16px; padding:20px; text-align:center; transition:.3s; }
  .card:hover { transform:translateY(-4px); }
  .card h2 { font-size:0.95rem; opacity:0.8; margin-bottom:10px; text-transform:uppercase; letter-spacing:1px; }
  .value { font-size:2rem; font-weight:bold; }
  .status-ok   { color:#4ade80; }
  .status-warn { color:#facc15; }
  .status-bad  { color:#f87171; }
  .bar-bg { background:rgba(255,255,255,0.15); border-radius:10px; height:14px; margin-top:10px; overflow:hidden; }
  .bar-fill { height:100%; background:linear-gradient(90deg,#38bdf8,#22d3ee); transition:width .4s; }
  .btn-row { display:flex; gap:10px; justify-content:center; margin-top:15px; flex-wrap:wrap; }
  button { border:none; padding:10px 18px; border-radius:10px; font-weight:600; cursor:pointer; transition:.2s; }
  button:active { transform:scale(0.95); }
  .btn-on   { background:#22c55e; color:#04270f; }
  .btn-off  { background:#ef4444; color:#2b0505; }
  .btn-auto { background:#38bdf8; color:#04202b; }
  .pump-status { margin-top:12px; font-size:1.1rem; font-weight:bold; }
  .alert-banner { display:none; background:#f87171; color:#3a0000; font-weight:bold; padding:12px;
                  border-radius:12px; text-align:center; max-width:1000px; margin:0 auto 18px auto; }
  .footer { text-align:center; margin-top:25px; opacity:0.5; font-size:0.8rem; }
</style>
</head>
<body>
  <h1>🌱 Smart Irrigation Dashboard</h1>
  <div id="motionBanner" class="alert-banner">🚨 Motion Detected - Possible Intruder!</div>

  <div class="grid">
    <div class="card">
      <h2>Soil Moisture</h2>
      <div class="value" id="soilVal">--</div>
      <div id="soilStatus" class="status-ok">--</div>
      <div class="bar-bg"><div class="bar-fill" id="soilBar" style="width:0%"></div></div>
    </div>

    <div class="card">
      <h2>Rain Status</h2>
      <div class="value" id="rainVal">--</div>
    </div>

    <div class="card">
      <h2>Motion Sensor</h2>
      <div class="value" id="motionVal">--</div>
    </div>

    <div class="card">
      <h2>Water Pump</h2>
      <div class="pump-status" id="pumpVal">--</div>
      <div class="btn-row">
        <button class="btn-on"   onclick="cmd('/pump/on')">ON</button>
        <button class="btn-off"  onclick="cmd('/pump/off')">OFF</button>
        <button class="btn-auto" onclick="cmd('/auto/on')">AUTO</button>
      </div>
      <div style="margin-top:8px; font-size:0.85rem; opacity:0.7;" id="modeVal">Mode: --</div>
    </div>
  </div>

  <div class="footer">ESP8266 Smart Irrigation • auto refresh 2s</div>

<script>
async function cmd(path){
  await fetch(path);
  fetchData();
}

async function fetchData(){
  try {
    const res = await fetch('/data');
    const d = await res.json();

    document.getElementById('soilVal').innerText = d.soil;
    document.getElementById('soilBar').style.width = Math.min(100,(d.soil/1023*100)).toFixed(0) + '%';
    const soilStatusEl = document.getElementById('soilStatus');
    soilStatusEl.innerText = d.soilStatus;
    soilStatusEl.className = d.soilStatus === 'Dry' ? 'status-bad' : 'status-ok';

    const rainEl = document.getElementById('rainVal');
    rainEl.innerText = d.rain ? '🌧️ Raining' : '☀️ No Rain';
    rainEl.className = 'value ' + (d.rain ? 'status-warn' : 'status-ok');

    const motionEl = document.getElementById('motionVal');
    motionEl.innerText = d.motion ? '⚠️ Detected' : '✅ Clear';
    motionEl.className = 'value ' + (d.motion ? 'status-bad' : 'status-ok');
    document.getElementById('motionBanner').style.display = d.motion ? 'block' : 'none';

    const pumpEl = document.getElementById('pumpVal');
    pumpEl.innerText = d.pump ? '💧 Pump ON' : '⛔ Pump OFF';
    pumpEl.className = 'pump-status ' + (d.pump ? 'status-ok' : 'status-bad');

    document.getElementById('modeVal').innerText = 'Mode: ' + (d.auto ? 'AUTO' : 'MANUAL');
  } catch(e) {
    console.log('fetch error', e);
  }
}

setInterval(fetchData, 2000);
fetchData();
</script>
</body>
</html>
)=====";

// ================= HANDLERS =================
void handleRoot() {
  server.send_P(200, "text/html", MAIN_page);
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

// ================= SETUP / LOOP =================
void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(RAIN_PIN, INPUT);
  pinMode(PUMP_RELAY, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/pump/on", handlePumpOn);
  server.on("/pump/off", handlePumpOff);
  server.on("/auto/on", handleAutoOn);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  // ---- PIR Motion ----
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
  soilValue = analogRead(SOIL_PIN); // 0-1023

  // ---- Auto Irrigation Logic ----
  if (autoMode) {
    if (rainDetected) {
      pumpState = false;                    // skip watering if raining
    } else if (soilValue > soilDryThreshold) {
      pumpState = true;                     // soil dry -> water
    } else {
      pumpState = false;                    // soil wet -> stop
    }
    digitalWrite(PUMP_RELAY, pumpState ? HIGH : LOW);
  }

  delay(200);
}
