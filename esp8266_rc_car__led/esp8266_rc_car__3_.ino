/*
  ============================================================
  ESP8266 (NodeMCU) + L298N Two-Wheel RC Car — v3
  ============================================================
  - Gamepad-style UI: left joystick drives, right joystick is a
    sticky vertical throttle-limiter fader, lightning = boost
  - Landscape layout (hold phone sideways like an RC controller)
  - WASD keyboard support retained for laptop use
  - Same backend as v2: /drive?x=..&y=.. differential mixing

  WIRING (unchanged):
    L298N ENA -> D5   (PWM, LEFT motor speed)
    L298N IN1 -> D1   (LEFT motor direction)
    L298N IN2 -> D2   (LEFT motor direction)
    L298N IN3 -> D7   (RIGHT motor direction)
    L298N IN4 -> D0   (RIGHT motor direction)
    L298N ENB -> D6   (PWM, RIGHT motor speed)
    L298N GND -> NodeMCU GND (shared with battery negative)
    L298N 12V/VIN -> 2x 18650 Li-ion in series (7.4V)

    Front of chassis: passive caster / roller ball (no wiring needed,
    just balances the frame since it's 2WD driven from the rear).
  ============================================================
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ---------- WIFI CREDENTIALS ----------
const char* ssid     = "Siddhu";
const char* password = "7020079306";

// ---------- MOTOR PIN DEFINITIONS ----------
#define ENA D5   // LEFT motor speed (PWM)
#define IN1 D1   // LEFT motor direction
#define IN2 D2   // LEFT motor direction
#define IN3 D7   // RIGHT motor direction
#define IN4 D0   // RIGHT motor direction
#define ENB D6   // RIGHT motor speed (PWM)

// ---------- HEADLIGHT (always on) ----------
#define LED_PIN D4   // external LED, always lit once powered up

ESP8266WebServer server(80);

// ============================================================
//                MOTOR CONTROL (proportional, -100..100)
// ============================================================
void setMotorLeft(int val) {
  val = constrain(val, -100, 100);
  int pwm = map(abs(val), 0, 100, 0, 1023);
  if (val > 0)      { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); }
  else if (val < 0) { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
  else              { digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW); pwm = 0; }
  analogWrite(ENA, pwm);
}

void setMotorRight(int val) {
  val = constrain(val, -100, 100);
  int pwm = map(abs(val), 0, 100, 0, 1023);
  if (val > 0)      { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); }
  else if (val < 0) { digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
  else              { digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW); pwm = 0; }
  analogWrite(ENB, pwm);
}

void stopMotors() {
  setMotorLeft(0);
  setMotorRight(0);
}

// Arcade-style differential drive mixing.
void drive(int x, int y) {
  x = constrain(x, -100, 100);
  y = constrain(y, -100, 100);
  int left  = constrain(y + x, -100, 100);
  int right = constrain(y - x, -100, 100);
  setMotorLeft(left);
  setMotorRight(right);
}

// ============================================================
//                      WEB DASHBOARD (HTML)
// ============================================================
const char MAIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no, viewport-fit=cover">
<title>RC Car Remote</title>
<style>
  :root{
    --gold:#ffcc33; --gold2:#ff9d1f; --cyan:#22e3ff; --danger:#ff4d5e;
    --text:#eef1f6; --muted:#8f97a8; --bg:#05060a;
  }
  *{box-sizing:border-box; -webkit-tap-highlight-color:transparent; user-select:none; touch-action:none;}
  html,body{ height:100%; overflow:hidden; }
  body{
    margin:0; background:
      radial-gradient(circle at 15% 20%, rgba(34,227,255,0.10), transparent 40%),
      radial-gradient(circle at 85% 25%, rgba(255,204,51,0.10), transparent 40%),
      radial-gradient(circle at 50% 90%, rgba(255,157,31,0.08), transparent 50%),
      var(--bg);
    color:var(--text); font-family:'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
    display:flex; flex-direction:column; height:100vh; padding:10px 14px 16px;
  }

  /* ---------- Top bar ---------- */
  .topbar{
    display:flex; align-items:center; justify-content:space-between;
    padding:8px 14px; border-radius:16px; background:rgba(255,255,255,0.04);
    border:1px solid rgba(255,255,255,0.08); backdrop-filter:blur(6px);
  }
  .stat{ display:flex; align-items:center; gap:6px; font-size:12px; color:var(--muted); }
  .dot{ width:8px; height:8px; border-radius:50%; background:var(--danger); box-shadow:0 0 8px var(--danger); }
  .dot.on{ background:var(--cyan); box-shadow:0 0 10px var(--cyan); }
  .timer{ font-size:13px; letter-spacing:2px; color:var(--gold); font-weight:600; }
  .boost{
    width:38px; height:38px; border-radius:50%; border:1px solid rgba(255,204,51,0.4);
    background:radial-gradient(circle, rgba(255,204,51,0.18), rgba(255,204,51,0.04));
    color:var(--gold); font-size:18px; display:flex; align-items:center; justify-content:center;
    box-shadow:0 0 10px rgba(255,204,51,0.25); cursor:pointer;
  }
  .boost.active{ background:radial-gradient(circle, var(--gold), var(--gold2)); color:#1a0e00;
    box-shadow:0 0 22px rgba(255,204,51,0.8); }

  /* ---------- Center speed gauge ---------- */
  .center{
    flex:1; display:flex; flex-direction:column; align-items:center; justify-content:center; gap:2px;
  }
  .gauge-wrap{ position:relative; width:118px; height:118px; }
  .gauge-wrap svg{ width:100%; height:100%; transform:rotate(-90deg); }
  .gauge-bg{ fill:none; stroke:rgba(255,255,255,0.07); stroke-width:8; }
  .gauge-fg{ fill:none; stroke:url(#grad); stroke-width:8; stroke-linecap:round;
    transition:stroke-dashoffset .12s linear; }
  .gauge-center{ position:absolute; inset:0; display:flex; flex-direction:column; align-items:center; justify-content:center; }
  .gauge-center b{ font-size:30px; line-height:1; color:var(--text); }
  .gauge-center small{ font-size:10px; color:var(--muted); letter-spacing:2px; margin-top:2px; }
  .limit-tag{ font-size:11px; color:var(--gold); margin-top:2px; letter-spacing:1px; }

  /* ---------- Controls row ---------- */
  .controls-row{
    display:flex; align-items:flex-end; justify-content:space-between; padding:0 6px 6px;
  }
  .joy-col{ display:flex; flex-direction:column; align-items:center; gap:6px; }
  .joy-tag{ font-size:10px; color:var(--muted); letter-spacing:2px; }

  .joy-base{
    position:relative; width:150px; height:150px; border-radius:50%;
    background:radial-gradient(circle at 35% 30%, #1a1d24, #0a0b0f 75%);
    border:2px solid rgba(255,204,51,0.25);
    box-shadow:0 0 0 6px rgba(255,204,51,0.05), inset 0 0 25px rgba(0,0,0,0.6), 0 0 20px rgba(255,204,51,0.12);
  }
  .joy-base.right{ border-color:rgba(34,227,255,0.3); box-shadow:0 0 0 6px rgba(34,227,255,0.05), inset 0 0 25px rgba(0,0,0,0.6), 0 0 20px rgba(34,227,255,0.15); }
  .joy-base::before{
    content:''; position:absolute; inset:12px; border-radius:50%; border:1px dashed rgba(255,255,255,0.08);
  }
  .joy-knob{
    position:absolute; width:56px; height:56px; border-radius:50%; left:47px; top:47px;
    background:radial-gradient(circle at 35% 30%, var(--gold), var(--gold2));
    box-shadow:0 0 18px rgba(255,204,51,0.6), 0 4px 10px rgba(0,0,0,0.5);
    display:flex; align-items:center; justify-content:center; font-size:22px;
    transition:left .08s ease, top .08s ease; cursor:grab;
  }
  .joy-knob.right{ background:radial-gradient(circle at 35% 30%, var(--cyan), #0090b0);
    box-shadow:0 0 18px rgba(34,227,255,0.6), 0 4px 10px rgba(0,0,0,0.5); }
  .joy-knob.dragging{ transition:none; cursor:grabbing; }

  /* ---------- Keyboard hint (desktop only) ---------- */
  .keys{ display:none; gap:6px; }
  .key{
    width:30px; height:30px; border-radius:7px; background:#14161c; border:1px solid rgba(255,255,255,0.1);
    display:flex; align-items:center; justify-content:center; font-size:12px; font-weight:700; color:var(--muted);
  }
  .key.active{ background:linear-gradient(145deg,var(--gold),var(--gold2)); color:#1a0e00; border-color:transparent; }
  @media (hover:hover) and (pointer:fine){ .keys{ display:flex; } }
</style>
</head>
<body>

<div class="topbar">
  <div class="stat"><div class="dot" id="statusDot"></div><span id="statusText">Connecting...</span></div>
  <div class="timer" id="timer">00:00</div>
  <div class="boost" id="boostBtn">&#9889;</div>
</div>

<div class="center">
  <div class="gauge-wrap">
    <svg viewBox="0 0 120 120">
      <defs>
        <linearGradient id="grad" x1="0%" y1="0%" x2="100%" y2="0%">
          <stop offset="0%" stop-color="#22e3ff"/>
          <stop offset="100%" stop-color="#ffcc33"/>
        </linearGradient>
      </defs>
      <circle class="gauge-bg" cx="60" cy="60" r="52"></circle>
      <circle class="gauge-fg" id="gaugeFg" cx="60" cy="60" r="52" stroke-dasharray="326.7" stroke-dashoffset="326.7"></circle>
    </svg>
    <div class="gauge-center">
      <b id="powerVal">0</b>
      <small>POWER %</small>
    </div>
  </div>
  <div class="limit-tag">LIMIT: <span id="limitVal">80</span>%</div>
</div>

<div class="controls-row">
  <div class="joy-col">
    <div class="joy-base" id="joyLeftBase">
      <div class="joy-knob" id="joyLeftKnob">&#128578;</div>
    </div>
    <div class="joy-tag">DRIVE</div>
    <div class="keys">
      <div class="key" id="keyW">W</div><div class="key" id="keyA">A</div>
      <div class="key" id="keyS">S</div><div class="key" id="keyD">D</div>
    </div>
  </div>

  <div class="joy-col">
    <div class="joy-base right" id="joyRightBase">
      <div class="joy-knob right" id="joyRightKnob">&#9889;</div>
    </div>
    <div class="joy-tag">SPEED LIMIT</div>
  </div>
</div>

<script>
  const dot = document.getElementById('statusDot');
  const statusText = document.getElementById('statusText');
  const powerVal = document.getElementById('powerVal');
  const limitValEl = document.getElementById('limitVal');
  const gaugeFg = document.getElementById('gaugeFg');
  const CIRC = 326.7;

  function setConnected(ok){
    dot.classList.toggle('on', ok);
    statusText.textContent = ok ? 'Connected' : 'Disconnected';
  }

  // ---------- Session timer ----------
  const startTime = Date.now();
  setInterval(()=>{
    const secs = Math.floor((Date.now()-startTime)/1000);
    const m = String(Math.floor(secs/60)).padStart(2,'0');
    const s = String(secs%60).padStart(2,'0');
    document.getElementById('timer').textContent = `${m}:${s}`;
  }, 1000);

  // ---------- Speed limiter (right joystick, sticky vertical fader) ----------
  let limit = 80;      // 20-100
  let boosting = false;

  function effectiveLimit(){ return boosting ? 100 : limit; }
  function updateLimitUI(){ limitValEl.textContent = boosting ? 100 : limit; }

  const boostBtn = document.getElementById('boostBtn');
  const startBoost = (e)=>{ e.preventDefault(); boosting = true; boostBtn.classList.add('active'); updateLimitUI(); };
  const endBoost = (e)=>{ e.preventDefault(); boosting = false; boostBtn.classList.remove('active'); updateLimitUI(); };
  boostBtn.addEventListener('pointerdown', startBoost);
  boostBtn.addEventListener('pointerup', endBoost);
  boostBtn.addEventListener('pointerleave', endBoost);
  boostBtn.addEventListener('pointercancel', endBoost);

  // ---------- Drive command throttling ----------
  let lastSentX = null, lastSentY = null;
  let rawX = 0, rawY = 0;

  function updateGauge(x, y){
    const mag = Math.min(100, Math.round(Math.sqrt(x*x + y*y)));
    powerVal.textContent = mag;
    gaugeFg.style.strokeDashoffset = CIRC * (1 - mag/100);
  }

  function queueDrive(x, y){ rawX = x; rawY = y; }

  setInterval(()=>{
    const f = effectiveLimit()/100;
    const sx = Math.round(rawX * f);
    const sy = Math.round(rawY * f);
    updateGauge(sx, sy);
    if (sx !== lastSentX || sy !== lastSentY){
      lastSentX = sx; lastSentY = sy;
      fetch(`/drive?x=${sx}&y=${sy}`).then(()=> setConnected(true)).catch(()=> setConnected(false));
    }
  }, 100);

  // ---------- Left joystick: momentary drive (springs back) ----------
  const lBase = document.getElementById('joyLeftBase');
  const lKnob = document.getElementById('joyLeftKnob');
  const LR = 47;
  let lDragging = false;

  function setKnobPos(knob, dx, dy){ knob.style.left = (47+dx)+'px'; knob.style.top = (47+dy)+'px'; }

  lBase.addEventListener('pointerdown', (e)=>{
    lDragging = true; lKnob.classList.add('dragging'); lBase.setPointerCapture(e.pointerId); handleLeft(e);
  });
  lBase.addEventListener('pointermove', (e)=>{ if(lDragging) handleLeft(e); });
  function endLeft(){ if(!lDragging) return; lDragging=false; lKnob.classList.remove('dragging'); setKnobPos(lKnob,0,0); queueDrive(0,0); }
  lBase.addEventListener('pointerup', endLeft);
  lBase.addEventListener('pointercancel', endLeft);

  function handleLeft(e){
    const rect = lBase.getBoundingClientRect();
    let dx = e.clientX - (rect.left+rect.width/2);
    let dy = e.clientY - (rect.top+rect.height/2);
    const dist = Math.sqrt(dx*dx+dy*dy);
    if (dist > LR){ const r = LR/dist; dx*=r; dy*=r; }
    setKnobPos(lKnob, dx, dy);
    queueDrive(Math.round((dx/LR)*100), Math.round((-dy/LR)*100));
  }

  // ---------- Right joystick: sticky vertical-only limiter fader ----------
  const rBase = document.getElementById('joyRightBase');
  const rKnob = document.getElementById('joyRightKnob');
  let rDragging = false;

  function setRightKnobFromLimit(l){
    const pct = (l-20)/80; // 0..1
    const dy = LR - pct*(2*LR); // top(-LR)=100%, bottom(+LR)=20%
    setKnobPos(rKnob, 0, dy);
  }
  setRightKnobFromLimit(limit);

  rBase.addEventListener('pointerdown', (e)=>{
    rDragging = true; rKnob.classList.add('dragging'); rBase.setPointerCapture(e.pointerId); handleRight(e);
  });
  rBase.addEventListener('pointermove', (e)=>{ if(rDragging) handleRight(e); });
  function endRight(){ if(!rDragging) return; rDragging=false; rKnob.classList.remove('dragging'); }
  rBase.addEventListener('pointerup', endRight);
  rBase.addEventListener('pointercancel', endRight);

  function handleRight(e){
    const rect = rBase.getBoundingClientRect();
    let dy = e.clientY - (rect.top+rect.height/2);
    dy = Math.max(-LR, Math.min(LR, dy));
    setKnobPos(rKnob, 0, dy);
    const pct = (LR - dy) / (2*LR); // 0 at bottom, 1 at top
    limit = Math.round(20 + pct*80); // 20..100
    updateLimitUI();
  }

  // ---------- WASD keyboard control ----------
  const keysPressed = { w:false, a:false, s:false, d:false };
  const keyEls = { w:document.getElementById('keyW'), a:document.getElementById('keyA'),
                   s:document.getElementById('keyS'), d:document.getElementById('keyD') };
  function updateFromKeys(){
    const y = (keysPressed.w?100:0) - (keysPressed.s?100:0);
    const x = (keysPressed.d?100:0) - (keysPressed.a?100:0);
    queueDrive(x,y);
  }
  window.addEventListener('keydown', (e)=>{
    const k = e.key.toLowerCase();
    if (k in keysPressed && !keysPressed[k]){ keysPressed[k]=true; keyEls[k].classList.add('active'); updateFromKeys(); }
  });
  window.addEventListener('keyup', (e)=>{
    const k = e.key.toLowerCase();
    if (k in keysPressed){ keysPressed[k]=false; keyEls[k].classList.remove('active'); updateFromKeys(); }
  });
  window.addEventListener('blur', ()=>{
    keysPressed.w=keysPressed.a=keysPressed.s=keysPressed.d=false;
    Object.values(keyEls).forEach(el=>el.classList.remove('active'));
    queueDrive(0,0); setKnobPos(lKnob,0,0);
  });

  updateLimitUI();
  setInterval(()=>{ fetch('/ping').then(()=> setConnected(true)).catch(()=> setConnected(false)); }, 3000);
</script>
</body>
</html>
)rawliteral";

// ============================================================
//                      HTTP HANDLERS
// ============================================================
void handleRoot() { server.send_P(200, "text/html", MAIN_PAGE); }

void handleDrive() {
  int x = 0, y = 0;
  if (server.hasArg("x")) x = server.arg("x").toInt();
  if (server.hasArg("y")) y = server.arg("y").toInt();
  drive(x, y);
  server.send(200, "text/plain", "OK");
}

void handleStop() { stopMotors(); server.send(200, "text/plain", "OK"); }
void handlePing() { server.send(200, "text/plain", "pong"); }
void handleNotFound() { server.send(404, "text/plain", "Not found"); }

// ============================================================
//                      SETUP / LOOP
// ============================================================
void setup() {
  Serial.begin(115200);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
  stopMotors();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);   // headlight on permanently, no further control needed

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Open this IP in your browser to control the car.");

  server.on("/", handleRoot);
  server.on("/drive", handleDrive);
  server.on("/stop", handleStop);
  server.on("/ping", handlePing);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();
}
