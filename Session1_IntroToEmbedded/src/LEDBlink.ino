#define BLINK_LED 16 // The GPIO Pin that you are connected to, like GPIO 16.

void setup() {
  pinMode(BLINK_LED, OUTPUT); // set the LED pin as output, so we can turn it on and off
}

// okay, so this will use some stuff we didn’t talk about yet.
// ledState keeps track of whether the LED is currently ON or OFF
// lastBlink stores the last time (in ms) the LED changed state

bool ledState = false; // basically “is the LED on?”
unsigned long lastBlink = 0; // how long it’s been since it blinked

void loop() {
  if (millis() - lastBlink >= 500) { // check if 500ms passed since last toggle
    ledState = !ledState; // flip LED state (so ON becomes OFF, OFF becomes ON)
    digitalWrite(BLINK_LED, ledState); // actually apply the new LED state to the pin
    lastBlink = millis(); // record the current time (so the timer resets)
  }
}
