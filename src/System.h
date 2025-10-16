#pragma once
#include <Arduino.h>
#include <M5Unified.h>
#include "services/BleService.h"
#include "services/ImuService.h"
#include "services/PowerService.h"

enum class SystemState { BOOTING, IDLE, STREAMING, SHUTTING_DOWN };

class System {
public:
  void begin();   // one-time bring-up (board + services) and land in IDLE
  void loop();    // poll inputs, run FSM, and perform state actions

private:
  void setState(SystemState s) { state_ = s; }
  void resetAllRuntimeState();  // clears volatile runtime state across services

  // --- UI helpers ---
  // --- UI helpers (new) ---
  void drawHeader();                                 // small title at top
  void drawCenteredLines(const char* line1,
                         const char* line2 = "");    // 1–2 centered lines
  void refreshStateBanner();                         // redraw only when needed

  // transition requests (latched; consumed in loop)
  bool reqStart_    = false;
  bool reqStop_     = false;
  bool reqShutdown_ = false;

  // --- state bookkeeping
  SystemState state_ = SystemState::BOOTING;
  uint32_t boot_ms_ = 0;
  uint32_t shutdown_ms_ = 0;

  // last-rendered flags to avoid flicker
  SystemState lastDrawnState_ = (SystemState)255;
  bool lastDrawnConn_ = false;
  bool lastDrawnSub_  = false;

  BleService   ble_;
  ImuService   imu_;
  PowerService power_;
};
