#include "System.h"
#include <M5Unified.h>

// ---------- UI helpers ----------
void System::drawHeader() {
  // Small header at top-center
  M5.Display.setTextDatum(textdatum_t::top_center);
  M5.Display.setFont(&fonts::Font2);          // small bitmap font
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.drawString("Stridera IMU BLE Streamer",
                        M5.Display.width() / 2, 4);
}

// Draw one or two centered lines (small font so everything fits)
void System::drawCenteredLines(const char* line1, const char* line2) {
  const int cx = M5.Display.width()  / 2;
  const int cy = M5.Display.height() / 2;

  M5.Display.setTextDatum(textdatum_t::middle_center);
  M5.Display.setFont(&fonts::Font2);          // smaller than Font4
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);

  if (line2 && *line2) {
    const int gap = 6;                         // small gap between lines
    int fh = M5.Display.fontHeight();
    M5.Display.drawString(line1, cx, cy - (fh/2 + gap/2));
    M5.Display.drawString(line2, cx, cy + (fh/2 + gap/2));
  } else {
    M5.Display.drawString(line1, cx, cy);
  }
}

void System::refreshStateBanner() {
  const bool conn = ble_.connected();
  const bool sub  = ble_.subscribed();

  if (state_ == lastDrawnState_ && conn == lastDrawnConn_ && sub == lastDrawnSub_) return;

  M5.Display.fillScreen(TFT_WHITE);
  drawHeader();

  switch (state_) {
    case SystemState::BOOTING:
      drawCenteredLines("BOOTING", "");
      break;

    case SystemState::IDLE:
      if (!conn)           drawCenteredLines("IDLE:", "not connected / not subscribed");
      else if (!sub)       drawCenteredLines("IDLE:", "connected / not subscribed");
      else                 drawCenteredLines("IDLE:", "connected / (unexpected)");
      break;

    case SystemState::STREAMING:
      // two-line banner so 'STREAMING' is always visible
      drawCenteredLines("STREAMING:", "connected / subscribed");
      break;

    case SystemState::SHUTTING_DOWN:
      drawCenteredLines("SHUTTING DOWN", "");
      break;
  }

  lastDrawnState_ = state_;
  lastDrawnConn_  = conn;
  lastDrawnSub_   = sub;
}

// ---------- lifecycle ----------
void System::begin() {
  auto cfg = M5.config();
  cfg.clear_display = true;
  cfg.pmic_button   = true;
  cfg.output_power  = true;
  M5.begin(cfg);                                           // single init (M5Unified)

  // Make sure panel is visible
  M5.Display.wakeup();
  M5.Display.setBrightness(160);

  // Optional: silence speaker "peep" on some Core2 batches
  M5.Speaker.setVolume(0);
  M5.Speaker.end();

  // Button behavior per M5Unified Button Class
  M5.BtnPWR.setDebounceThresh(50);                         // ms
  M5.BtnPWR.setHoldThresh(1500);                           // <- 1.5 s shutdown hold

  // Services
  ble_.begin();
  imu_.begin();

  resetAllRuntimeState();

  boot_ms_ = millis();
  state_   = SystemState::BOOTING;                         // show BOOTING first
  refreshStateBanner();                                    // draw initial banner
  Serial.println("[FSM] -> BOOTING");
}

void System::resetAllRuntimeState() {
  ble_.reset();
  imu_.reset();
  reqStart_ = reqStop_ = reqShutdown_ = false;

  // force next UI draw
  lastDrawnState_ = (SystemState)255;
}

// ---------- main loop ----------
void System::loop() {
  // ---- 1) Poll inputs / intents ----
  const uint32_t now = millis();
  const bool past_boot_grace = (now - boot_ms_) > 2000;    // 2 s grace after boot

  if (past_boot_grace && power_.longPressToggled()) {      // release-after-hold edge
    reqShutdown_ = true;
  }

  if (ble_.shouldStartStreaming()) reqStart_ = true;
  if (ble_.shouldStopStreaming())  reqStop_  = true;

  // ---- 2) State transitions & timed windows ----
  if (state_ == SystemState::BOOTING) {
    // After ~2s, move to IDLE banner automatically
    if ((now - boot_ms_) > 2000) {
      state_ = SystemState::IDLE;
      Serial.println("[FSM] -> IDLE");
    }
    refreshStateBanner();
    delay(10);
    return;
  }

  if (reqShutdown_) {
    reqShutdown_ = false;
    state_ = SystemState::SHUTTING_DOWN;
    shutdown_ms_ = now;                                     // mark start of 2 s window
    refreshStateBanner();                                   // show "SHUTTING DOWN"
    Serial.println("[FSM] -> SHUTTING_DOWN");
  }

  if (reqStart_) {
    reqStart_ = false;
    if (state_ == SystemState::IDLE && ble_.connected()) {
      resetAllRuntimeState();   // new session (clears latched edges; BLE link persists)
      state_ = SystemState::STREAMING;
      Serial.println("[FSM] -> STREAMING");
    }
  }

  if (reqStop_) {
    reqStop_ = false;
    if (state_ == SystemState::STREAMING) {
      ble_.stopNotifications();
      state_ = SystemState::IDLE;
      Serial.println("[FSM] -> IDLE");
    }
  }

  if (!ble_.connected() && state_ == SystemState::STREAMING) {
    state_ = SystemState::IDLE;
    Serial.println("[FSM] disconnected -> IDLE");
  }

  // ---- 3) State actions & UI ----
  switch (state_) {
    case SystemState::IDLE:
      refreshStateBanner();         // will reflect connection/subscription changes
      ble_.poll();
      delay(20);
      break;

    case SystemState::STREAMING:
      refreshStateBanner();         // header + streaming banner stays visible
      imu_.update();
      ble_.sendImu(imu_.current());
      delay(10);                    // ~100 Hz
      break;

    case SystemState::SHUTTING_DOWN:
      refreshStateBanner();         // keep the "SHUTTING DOWN" text on screen
      if ((now - shutdown_ms_) >= 2000) {
        // Orderly stop -> then power off
        ble_.flush();
        ble_.stopNotifications();
        ble_.stopAdvertising();
        imu_.end();
        power_.powerOff();          // usually never returns
        state_ = SystemState::IDLE; // fallback if it ever returns
      }
      delay(10);
      break;

    default: break;
  }
}
