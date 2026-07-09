/*
 * Fixed & Optimized Smart Parking System
 * * Hardware Connections:
 * - I2C LCD: SDA -> A4, SCL -> A5
 * - Servo Motor: Signal Pin -> Digital Pin 4
 * - IR Entry Sensor: Output -> Digital Pin 3
 * - IR Exit Sensor: Output -> Digital Pin 5
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// Pin Definitions
#define IR_SENSOR_ENTRY 3
#define IR_SENSOR_EXIT 5
#define SERVO_PIN 4

// Initialize 16x2 LCD at I2C Address 0x27
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo parkingServo;

// Configuration
int totalSlots = 2;       // Maximum parking slots available
const int maxSlots = 2;   // Upper threshold cap

void setup() {
  Serial.begin(9600);

  // FIX: Using lcd.init() instead of lcd.begin() for modern I2C libraries
  lcd.init(); 
  lcd.backlight();

  // Setup Actuator
  parkingServo.attach(SERVO_PIN);
  parkingServo.write(0); // Ensure gate starts closed (0 degrees)

  // Setup Sensor Inputs
  pinMode(IR_SENSOR_ENTRY, INPUT);
  pinMode(IR_SENSOR_EXIT, INPUT);

  // Splash Screen
  lcd.setCursor(0, 0);
  lcd.print("Smart Parking");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
}

void loop() {
  // Read sensor states (LOW = Object Detected)
  int entryDetected = digitalRead(IR_SENSOR_ENTRY);
  int exitDetected = digitalRead(IR_SENSOR_EXIT);

  // --- ENTRY DISPATCH LOGIC ---
  if (entryDetected == LOW) { 
    if (totalSlots > 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Access Granted ");
      lcd.setCursor(0, 1);
      lcd.print("Gate Opening... ");
      
      parkingServo.write(90); // Raise boom barrier
      totalSlots--;           // Decrement available slots
      delay(3000);            // Hold gate open for car to clear the sensor
      parkingServo.write(0);  // Close barrier
      lcd.clear();
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Parking Status: ");
      lcd.setCursor(0, 1);
      lcd.print("LOT FULL!       ");
      delay(2000);
      lcd.clear();
    }
  }

  // --- EXIT DISPATCH LOGIC ---
  if (exitDetected == LOW) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Car Exiting...  ");
    lcd.setCursor(0, 1);
    lcd.print("Gate Opening... ");
    
    parkingServo.write(90); // Raise boom barrier
    totalSlots++;           // Increment available slots
    if (totalSlots > maxSlots) totalSlots = maxSlots; // System sanity limit Check
    
    delay(3000);            // Hold gate open for car to clear the sensor
    parkingServo.write(0);  // Close barrier
    lcd.clear();
  }

  // --- STEADY STATE DEFAULT MONITOR ---
  lcd.setCursor(0, 0);
  lcd.print("Free Slots: ");
  lcd.print(totalSlots);
  lcd.print("   ");       // Clears leftover trailing characters/ghost digits
  
  lcd.setCursor(0, 1);
  if (totalSlots == 0) {
    lcd.print("Status: FULL    ");
  } else {
    lcd.print("Status: OPEN    ");
  }

  delay(200); // Polling stability pause
}
