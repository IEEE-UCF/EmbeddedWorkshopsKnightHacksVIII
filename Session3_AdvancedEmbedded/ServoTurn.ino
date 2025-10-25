#include <Arduino.h>

#define SERVO_PIN 32 // GPIO32 for servo signal
#define PWM_FREQ 50 // 50Hz signal for servo (20ms period)
#define PWM_RES 16 // 16-bit resolution for precise duty

void setup() {
  ledcAttach(SERVO_PIN, PWM_FREQ, PWM_RES); // attach pin with freq and resolution
}

void loop() {
  setServoPulse(1000); // 0 degrees
  delay(500);
  
  setServoPulse(1250); // 45 degrees
  delay(500);
  
  setServoPulse(1500); // 90 degrees
  delay(500);
  
  setServoPulse(1750); // 135 degrees
  delay(500);
  
  setServoPulse(2000); // 180 degrees
  delay(500);
  
  setServoPulse(1750); // 135 degrees (going back)
  delay(500);
  
  setServoPulse(1500); // 90 degrees
  delay(500);
  
  setServoPulse(1250); // 45 degrees
  delay(500);
}

void setServoPulse(int pulseWidth) {
  int duty = (int)((pulseWidth / 20000.0) * 65535); // convert pulse to 16-bit duty
  ledcWrite(SERVO_PIN, duty); // write duty cycle directly to pin
}
