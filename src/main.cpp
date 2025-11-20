#include <Arduino.h>
#include <TFT_eSPI.h>

// Simple Snake game for ESP32 + ILI9341 2.8" display (TPM408-2.8 assumed)
// Controls: touchscreen (XPT2046). If you wired buttons, adapt accordingly.

// Touch controller CS pin (must match `TOUCH_CS` in `User_Setup.h`)
// Board label: TP CS = IO33
#define TOUCH_CS 33

TFT_eSPI tft = TFT_eSPI();

const int SCREEN_W = 240;
const int SCREEN_H = 320;

const int CELL = 10; // cell size in pixels
const int COLS = SCREEN_W / CELL;
const int ROWS = SCREEN_H / CELL;

struct Point { int x, y; };

#include <deque>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>

// Touch calibration values (default). Will be overwritten by saved calibration.
int TS_MINX = 200;
int TS_MAXX = 3900;
int TS_MINY = 200;
int TS_MAXY = 3900;

XPT2046_Touchscreen ts(TOUCH_CS);
Preferences prefs;
std::deque<Point> snake;
Point food;
int dirX = 1, dirY = 0; // moving right initially
unsigned long lastMove = 0;
int speedMs = 150;
bool gameOver = false;

void placeFood() {
  while (true) {
    int fx = random(0, COLS);
    int fy = random(0, ROWS);
    bool onSnake = false;
    for (auto &p : snake) if (p.x == fx && p.y == fy) { onSnake = true; break; }
    if (!onSnake) { food.x = fx; food.y = fy; break; }
  }
}

void drawCell(int x, int y, uint16_t color) {
  tft.fillRect(x * CELL, y * CELL, CELL, CELL, color);
}

void resetGame() {
  snake.clear();
  snake.push_back({COLS/2, ROWS/2});
  snake.push_back({COLS/2-1, ROWS/2});
  snake.push_back({COLS/2-2, ROWS/2});
  dirX = 1; dirY = 0;
  placeFood();
  gameOver = false;
  tft.fillScreen(TFT_BLACK);
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));
  tft.init();
  tft.setRotation(1); // landscape
  tft.fillScreen(TFT_BLACK);

  // initialize touchscreen
  SPI.begin();
  ts.begin();

  // load calibration from NVS if present
  prefs.begin("touch", true);
  if (prefs.isKey("cal_done")) {
    TS_MINX = prefs.getInt("minx", TS_MINX);
    TS_MAXX = prefs.getInt("maxx", TS_MAXX);
    TS_MINY = prefs.getInt("miny", TS_MINY);
    TS_MAXY = prefs.getInt("maxy", TS_MAXY);
    Serial.println("Loaded touch calibration from NVS");
  }
  prefs.end();

  // if no calibration, run calibration routine
  prefs.begin("touch", false);
  bool needCal = !prefs.isKey("cal_done");
  prefs.end();
  if (needCal) {
    // run calibration (blocking)
    Serial.println("Running touch calibration...");
    delay(500);
    // calibrate and store
    prefs.begin("touch", false);
    // leave setup screen clear for calibration function
    // calibration function will set prefs
    prefs.end();
    // small delay to allow screen to update
    delay(200);
    calibrateTouch();
  }

  resetGame();
}

// Helper: draw a crosshair at (x,y)
void drawCross(int x, int y, uint16_t color) {
  int s = 8;
  tft.drawLine(x - s, y, x + s, y, color);
  tft.drawLine(x, y - s, x, y + s, color);
}

// Calibration routine: tap four points (TL, TR, BR, BL)
void calibrateTouch() {
  prefs.begin("touch", false);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("Touch calibration");
  delay(300);

  int cornersX[4] = {20, SCREEN_W - 20, SCREEN_W - 20, 20};
  int cornersY[4] = {20, 20, SCREEN_H - 20, SCREEN_H - 20};
  int rawX[4];
  int rawY[4];

  for (int i = 0; i < 4; ++i) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 10);
    tft.print("Tap the crosshair");
    drawCross(cornersX[i], cornersY[i], TFT_RED);
    // wait for touch
    while (!ts.touched()) { delay(10); }
    TS_Point p = ts.getPoint();
    rawX[i] = p.x;
    rawY[i] = p.y;
    Serial.printf("corner %d raw: %d,%d\n", i, p.x, p.y);
    // wait for release
    delay(150);
    while (ts.touched()) { delay(10); }
    delay(200);
  }

  // compute min/max from collected corner samples
  int minx = min(min(rawX[0], rawX[3]), min(rawX[1], rawX[2]));
  int maxx = max(max(rawX[0], rawX[3]), max(rawX[1], rawX[2]));
  int miny = min(min(rawY[0], rawY[1]), min(rawY[2], rawY[3]));
  int maxy = max(max(rawY[0], rawY[1]), max(rawY[2], rawY[3]));

  TS_MINX = minx; TS_MAXX = maxx; TS_MINY = miny; TS_MAXY = maxy;

  prefs.putInt("minx", TS_MINX);
  prefs.putInt("maxx", TS_MAXX);
  prefs.putInt("miny", TS_MINY);
  prefs.putInt("maxy", TS_MAXY);
  prefs.putBool("cal_done", true);
  prefs.end();

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("Calibration saved.");
  delay(800);
}

void readControls() {
  if (!ts.touched()) return;
  TS_Point p = ts.getPoint();
  // map raw touch to screen coords
  int x = map(p.x, TS_MINX, TS_MAXX, 0, SCREEN_W - 1);
  int y = map(p.y, TS_MINY, TS_MAXY, 0, SCREEN_H - 1);

  int cx = SCREEN_W / 2;
  int cy = SCREEN_H / 2;
  int dx = x - cx;
  int dy = y - cy;

  if (abs(dx) > abs(dy)) {
    if (dx < 0 && dirX == 0) { dirX = -1; dirY = 0; }
    else if (dx > 0 && dirX == 0) { dirX = 1; dirY = 0; }
  } else {
    if (dy < 0 && dirY == 0) { dirX = 0; dirY = -1; }
    else if (dy > 0 && dirY == 0) { dirX = 0; dirY = 1; }
  }

  // simple debounce / wait for release
  delay(80);
  while (ts.touched()) { delay(10); }
}

void loop() {
  if (gameOver) {
    // Show message and wait for any touch to restart
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, SCREEN_H/2 - 10);
    tft.print("Game Over. Press any");
    tft.setCursor(10, SCREEN_H/2 + 10);
    tft.print("button to restart");
    if (ts.touched()) {
      resetGame();
      delay(300);
    }
    delay(50);
    return;
  }

  readControls();

  unsigned long now = millis();
  if (now - lastMove < (unsigned long)speedMs) return;
  lastMove = now;

  // compute new head
  Point head = snake.front();
  Point nh = { head.x + dirX, head.y + dirY };

  // wrap-around (optional) or collision
  if (nh.x < 0) nh.x = COLS - 1;
  if (nh.x >= COLS) nh.x = 0;
  if (nh.y < 0) nh.y = ROWS - 1;
  if (nh.y >= ROWS) nh.y = 0;

  // check collision with self
  for (auto &p : snake) if (p.x == nh.x && p.y == nh.y) { gameOver = true; return; }

  // draw head
  snake.push_front(nh);
  drawCell(nh.x, nh.y, TFT_GREEN);

  // ate food?
  if (nh.x == food.x && nh.y == food.y) {
    placeFood();
    drawCell(food.x, food.y, TFT_RED);
  } else {
    // remove tail
    Point tail = snake.back();
    drawCell(tail.x, tail.y, TFT_BLACK);
    snake.pop_back();
  }

  // draw food (in case placed)
  drawCell(food.x, food.y, TFT_RED);
}
