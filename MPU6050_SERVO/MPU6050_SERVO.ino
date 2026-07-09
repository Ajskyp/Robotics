#include <Wire.h>
#include <LSM303.h>
#include <Servo.h>
#include <math.h>

LSM303 compass;
Servo myServo;

const int servoPin = 13;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  if (!compass.init()) {
    Serial.println("LSM303 not found!");
    while (1);
  }

  compass.enableDefault();
  myServo.attach(servoPin);
}

void loop() {
  compass.read();

  float ax = compass.a.x;
  float ay = compass.a.y;
  float az = compass.a.z;

  // Roll = tilt around X axis
  float roll = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;

  // Map roll (-90 to +90) to servo range (0 to 180)
  int servoAngle = map(roll, -90, 90, 0, 180);
  servoAngle = constrain(servoAngle, 0, 180);  // safety clamp

  myServo.write(servoAngle);

  Serial.print("Roll: ");
  Serial.print(roll);
  Serial.print(" deg\tServo angle: ");
  Serial.println(servoAngle);

  delay(50);
}