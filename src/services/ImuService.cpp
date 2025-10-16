#include "ImuService.h"
#include "config.h"

void ImuService::begin() {
  // M5.begin() should already be called in setup/System::begin()
  accel_.begin();                    // your existing driver
  reset();
}

void ImuService::end() {
  // If you later add explicit power-down for IMU/I2C, put it here.
}

void ImuService::reset() {
  memset(&current_, 0, sizeof(current_));
  lastUi_ = 0;
}

void ImuService::update() {
  int16_t ax, ay, az;
  accel_.read_mg(ax, ay, az);
  current_.ts_ms   = millis();
  current_.ax_mg   = ax;
  current_.ay_mg   = ay;
  current_.az_mg   = az;
  current_.rate_hz = accel_.sample_rate_hz();

  // --- Bottom-of-screen HUD once per second (small, unobtrusive) ---
  if (millis() - lastUi_ > UI_REFRESH_MS) {
    lastUi_ = millis();

    // Use small font and bottom-center datum
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setTextDatum(textdatum_t::bottom_center);

    // Compute a slim bar area at the bottom and clear it
    int fh = M5.Display.fontHeight();
    int y  = M5.Display.height() - 2;          // a couple pixels above the edge
    M5.Display.fillRect(0, M5.Display.height() - (fh + 6),
                        M5.Display.width(), fh + 6, TFT_WHITE);

    // Centered one-line IMU HUD
    char buf[64];
    snprintf(buf, sizeof(buf), "IMU mg: %d %d %d", ax, ay, az);
    M5.Display.drawString(buf, M5.Display.width()/2, y);
  }
}
