/*
 * Arduino Sketch for an Obstacle Avoiding Robot
 * 
 * This code controls an obstacle avoiding robot using an Arduino UNO R3, 
 * an ultrasonic sensor, and two BO motors with wheels driven by an L298N 
 * motor driver. The robot moves forward until it detects an obstacle 
 * within a certain distance, at which point it stops and turns to avoid 
 * the obstacle.
 */

#define TRIG_PIN 2
#define ECHO_PIN 3
#define MOTOR1_PIN1 8
#define MOTOR1_PIN2 9
#define MOTOR2_PIN1 10
#define MOTOR2_PIN2 11
#define MOTOR_SPEED 255
#define SAFE_DISTANCE 20 // in centimeters

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MOTOR1_PIN1, OUTPUT);
  pinMode(MOTOR1_PIN2, OUTPUT);
  pinMode(MOTOR2_PIN1, OUTPUT);
  pinMode(MOTOR2_PIN2, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  long duration, distance;
  
  // Send a 10us pulse to trigger the ultrasonic sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read the echo pin and calculate distance
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = (duration / 2) / 29.1; // Convert to cm
  
  Serial.print("Distance: ");
  Serial.println(distance);
  
  if (distance < SAFE_DISTANCE) {
    // Stop motors if obstacle is detected
    stopMotors();
    delay(500);
    // Turn right
    turnRight();
    delay(500);
  } else {
    // Move forward
    moveForward();
  }
}

void moveForward() {
  analogWrite(MOTOR1_PIN1, MOTOR_SPEED);
  analogWrite(MOTOR1_PIN2, 0);
  analogWrite(MOTOR2_PIN1, MOTOR_SPEED);
  analogWrite(MOTOR2_PIN2, 0);
}

void stopMotors() {
  analogWrite(MOTOR1_PIN1, 0);
  analogWrite(MOTOR1_PIN2, 0);
  analogWrite(MOTOR2_PIN1, 0);
  analogWrite(MOTOR2_PIN2, 0);
}

void turnRight() {
  analogWrite(MOTOR1_PIN1, MOTOR_SPEED);
  analogWrite(MOTOR1_PIN2, 0);
  analogWrite(MOTOR2_PIN1, 0);
  analogWrite(MOTOR2_PIN2, MOTOR_SPEED);
}