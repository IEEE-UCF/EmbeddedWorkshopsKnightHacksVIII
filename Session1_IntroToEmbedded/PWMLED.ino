#include <Arduino.h>

#define BLINK_LED 16 // GPIO16 (RXD 2)
#define PWM_LED 17 // GPIO17 (TXD 2)
#define POT_PIN 4 // GPIO_4

bool ledState = false; // tracks LED on/off state
unsigned long lastBlink = 0; // for blink timing

void setup() {
  Serial.begin(115200); // start serial monitor for debugging
  
  pinMode(BLINK_LED, OUTPUT); // setup blink LED as output
  
  ledcAttach(PWM_LED, 5000, 8); // attach PWM LED at 5kHz with 8-bit resolution
}

void loop() {
  int potValue = analogRead(POT_PIN); // read potentiometer (0–4095)
  int brightness = map(potValue, 0, 4095, 0, 255); // map to 8-bit range
  
  ledcWrite(PWM_LED, brightness); // update PWM LED brightness
  
  if (millis() - lastBlink >= 500) { // check if 500ms passed
    ledState = !ledState; // toggle LED state
    digitalWrite(BLINK_LED, ledState); // update blink LED
    lastBlink = millis(); // reset blink timer
  }
}