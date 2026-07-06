


# Senior Design Project

## **Laser Imaging Detection and Ranging <br> (LiDAR) Based Proximity Alert <br> for Bicycle Helmet**

### Part 1: Planning

---

An important part of any project is the planning stage, this project is no exception. <br> Below is the table in the report to show when each part of the project was completed.
![](https://github.com/VancouverSatelite/Senior-Design/blob/main/Pictures/schedule.png)
<br>
Now, you may notice some missing lines, try to ignore that.

This project was done as a group, but I did the 3D modeling, the Wiring Diagram, and general part oversight.
<br> I was the main developer, however my groupmates helped me a lot with testing, and with putting together presentation materials.

---

### Part 2: Initial Design

---

Keeping to schedule is a key part of project productivity, however just scheduling isn't going ot do all of the work.

The first step of designing this project was to understand what exactly we were supposed to be making.

When I showed up to class and was told, <br>"You're doing the bicycle helmet thing!" <br> It wasn't super helpful, so I sought out help from my professor, asking him to clarify what exactly it was that our group was supposed to be making.

It turned out to be **"A bicycle helmet mounted device that improved cyclist safety."**

So naturally I was immediately trying to brainstorm how I would go about this.

A common way automobiles go about keeping distance from others is by using a parallel camera system (as depicted below), so maybe this would work, but you know what I've always wanted to work with? LiDAR, which is close enough to that


---
![](https://github.com/VancouverSatelite/Senior-Design/blob/main/Pictures/volvo.jpg) Source: [Volvo UK](https://www.volvocars.com/uk/support/car/xc40/article/af42dd367f7db52ec0a80151108cf1cd/)

---
<br>
So I got on that immediately, and started looking around, ending up on the TF-MINIs LiDAR sensor, as it works with arduinos.
<br>
Ah yes, Arduinos, the classic low-power low level microcontrollers that are super versatile. Which one should we use?
 
 Originally, the plan was to use an Arduino Nano, as it's the smallest package you can get that amount of capability in, but unfortunately while trying to get the nano off of a breadboard, I completely biffed it and knocked a resistor off the bottom of it.... Whoops.

 This led to us using an Arduino Mega 2650 (from Elegoo) for the majority of the time we were developing the actual device.

 ---

### Part 3: Testing 

---

### Part 4: The final Design

---

![The Wiring diagram of the project, which I threw together in KICAD](https://github.com/VancouverSatelite/Senior-Design/blob/main/Pictures/FinalDesign.png)
  [Rear Casing](https://sketchfab.com/3d-models/rear-casing-311979b84c3f495db25b09bcab5221ab)  <br>
  ![A spinning 3D model which is the design of the rear case portion of the device.](https://github.com/VancouverSatelite/Senior-Design/blob/main/Pictures/trim-rear.gif)<br>

  [Front Casing](https://sketchfab.com/3d-models/front-panel-c0035471094749f2b10e251a77223106)<br>
  
  ![A spinning 3D model which is the design of the front case portion of the device.](https://github.com/VancouverSatelite/Senior-Design/blob/main/Pictures/trimmed-front.gif)

  ---

### Part 5: Code

---

  If you are interested in the code used, it was a permutation of this:
  
  ```
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
```