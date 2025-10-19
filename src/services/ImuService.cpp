#include "ImuService.h"
#include "config.h"

void ImuService::begin() {
  // Try to detect a CSV file; if found -> Replay; else -> Live
  // We optimistically try to start replay here; if it fails we fall back to live.
  if (!replayReady_) {
    // Lazy init: try the default path
    player_.setPath("/snapchat.csv");
    replayReady_ = player_.begin("/snapchat.csv");
    if (replayReady_) {
      mode_ = Mode::Replay;
    }
  }

  if (mode_ == Mode::Live) {
    accel_.begin();    
  }
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
  if (mode_ == Mode::Replay && replayReady_) {
    // Advance one CSV row per call; stream at firmware loop's pace
    StrideraAccelPacket pkt;
    if (player_.readNext(pkt, replayRateHz_)) {
      current_ = pkt;
    }
  } else {
    int16_t ax, ay, az;
    accel_.read_mg(ax, ay, az);
    current_.ts_ms   = millis();
    current_.ax_mg   = ax;
    current_.ay_mg   = ay;
    current_.az_mg   = az;
    current_.rate_hz = accel_.sample_rate_hz();
  }

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
    if (mode_ == Mode::Replay) {
      snprintf(buf, sizeof(buf), "REPLAY mg: %d %d %d @%uHz",
               current_.ax_mg, current_.ay_mg, current_.az_mg, sampleRateHz());
    } else {
      snprintf(buf, sizeof(buf), "IMU mg: %d %d %d", current_.ax_mg, current_.ay_mg, current_.az_mg);
    }
    M5.Display.drawString(buf, M5.Display.width()/2, y);
  }
}
