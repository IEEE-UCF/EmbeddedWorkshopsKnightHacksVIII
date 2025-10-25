#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <SPI.h>
#include "IEEE.h"
#include "KnightHacks.h"

#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  4

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

unsigned long lastSwitch = 0;
unsigned long startRotation = 0;
int phase = 0; // 0–3 = swapping images, 4 = rotation phase
int rotationIndex = 0; // 0–3 for Adafruit rotation states

void showImage(const uint16_t *img) {
  tft.fillScreen(GC9A01A_BLACK);
  tft.drawRGBBitmap(0, 0, img, 240, 240);
}

void setup() {
  tft.begin();
  tft.fillScreen(GC9A01A_BLACK);
  delay(200);
  showImage(IEEE); // start on IEEE logo
  lastSwitch = millis();
}

void loop() {
  unsigned long now = millis();

  // alternating images
  if (phase < 4) {
    if (now - lastSwitch >= 1000) {
      if (phase % 2 == 0) showImage(KnightHacks);
      else showImage(IEEE);

      phase++;
      lastSwitch = now;

      if (phase == 4) { // start rotation after 4 seconds
        startRotation = now;
        rotationIndex = 0;
      }
    }
  }

  // rotation
  else if (phase == 4) {
    if (now - startRotation <= 3000) {
      if (now - lastSwitch >= 250) { // change every 0.25 s
        rotationIndex = (rotationIndex + 1) % 4;
        tft.setRotation(rotationIndex);

        // alternate which logo shows while spinning
        if (rotationIndex % 2 == 0)
          showImage(IEEE);
        else
          showImage(KnightHacks);

        lastSwitch = now;
      }
    } else {
      // end of spin, reset state
      tft.setRotation(0);
      phase = 0;
      lastSwitch = now;
      showImage(IEEE);
    }
  }
}