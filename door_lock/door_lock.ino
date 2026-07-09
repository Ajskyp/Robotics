#include <Servo.h>
#include <SoftwareSerial.h>

// Define pins for Bluetooth
SoftwareSerial Bluetooth(1, 0); // RX, TX

// Create servo object
Servo lockServo;

char command;
const int lockAngle = 0;     // Adjust based on your physical lock setup
const int unlockAngle = 90;  // Adjust based on your physical lock setup

void setup() {
  // Initialize Serial communication for debugging and Bluetooth
  Serial.begin(9600);
  Bluetooth.begin(9600);
  
  // Attach servo to pin 9
  lockServo.attach(3);
  
  // Set initial state to Locked
  lockServo.write(lockAngle); 
  Serial.println("System Initialized. Door Locked.");
}

void loop() {
  if (Bluetooth.available() > 0) {
    command = Bluetooth.read();
    Serial.print("Command Received: ");
    Serial.println(command);
    
    if (command == 'U' || command == 'u') {
      lockServo.write(unlockAngle);
      Serial.println("Door Unlocked.");
    } 
    else if (command == 'L' || command == 'l') {
      lockServo.write(lockAngle);
      Serial.println("Door Locked.");
    }
  }
}