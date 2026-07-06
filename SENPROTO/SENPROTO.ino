#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>

// ---------------- CONFIG ----------------
#define LED_PIN 2
#define BUZZER_PIN 3
#define NUM_LEDS 64

#define ROWS 8
#define COLS 8

#define MIN_DIST 50
#define MAX_DIST 600   // LEDs on at 6 meters

// TFmini serial pins
#define TF_RX 10
#define TF_TX 11

SoftwareSerial tfSerial(TF_RX, TF_TX);

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ---------------- STATE ----------------
uint16_t distance = 0;
float smoothDist = 0;

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  tfSerial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);

  strip.begin();
  strip.setBrightness(40);
  strip.show();

  Serial.println("Radar system (6m visual / 4m audio)");
}

// ---------------- INDEX HELPER ----------------
int getIndex(int row, int col) {
  if (row % 2 == 0) {
    return row * COLS + col;
  } else {
    return row * COLS + (COLS - 1 - col);
  }
}

// ---------------- READ TFMINI ----------------
bool readDistance(uint16_t &dist) {
  static uint8_t buf[9];
  static uint8_t idx = 0;

  while (tfSerial.available()) {
    uint8_t b = tfSerial.read();

    if (idx == 0 && b != 0x59) continue;
    if (idx == 1 && b != 0x59) {
      idx = 0;
      continue;
    }

    buf[idx++] = b;

    if (idx == 9) {
      idx = 0;

      uint16_t sum = 0;
      for (int i = 0; i < 8; i++) sum += buf[i];
      if ((sum & 0xFF) != buf[8]) return false;

      uint16_t d = buf[2] + (buf[3] << 8);
      if (d == 0 || d > 1200) return false;

      dist = d;
      return true;
    }
  }
  return false;
}

// ---------------- DISTANCE → RATIO ----------------
float getRatio(uint16_t d) {
  if (d < MIN_DIST) d = MIN_DIST;
  if (d > MAX_DIST) d = MAX_DIST;

  return 1.0 - ((float)(d - MIN_DIST) / (MAX_DIST - MIN_DIST));
}

// ---------------- COLOR ----------------
uint32_t getColor(uint16_t d, float r) {
  if (d <= 200) return strip.Color(255, 0, 0);

  if (r < 0.5) {
    return strip.Color(r * 2 * 255, 255, 0);
  } else {
    return strip.Color(255, (1.0 - (r - 0.5) * 2) * 255, 0);
  }
}

// ---------------- RADAR DISPLAY ----------------
void updateDisplay(uint16_t d) {
  float r = getRatio(d);
  int rowsToLight = r * ROWS;
  uint32_t color = getColor(d, r);

  int center = ROWS / 2;

  strip.clear();

  for (int i = 0; i < rowsToLight; i++) {

    int rowUp = center + (i / 2);
    int rowDown = center - 1 - (i / 2);
    int targetRow = (i % 2 == 0) ? rowDown : rowUp;

    if (targetRow >= 0 && targetRow < ROWS) {
      for (int col = 0; col < COLS; col++) {
        strip.setPixelColor(getIndex(targetRow, col), color);
      }
    }
  }

  strip.show();
}

// ---------------- BUZZER (4m start) ----------------
void updateBuzzer(uint16_t d) {
  static unsigned long lastBeep = 0;
  static bool beepState = false;

  //  further 4m
  if (d > 400) {
    noTone(BUZZER_PIN);
    return;
  }

  //  solid beep within 2m
  if (d <= 200) {
    tone(BUZZER_PIN, 1200);
    return;
  }

  int interval = map(d, 200, 400, 50, 500);

  if (millis() - lastBeep > interval) {
    beepState = !beepState;

    if (beepState) tone(BUZZER_PIN, 900);
    else noTone(BUZZER_PIN);

    lastBeep = millis();
  }
}

// ---------------- LOOP ----------------
void loop() {

  if (readDistance(distance)) {

    smoothDist = smoothDist * 0.7 + distance * 0.3;
    uint16_t d = (uint16_t)smoothDist;

    updateDisplay(d);
    updateBuzzer(d);

    Serial.print("Raw: ");
    Serial.print(distance);
    Serial.print("  Smooth: ");
    Serial.println(d);
  }
}