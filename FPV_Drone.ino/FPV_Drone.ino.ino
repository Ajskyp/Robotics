#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// ─── MOTOR PINS ────────────────────────────────────────────────────
#define MOTOR_M1  4
#define MOTOR_M2  5
#define MOTOR_M3  6
#define MOTOR_M4  7

// ─── RECEIVER INPUT PINS ───────────────────────────────────────────
#define CH_ROLL      A0
#define CH_PITCH     A1
#define CH_THROTTLE  A2
#define CH_YAW       A3

// ─── GPS ───────────────────────────────────────────────────────────
#define GPS_RX    1
#define GPS_TX    0
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
TinyGPSPlus gps;

// ─── ESC RANGE ─────────────────────────────────────────────────────
#define ESC_MIN   1000
#define ESC_ARM   1050
#define ESC_MAX   2000

// ─── PID GAINS ─────────────────────────────────────────────────────
float Kp = 1.2f, Ki = 0.02f, Kd = 0.8f;

float rollError  = 0, pitchError  = 0, yawError  = 0;
float rollPrev   = 0, pitchPrev   = 0, yawPrev   = 0;
float rollI      = 0, pitchI      = 0, yawI      = 0;

unsigned long lastTime = 0;

// ─── FORWARD DECLARATIONS ──────────────────────────────────────────
void writeAllMotors(int m1, int m2, int m3, int m4);
int readReceiver(int pin);

// ─── READ RECEIVER CHANNEL ─────────────────────────────────────────
// FlySky receiver outputs PWM 1000-2000µs on each channel
int readReceiver(int pin) {
  return pulseIn(pin, HIGH, 25000);  // timeout 25ms
}

// ─── PWM OUTPUT ────────────────────────────────────────────────────
void writeAllMotors(int m1, int m2, int m3, int m4) {
  m1 = constrain(m1, ESC_MIN, ESC_MAX);
  m2 = constrain(m2, ESC_MIN, ESC_MAX);
  m3 = constrain(m3, ESC_MIN, ESC_MAX);
  m4 = constrain(m4, ESC_MIN, ESC_MAX);

  PORTD |= 0b11110000;

  unsigned long start = micros();
  bool d4 = true, d5 = true, d6 = true, d7 = true;

  while (d4 || d5 || d6 || d7) {
    unsigned long elapsed = micros() - start;
    if (d4 && elapsed >= (unsigned long)m1) { PORTD &= ~(1 << 4); d4 = false; }
    if (d5 && elapsed >= (unsigned long)m2) { PORTD &= ~(1 << 5); d5 = false; }
    if (d6 && elapsed >= (unsigned long)m3) { PORTD &= ~(1 << 6); d6 = false; }
    if (d7 && elapsed >= (unsigned long)m4) { PORTD &= ~(1 << 7); d7 = false; }
  }
}

// ─── SETUP ─────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);

  pinMode(MOTOR_M1, OUTPUT);
  pinMode(MOTOR_M2, OUTPUT);
  pinMode(MOTOR_M3, OUTPUT);
  pinMode(MOTOR_M4, OUTPUT);

  pinMode(CH_ROLL,     INPUT);
  pinMode(CH_PITCH,    INPUT);
  pinMode(CH_THROTTLE, INPUT);
  pinMode(CH_YAW,      INPUT);

  // ESC arming
  Serial.println("Arming ESC...");
  for (int i = 0; i < 200; i++) {
    writeAllMotors(ESC_MIN, ESC_MIN, ESC_MIN, ESC_MIN);
    delay(10);
  }
  for (int i = 0; i < 100; i++) {
    writeAllMotors(ESC_ARM, ESC_ARM, ESC_ARM, ESC_ARM);
    delay(10);
  }
  Serial.println("Ready. Push throttle stick down to arm controller.");

  lastTime = millis();
}

// ─── LOOP ──────────────────────────────────────────────────────────
void loop() {
  // Feed GPS
  while (gpsSerial.available())
    gps.encode(gpsSerial.read());

  // ── Read receiver channels ───────────────────────────────────────
  int rxRoll     = readReceiver(CH_ROLL);      // 1000-2000
  int rxPitch    = readReceiver(CH_PITCH);     // 1000-2000
  int rxThrottle = readReceiver(CH_THROTTLE);  // 1000-2000
  int rxYaw      = readReceiver(CH_YAW);       // 1000-2000

  // Safety: if no signal received, stop motors
  if (rxThrottle == 0) {
    writeAllMotors(ESC_MIN, ESC_MIN, ESC_MIN, ESC_MIN);
    Serial.println("No receiver signal!");
    return;
  }

  // ── Map receiver to target angles ───────────────────────────────
  // Stick center (1500) = 0 degrees, ends = ±30 degrees
  float rollTarget  = map(rxRoll,  1000, 2000, -30, 30);
  float pitchTarget = map(rxPitch, 1000, 2000, -30, 30);
  float yawTarget   = map(rxYaw,   1000, 2000, -30, 30);
  int   throttle    = constrain(rxThrottle, ESC_MIN, ESC_MAX);

  // ── IMU reads (replace 0.0f with MPU-6050) ──────────────────────
  float rollActual  = 0.0f;   // mpu.getAngleX()
  float pitchActual = 0.0f;   // mpu.getAngleY()
  float yawActual   = 0.0f;   // mpu.getAngleZ()

  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0f;
  if (dt < 0.001f) dt = 0.01f;
  lastTime = now;

  // ── PID ─────────────────────────────────────────────────────────
  rollError  = rollTarget  - rollActual;
  pitchError = pitchTarget - pitchActual;
  yawError   = yawTarget   - yawActual;

  rollI  = constrain(rollI  + rollError  * dt, -200.0f, 200.0f);
  pitchI = constrain(pitchI + pitchError * dt, -200.0f, 200.0f);
  yawI   = constrain(yawI   + yawError   * dt, -200.0f, 200.0f);

  float rollD  = (rollError  - rollPrev)  / dt;
  float pitchD = (pitchError - pitchPrev) / dt;
  float yawD   = (yawError   - yawPrev)   / dt;

  float rollCorr  = Kp*rollError  + Ki*rollI  + Kd*rollD;
  float pitchCorr = Kp*pitchError + Ki*pitchI + Kd*pitchD;
  float yawCorr   = Kp*yawError   + Ki*yawI   + Kd*yawD;

  rollPrev  = rollError;
  pitchPrev = pitchError;
  yawPrev   = yawError;

  // ── Motor mixing (X-frame) ───────────────────────────────────────
  //   M1(FL) M2(FR)
  //   M3(RL) M4(RR)
  int m1 = throttle + (int)rollCorr - (int)pitchCorr - (int)yawCorr;
  int m2 = throttle - (int)rollCorr - (int)pitchCorr + (int)yawCorr;
  int m3 = throttle + (int)rollCorr + (int)pitchCorr + (int)yawCorr;
  int m4 = throttle - (int)rollCorr + (int)pitchCorr - (int)yawCorr;

  writeAllMotors(m1, m2, m3, m4);

  // ── GPS every 2s ────────────────────────────────────────────────
  static unsigned long lastGPS = 0;
  if (millis() - lastGPS > 2000) {
    lastGPS = millis();
    if (gps.location.isValid()) {
      Serial.print("LAT:"); Serial.print(gps.location.lat(), 6);
      Serial.print(" LON:"); Serial.print(gps.location.lng(), 6);
      Serial.print(" ALT:"); Serial.println(gps.altitude.meters());
    } else {
      Serial.print("GPS sats: "); Serial.println(gps.satellites.value());
    }
  }

  delay(10);
}