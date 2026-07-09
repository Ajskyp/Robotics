/*
  ============================================================
  ESP8266 (NodeMCU) + L298N Two-Wheel RC Car — v3 (DIAGONAL SUPPORT)
  ============================================================
  - Virtual joystick control (mobile + laptop, proportional speed)
  - WASD keyboard control on laptop (diagonals supported!)
  - Speed limiter slider (caps max power)
  - True differential drive mixing (arcade drive)
  - Diagonal movement: Forward+Left, Forward+Right, Backward+Left, Backward+Right

  FIX NOTES (see chat for details):
  - The dashboard's fetch() call was missing backticks around its template
    string, which silently broke ALL the page's JavaScript (joystick, WASD,
    and the connection indicator). That's fixed below.
  - Added WiFi status-code diagnostics to setup() so the Serial Monitor tells
    you *why* it's stuck if it still won't join your network.

  WIRING (unchanged):
    L298N ENA -> D5   (PWM, LEFT motor speed)
    L298N IN1 -> D1   (LEFT motor direction)
    L298N IN2 -> D2   (LEFT motor direction)
    L298N IN3 -> D7   (RIGHT motor direction)
    L298N IN4 -> D0   (RIGHT motor direction)
    L298N ENB -> D6   (PWM, RIGHT motor speed)
    L298N GND -> NodeMCU GND (shared with battery negative)
    L298N 12V/VIN -> 2x 18650 Li-ion in series (7.4V)
  ============================================================
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ---------- WIFI CREDENTIALS ----------
const char* ssid     = "SINDHUDURG WING";
const char* password = "86934398";

// ---------- MOTOR PIN DEFINITIONS ----------
#define ENA D5   // LEFT motor speed (PWM)
#define IN1 D1   // LEFT motor direction
#define IN2 D2   // LEFT motor direction
#define IN3 D7   // RIGHT motor direction
#define IN4 D0   // RIGHT motor direction
#define ENB D6   // RIGHT motor speed (PWM)

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
// x = turn (-100 left .. 100 right), y = throttle (-100 back .. 100 fwd)
void drive(int x, int y) {
  x = constrain(x, -100, 100);
  y = constrain(y, -100, 100);
  
  // DIAGONAL MOVEMENT: Mix turn and throttle
  int left  = constrain(y + x, -100, 100);
  int right = constrain(y - x, -100, 100);
  
  // Enable diagonal movement by keeping both motors active
  // The differential drive naturally handles diagonals!
  setMotorLeft(left);
  setMotorRight(right);
  
  // Debug output (optional - uncomment for testing)
  // Serial.printf("X:%d Y:%d | L:%d R:%d\n", x, y, left, right);
}

// ============================================================
//                      WEB DASHBOARD (HTML)
// ============================================================
const char MAIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>RC Car Remote - Diagonal Support</title>
<style>
  :root{
    --bg1:#0b0d12; --accent:#00e5a0; --accent2:#00b8ff;
    --danger:#ff4d5e; --text:#e8ecf1; --muted:#8b93a3;
  }
  *{box-sizing:border-box; -webkit-tap-highlight-color:transparent; user-select:none; touch-action:none;}
  html,body{ height:100%; }
  body{margin:0; display:flex; flex-direction:column; align-items:center;
    gap:14px; padding:18px 16px 30px; min-height:100vh;
    background:radial-gradient(circle at 20% 0%, #1c2230 0%, var(--bg1) 55%);
    font-family:'Segoe UI', Roboto, Helvetica, Arial, sans-serif; color:var(--text);
  }
  header{ text-align:center; }
  header h1{
    margin:0; font-size:21px; letter-spacing:1px; font-weight:700;
    background:linear-gradient(90deg,var(--accent),var(--accent2));
    -webkit-background-clip:text; background-clip:text; color:transparent;
  }
  header p{ margin:4px 0 0; font-size:12px; color:var(--muted); }

  .status{
    display:flex; align-items:center; gap:8px; font-size:12px; color:var(--muted);
    background:rgba(255,255,255,0.04); padding:6px 14px; border-radius:20px;
    border:1px solid rgba(255,255,255,0.06);
  }
  .dot{ width:9px; height:9px; border-radius:50%; background:var(--danger); box-shadow:0 0 8px var(--danger); }
  .dot.on{ background:var(--accent); box-shadow:0 0 10px var(--accent); }

  .panel{
    width:100%; max-width:380px; background:linear-gradient(160deg, rgba(255,255,255,0.05), rgba(255,255,255,0.02));
    border:1px solid rgba(255,255,255,0.08); border-radius:22px; padding:20px;
    backdrop-filter:blur(10px); box-shadow:0 10px 30px rgba(0,0,0,0.4);
    display:flex; flex-direction:column; align-items:center; gap:18px;
  }

  /* Joystick */
  .joy-wrap{ position:relative; width:220px; height:220px; }
  .joy-base{
    width:220px; height:220px; border-radius:50%;
    background:radial-gradient(circle at 35% 30%, #232a36, #12151b 75%);
    border:1px solid rgba(255,255,255,0.08);
    box-shadow:inset 0 0 25px rgba(0,0,0,0.6), 0 6px 18px rgba(0,0,0,0.4);
    position:relative;
  }
  .joy-base::before{
    content:''; position:absolute; inset:14px; border-radius:50%;
    border:1px dashed rgba(255,255,255,0.08);
  }
  /* Diagonal direction indicators */
  .dir-label{
    position:absolute; font-size:9px; color:rgba(255,255,255,0.15);
    font-weight:700; letter-spacing:0.5px;
  }
  .dir-top{ top:5px; left:50%; transform:translateX(-50%); }
  .dir-bottom{ bottom:5px; left:50%; transform:translateX(-50%); }
  .dir-left{ top:50%; left:5px; transform:translateY(-50%); }
  .dir-right{ top:50%; right:5px; transform:translateY(-50%); }
  
  .joy-knob{
    position:absolute; width:78px; height:78px; border-radius:50%; left:71px; top:71px;
    background:linear-gradient(145deg,var(--accent),var(--accent2));
    box-shadow:0 0 20px rgba(0,229,160,0.5), 0 4px 10px rgba(0,0,0,0.4);
    display:flex; align-items:center; justify-content:center; font-size:22px; color:#08110d;
    transition:left .08s ease, top .08s ease;
    cursor:grab;
  }
  .joy-knob.dragging{ transition:none; cursor:grabbing; }

  .readout{
    display:flex; justify-content:space-between; width:100%; font-size:12px; color:var(--muted);
  }
  .readout span b{ color:var(--accent); }

  .speed-block{ width:100%; }
  .speed-label{ display:flex; justify-content:space-between; font-size:13px; color:var(--muted); margin-bottom:8px; }
  #limitVal{ color:var(--accent); font-weight:600; }
  input[type=range]{
    -webkit-appearance:none; width:100%; height:6px; border-radius:6px;
    background:linear-gradient(90deg,var(--accent),var(--accent2)); outline:none;
  }
  input[type=range]::-webkit-slider-thumb{
    -webkit-appearance:none; width:22px; height:22px; border-radius:50%;
    background:#fff; border:3px solid var(--accent2); cursor:pointer; box-shadow:0 2px 6px rgba(0,0,0,0.4);
  }

  .keys{
    display:none; gap:6px; margin-top:2px;
  }
  .key{
    width:34px; height:34px; border-radius:8px; background:#161a22; border:1px solid rgba(255,255,255,0.1);
    display:flex; align-items:center; justify-content:center; font-size:13px; font-weight:700; color:var(--muted);
  }
  .key.active{ background:linear-gradient(145deg,var(--accent),var(--accent2)); color:#0b0e13; border-color:transparent; 
  }
  .movement-info{
    font-size:11px; color:var(--muted); text-align:center; padding:6px;
    background:rgba(255,255,255,0.04); border-radius:12px;
    width:100%;
  }
  .movement-info span{ color:var(--accent); font-weight:600; }

  footer{ font-size:11px; color:var(--muted); text-align:center; max-width:320px; line-height:1.6; }

  @media (hover:hover) and (pointer:fine){
    .keys{ display:flex; }
  }
</style>
</head>
<body>

<header>
  <h1>RC CAR REMOTE</h1>
  <p>ESP8266 &bull; L298N &bull; DIAGONAL DRIVE</p>
</header>

<div class="status">
  <div class="dot" id="statusDot"></div>
  <span id="statusText">Connecting...</span>
</div>

<div class="panel">
  <div class="joy-wrap">
    <div class="joy-base" id="joyBase">
      <div class="dir-label dir-top">FWD</div>
      <div class="dir-label dir-bottom">BWD</div>
      <div class="dir-label dir-left">LEFT</div>
      <div class="dir-label dir-right">RIGHT</div>
      <div class="joy-knob" id="joyKnob">&#9673;</div>
    </div>
  </div>

  <div class="readout">
    <span>THROTTLE: <b id="yVal">0</b></span>
    <span>TURN: <b id="xVal">0</b></span>
  </div>

  <div class="keys" id="wasdKeys">
    <div class="key" id="keyW">W</div>
    <div class="key" id="keyA">A</div>
    <div class="key" id="keyS">S</div>
    <div class="key" id="keyD">D</div>
  </div>

  <div class="speed-block">
    <div class="speed-label"><span>MAX SPEED LIMIT</span><span id="limitVal">80%</span></div>
    <input type="range" id="limitSlider" min="20" max="100" value="80">
  </div>
  
  <div class="movement-info" id="movementInfo">
    🚗 <span id="dirDisplay">STOPPED</span>
  </div>
</div>

<footer>Drag joystick for diagonal movement! W/A/S/D for keyboard control. Release to stop.</footer>

<script>
  const dot = document.getElementById('statusDot');
  const statusText = document.getElementById('statusText');
  const xValEl = document.getElementById('xVal');
  const yValEl = document.getElementById('yVal');
  const dirDisplay = document.getElementById('dirDisplay');

  function setConnected(ok){
    dot.classList.toggle('on', ok);
    statusText.textContent = ok ? 'Connected' : 'Disconnected';
  }

  // ---------- Direction Display ----------
  function updateDirection(x, y) {
    const threshold = 15;
    let dir = '';
    
    if (Math.abs(x) < threshold && Math.abs(y) < threshold) {
      dir = '⏹ STOPPED';
    } else {
      // Vertical (forward/backward)
      if (y > threshold) dir = '⬆ FORWARD';
      else if (y < -threshold) dir = '⬇ BACKWARD';
      
      // Horizontal (left/right) - combine with vertical for diagonal
      if (x > threshold) {
        dir = dir ? (dir + ' + ➡ RIGHT') : '➡ RIGHT';
      } else if (x < -threshold) {
        dir = dir ? (dir + ' + ⬅ LEFT') : '⬅ LEFT';
      }
    }
    dirDisplay.textContent = dir || 'STOPPED';
  }

  // ---------- Speed limiter ----------
  let limit = 80;
  const limitSlider = document.getElementById('limitSlider');
  const limitVal = document.getElementById('limitVal');
  limitSlider.addEventListener('input', ()=>{
    limit = parseInt(limitSlider.value, 10);
    limitVal.textContent = limit + '%';
  });

  // ---------- Sending drive commands (throttled) ----------
  let lastSentX = null, lastSentY = null;
  let pendingX = 0, pendingY = 0;

  function scale(v){ return Math.round(v * (limit/100)); }

  function queueDrive(x, y){
    pendingX = x; pendingY = y;
    xValEl.textContent = scale(x);
    yValEl.textContent = scale(y);
    updateDirection(x, y);
  }

  setInterval(()=>{
    const sx = scale(pendingX), sy = scale(pendingY);
    if (sx !== lastSentX || sy !== lastSentY){
      lastSentX = sx; lastSentY = sy;
      fetch(`/drive?x=${sx}&y=${sy}`)
        .then(()=> setConnected(true))
        .catch(()=> setConnected(false));
    }
  }, 100);

  // ---------- Joystick (pointer events: works for mouse + touch) ----------
  const base = document.getElementById('joyBase');
  const knob = document.getElementById('joyKnob');
  const radius = 71; // max travel distance from center in px
  let dragging = false;
  function setKnob(dx, dy){
    knob.style.left = (71 + dx) + 'px';
    knob.style.top  = (71 + dy) + 'px';
  }

  function resetKnob(){
    setKnob(0, 0);
    queueDrive(0, 0);
  }

  base.addEventListener('pointerdown', (e)=>{
    dragging = true;
    knob.classList.add('dragging');
    base.setPointerCapture(e.pointerId);
    handlePointer(e);
  });

  base.addEventListener('pointermove', (e)=>{
    if (!dragging) return;
    handlePointer(e);
  });

  function endDrag(e){
    if (!dragging) return;
    dragging = false;
    knob.classList.remove('dragging');
    resetKnob();
  }
  base.addEventListener('pointerup', endDrag);
  base.addEventListener('pointercancel', endDrag);

  function handlePointer(e){
    const rect = base.getBoundingClientRect();
    const cx = rect.left + rect.width/2;
    const cy = rect.top + rect.height/2;
    let dx = e.clientX - cx;
    let dy = e.clientY - cy;
    const dist = Math.sqrt(dx*dx + dy*dy);
    if (dist > radius){
      const ratio = radius / dist;
      dx *= ratio; dy *= ratio;
    }
    setKnob(dx, dy);
    // x: right positive, y: up positive (invert dy)
    const x = Math.round((dx / radius) * 100);
    const y = Math.round((-dy / radius) * 100);
    queueDrive(x, y);
  }

  // ---------- WASD keyboard control (DIAGONALS SUPPORTED!) ----------
  const keysPressed = { w:false, a:false, s:false, d:false };
  const keyEls = { w:document.getElementById('keyW'), a:document.getElementById('keyA'),
                   s:document.getElementById('keyS'), d:document.getElementById('keyD') };

  function updateFromKeys(){
    // Combine keys for diagonal movement!
    // W+A = Forward+Left, W+D = Forward+Right
    // S+A = Backward+Left, S+D = Backward+Right
    const y = (keysPressed.w ? 100 : 0) - (keysPressed.s ? 100 : 0);
    const x = (keysPressed.d ? 100 : 0) - (keysPressed.a ? 100 : 0);
    
    // For diagonal, the car will naturally turn and move forward/backward
    queueDrive(x, y);
  }

  window.addEventListener('keydown', (e)=>{
    const k = e.key.toLowerCase();
    if (k in keysPressed && !keysPressed[k]){
      keysPressed[k] = true;
      keyEls[k].classList.add('active');
      updateFromKeys();
    }
  });

  window.addEventListener('keyup', (e)=>{
    const k = e.key.toLowerCase();
    if (k in keysPressed){
      keysPressed[k] = false;
      keyEls[k].classList.remove('active');
      updateFromKeys();
    }
  });

  // Stop everything if the window/tab loses focus (safety)
  window.addEventListener('blur', ()=>{
    keysPressed.w = keysPressed.a = keysPressed.s = keysPressed.d = false;
    Object.values(keyEls).forEach(el => el.classList.remove('active'));
    resetKnob();
  });

  // Periodic connection check
  setInterval(()=>{
    fetch('/ping').then(()=> setConnected(true)).catch(()=> setConnected(false));
  }, 3000);
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
  
  // Debug output to Serial Monitor
  Serial.printf("Drive Command - X:%d Y:%d\n", x, y);
  
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
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  int dotCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
    dotCount++;
    // Every ~6 seconds, print the numeric status so you can tell WHY it's stuck:
    //   0 = WL_IDLE_STATUS       1 = WL_NO_SSID_AVAIL (SSID not found — ESP8266 is
    //                                2.4GHz ONLY, check your router isn't 5GHz-only)
    //   4 = WL_CONNECT_FAILED    (wrong password)      6 = WL_DISCONNECTED
    if (dotCount % 15 == 0) {
      Serial.printf("\n  [still trying... WiFi.status()=%d]\n", WiFi.status());
    }
  }

  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Open this IP in your browser to control the car.");
  Serial.println("DIAGONAL MOVEMENT ENABLED!");
  Serial.println("Combined movements: Forward+Left, Forward+Right, Backward+Left, Backward+Right");

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
