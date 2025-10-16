#pragma once
#include <Arduino.h>
#include <M5Unified.h>
#include "accel_m5_core2.h"
#include "stridera_packet.h"

class ImuService {
public:
  void begin();
  void end();
  void reset();
  void update();                                   // refresh current_ from sensor (only used in STREAMING)
  const StrideraAccelPacket& current() const { return current_; }
  uint8_t sampleRateHz() const { return accel_.sample_rate_hz(); }

private:
  AccelM5Core2 accel_;
  StrideraAccelPacket current_{};
  uint32_t lastUi_ = 0;
};
