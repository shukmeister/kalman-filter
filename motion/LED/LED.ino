#include <Adafruit_NeoPixel.h>

#define pixelPin 3

bool LEDINIT = false;
uint16_t HP;
uint16_t shield;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
// NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
// NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
// NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
// NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
// NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

// TODO: change strip to ring object name
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, pixelPin, NEO_GRB + NEO_KHZ800);

int ringLen = strip.numPixels();

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  Serial.begin(9600);
  ledSetup();
}

void loop() {
  if (LEDINIT == false) {
    hpInit();
    shieldInit(8);
    LEDINIT = true;
  } else {

    loseShield(2);
    Serial.println(HP);
    delay(2000);
    loseShield(2);
    Serial.println(HP);
    delay(2000);
    loseShield(2);
    Serial.println(HP);
    delay(2000);
    gainShield(2);
    Serial.println(HP);
    delay(2000);
    gainShield(2);
    Serial.println(HP);
    delay(2000);
    gainShield(2);
    Serial.println(HP);
    delay(2000);
  }

  // // Some example procedures showing how to display to the pixels:
  // colorWipe(strip.Color(255, 0, 0), 50); // Red
  // colorWipe(strip.Color(0, 255, 0), 50); // Green
  // colorWipe(strip.Color(0, 0, 255), 50); // Blue
  // //colorWipe(strip.Color(0, 0, 0, 255), 50); // White RGBW
  // // Send a theater pixel chase in...
  // theaterChase(strip.Color(127, 127, 127), 50); // White
  // theaterChase(strip.Color(127, 0, 0), 50); // Red
  // theaterChase(strip.Color(0, 0, 127), 50); // Blue
}

void ledSetup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(20);
}

void hpInit() {
  for(uint16_t i=1; i <= ringLen; i++) {
    strip.setPixelColor(ringLen - i, strip.Color(0, 255, 0));
    strip.show();
    delay(100);
  }
  HP = ringLen;
}

void shieldInit(uint16_t units) {
  for(uint16_t i=1; i <= units; i++) {
    strip.setPixelColor(ringLen - i, strip.Color(0, 50, 255));
    strip.show();
    delay(100);
  }
  shield = units;
}

// iterate normally instead of inverse
void loseHP(uint16_t units) {
  for (uint8_t i = 0; i < units; i++) {
    strip.setPixelColor(ringLen - HP, strip.Color(0,0,0));
    strip.show();
    delay(10);
    HP --;
  }
}

void gainHP(uint16_t units) {
  for (uint8_t i = 0; i < units; i++) {
    strip.setPixelColor(ringLen - HP - 1, strip.Color(0, 255,0));
    strip.show();
    delay(10);
    HP ++;
  }
}

void loseShield(uint16_t units) {
  for (uint8_t i = 0; i < units; i++) {
    strip.setPixelColor(ringLen - shield, strip.Color(0, 255,0));
    strip.show();
    delay(10);
    shield --;
  }
}

void gainShield(uint16_t units) {
  for (uint8_t i = 0; i < units; i++) {
    strip.setPixelColor(ringLen - shield - 1, strip.Color(0, 50,255));
    strip.show();
    delay(10);
    shield ++;
  }
}

// Adafruit library functions:

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
