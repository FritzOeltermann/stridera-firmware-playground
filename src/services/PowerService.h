#pragma once
#include <Arduino.h>
#include <M5Unified.h>

// PowerService.h
class PowerService {
public:
  bool longPressToggled();   // now: returns true once on RELEASE after a long press
  void powerOff();
private:
  static constexpr uint32_t kLongPressMs = 2000;
  bool holdSeen_ = false;    // internal latch: we saw a long press, waiting for release
};

