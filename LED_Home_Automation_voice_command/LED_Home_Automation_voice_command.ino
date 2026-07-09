/*
  =====================================================
   Home Automation - Control 2 LEDs via NodeMCU ESP8266
   Mobile Web App built-in (HTML5 + CSS3 + JS, AJAX/fetch)
  =====================================================

  Wiring:
    LED1  -> D1 (GPIO5) -> 220ohm resistor -> GND
    LED2  -> D2 (GPIO4) -> 220ohm resistor -> GND

  How to use:
    1. Install "ESP8266" board package in Arduino IDE
       (Boards Manager -> search "esp8266")
    2. Select Board: NodeMCU 1.0 (ESP-12E Module)
    3. Update WIFI_SSID and WIFI_PASS below
    4. Upload the sketch
    5. Open Serial Monitor @ 115200 baud, note the IP printed
    6. On your phone (same WiFi network), open a browser and
       go to that IP address, e.g. http://192.168.1.15
    7. Control LEDs with the toggle switches
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ---------- CONFIG ----------
const char* WIFI_SSID = "mechanical Dept";
const char* WIFI_PASS = "2026@mechanical";

#define LED1_PIN D1   // GPIO5
#define LED2_PIN D2   // GPIO4
// -----------------------------

ESP8266WebServer server(80);

bool led1State = false;
bool led2State = false;

// ---------------- HTML PAGE (mobile web app) ----------------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Home Automation</title>
<style>
  * { box-sizing: border-box; margin:0; padding:0; }
  body {
    font-family: 'Segoe UI', Arial, sans-serif;
    background: linear-gradient(135deg,#1f2933,#0b0f14);
    min-height: 100vh;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 20px;
  }
  .app {
    width: 100%;
    max-width: 400px;
    background: #14181f;
    border-radius: 20px;
    padding: 28px 22px;
    box-shadow: 0 10px 30px rgba(0,0,0,0.5);
    color: #fff;
  }
  h1 {
    text-align: center;
    font-size: 22px;
    margin-bottom: 6px;
    letter-spacing: 0.5px;
  }
  .subtitle {
    text-align: center;
    color: #8b95a1;
    font-size: 13px;
    margin-bottom: 26px;
  }
  .card {
    background: #1c2230;
    border-radius: 16px;
    padding: 18px 20px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 16px;
    transition: background 0.3s;
  }
  .card.on { background: #24304a; }
  .device-info { display: flex; align-items: center; gap: 14px; }
  .icon {
    width: 44px; height: 44px;
    border-radius: 50%;
    display: flex; align-items: center; justify-content: center;
    background: #2c3444;
    font-size: 20px;
    transition: 0.3s;
  }
  .card.on .icon { background: #ffb733; box-shadow: 0 0 18px #ffb73399; }
  .device-name { font-size: 16px; font-weight: 600; }
  .device-status { font-size: 12px; color: #8b95a1; margin-top: 2px; }

  /* Toggle switch */
  .switch { position: relative; width: 54px; height: 30px; }
  .switch input { opacity: 0; width: 0; height: 0; }
  .slider {
    position: absolute; cursor: pointer;
    top:0; left:0; right:0; bottom:0;
    background-color: #3a4356;
    transition: 0.3s;
    border-radius: 30px;
  }
  .slider:before {
    position: absolute;
    content: "";
    height: 24px; width: 24px;
    left: 3px; bottom: 3px;
    background-color: white;
    transition: 0.3s;
    border-radius: 50%;
  }
  input:checked + .slider { background-color: #ffb733; }
  input:checked + .slider:before { transform: translateX(24px); }

  .footer {
    text-align: center;
    margin-top: 20px;
    font-size: 11px;
    color: #545c6b;
  }
  .connDot {
    display:inline-block; width:8px; height:8px; border-radius:50%;
    background:#3ddc84; margin-right:6px;
  }

  .micBtn {
    width: 100%;
    margin-top: 4px;
    padding: 14px;
    border: none;
    border-radius: 14px;
    background: #2c3444;
    color: #fff;
    font-size: 15px;
    font-weight: 600;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 10px;
    cursor: pointer;
    transition: 0.25s;
  }
  .micBtn:active { transform: scale(0.97); }
  .micBtn.listening {
    background: #d84343;
    box-shadow: 0 0 20px #d8434377;
    animation: pulse 1.2s infinite;
  }
  @keyframes pulse {
    0% { box-shadow: 0 0 0 0 #d8434366; }
    70% { box-shadow: 0 0 0 14px #d8434300; }
    100% { box-shadow: 0 0 0 0 #d8434300; }
  }
  .voiceText {
    text-align: center;
    font-size: 11px;
    color: #6b7382;
    margin-top: 10px;
    line-height: 1.5;
  }
</style>
</head>
<body>
  <div class="app">
    <h1>🏠 Home Automation</h1>
    <div class="subtitle">NodeMCU ESP8266 &middot; LED Control</div>

    <div class="card" id="card1">
      <div class="device-info">
        <div class="icon" id="icon1">💡</div>
        <div>
          <div class="device-name">LED 1</div>
          <div class="device-status" id="status1">OFF</div>
        </div>
      </div>
      <label class="switch">
        <input type="checkbox" id="toggle1" onchange="toggleLed(1)">
        <span class="slider"></span>
      </label>
    </div>

    <div class="card" id="card2">
      <div class="device-info">
        <div class="icon" id="icon2">💡</div>
        <div>
          <div class="device-name">LED 2</div>
          <div class="device-status" id="status2">OFF</div>
        </div>
      </div>
      <label class="switch">
        <input type="checkbox" id="toggle2" onchange="toggleLed(2)">
        <span class="slider"></span>
      </label>
    </div>

    <button class="micBtn" id="micBtn" onclick="toggleListening()">
      <span id="micIcon">🎤</span> <span id="micLabel">Tap to Speak</span>
    </button>
    <div class="voiceText" id="voiceText">Say "turn on led 1" / "turn off led 2" / "all on" / "all off"</div>

    <div class="footer"><span class="connDot"></span>Connected to NodeMCU</div>
  </div>

<script>
  // Fetch current state from NodeMCU and sync UI
  function refreshState() {
    fetch('/state')
      .then(res => res.json())
      .then(data => {
        setUI(1, data.led1);
        setUI(2, data.led2);
      })
      .catch(err => console.log('State fetch failed', err));
  }

  function setUI(num, isOn) {
    document.getElementById('toggle' + num).checked = isOn;
    document.getElementById('status' + num).textContent = isOn ? 'ON' : 'OFF';
    document.getElementById('card' + num).classList.toggle('on', isOn);
  }

  function toggleLed(num) {
    const isOn = document.getElementById('toggle' + num).checked;
    const endpoint = '/led' + num + (isOn ? 'on' : 'off');
    fetch(endpoint)
      .then(res => res.json())
      .then(data => {
        setUI(1, data.led1);
        setUI(2, data.led2);
      })
      .catch(err => console.log('Toggle failed', err));
  }

  // Sync UI on page load, and poll every 3s in case state
  // changed from elsewhere (e.g. physical button, other phone)
  window.onload = refreshState;
  setInterval(refreshState, 3000);

  // ---------------- Voice Command (Web Speech API) ----------------
  const micBtn = document.getElementById('micBtn');
  const micLabel = document.getElementById('micLabel');
  const voiceText = document.getElementById('voiceText');

  const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
  let recognition = null;
  let listening = false;

  if (SpeechRecognition) {
    recognition = new SpeechRecognition();
    recognition.lang = 'en-US';
    recognition.continuous = false;
    recognition.interimResults = false;

    recognition.onstart = () => {
      listening = true;
      micBtn.classList.add('listening');
      micLabel.textContent = 'Listening...';
    };

    recognition.onend = () => {
      listening = false;
      micBtn.classList.remove('listening');
      micLabel.textContent = 'Tap to Speak';
    };

    recognition.onerror = (e) => {
      voiceText.textContent = 'Voice error: ' + e.error + ' — try again';
    };

    recognition.onresult = (event) => {
      const transcript = event.results[0][0].transcript.toLowerCase().trim();
      voiceText.textContent = 'Heard: "' + transcript + '"';
      handleVoiceCommand(transcript);
    };
  } else {
    micLabel.textContent = 'Voice not supported';
    micBtn.disabled = true;
    voiceText.textContent = 'Your browser does not support voice recognition. Use Chrome on Android.';
  }

  function toggleListening() {
    if (!recognition) return;
    if (listening) {
      recognition.stop();
    } else {
      recognition.start();
    }
  }

  function handleVoiceCommand(text) {
    // Normalize common speech variants
    const isOn = /\bon\b/.test(text) && !/\bturn off|\bswitch off/.test(text);
    const isOff = /\boff\b/.test(text);

    const mentionsLed1 = /led\s*1|light\s*1|one/.test(text);
    const mentionsLed2 = /led\s*2|light\s*2|two/.test(text);
    const mentionsAll = /all|both/.test(text);

    if (mentionsAll) {
      if (isOn) { fetchAndSync('/led1on'); fetchAndSync('/led2on'); }
      else if (isOff) { fetchAndSync('/led1off'); fetchAndSync('/led2off'); }
      return;
    }

    if (mentionsLed1) {
      if (isOn) fetchAndSync('/led1on');
      else if (isOff) fetchAndSync('/led1off');
    }

    if (mentionsLed2) {
      if (isOn) fetchAndSync('/led2on');
      else if (isOff) fetchAndSync('/led2off');
    }
  }

  function fetchAndSync(endpoint) {
    fetch(endpoint)
      .then(res => res.json())
      .then(data => {
        setUI(1, data.led1);
        setUI(2, data.led2);
      })
      .catch(err => console.log('Voice command fetch failed', err));
  }
</script>
</body>
</html>
)rawliteral";

// ---------------- Handlers ----------------

void sendState() {
  String json = "{\"led1\":" + String(led1State ? "true" : "false") +
                ",\"led2\":" + String(led2State ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleLed1On()  { led1State = true;  digitalWrite(LED1_PIN, HIGH); sendState(); }
void handleLed1Off() { led1State = false; digitalWrite(LED1_PIN, LOW);  sendState(); }
void handleLed2On()  { led2State = true;  digitalWrite(LED2_PIN, HIGH); sendState(); }
void handleLed2Off() { led2State = false; digitalWrite(LED2_PIN, LOW);  sendState(); }
void handleState()   { sendState(); }

void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}

// ---------------- Setup / Loop ----------------

void setup() {
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("Open this URL on your phone browser: http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/led1on", handleLed1On);
  server.on("/led1off", handleLed1Off);
  server.on("/led2on", handleLed2On);
  server.on("/led2off", handleLed2Off);
  server.on("/state", handleState);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
