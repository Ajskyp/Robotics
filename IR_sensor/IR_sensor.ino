// C++ code
//
#include <Servo.h>

int servo = 0;

int ir = 0;

Servo servo_9;

void setup()
{
  pinMode(2, INPUT);
  Serial.begin(9600);
  servo_9.attach(9, 500, 2500);
}

void loop()
{
  ir = digitalRead(2);
  Serial.println(ir);
  if (ir == 1) {
    servo_9.write(45);
  } else {
    servo_9.write(90);
  }
  delay(10); // Delay a little bit to improve simulation performance
}