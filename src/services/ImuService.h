#pragma once
#include <Arduino.h>
#include <M5Unified.h>
#include "accel_m5_core2.h"
#include "stridera_packet.h"
#include "CsvReplay.h"

class ImuService {
public:
  void begin();
  void end();
  void reset();
  void update();                                   // refresh current_ packet (live imu or csv replay mode) (only used in STREAMING)
  const StrideraAccelPacket& current() const { return current_; }
  uint8_t sampleRateHz() const { return mode_ == Mode::Replay ? replayRateHz_ : accel_.sample_rate_hz(); }

private:
  enum class Mode { Live, Replay };
  Mode mode_ = Mode::Live;

  AccelM5Core2 accel_;
  StrideraAccelPacket current_{};
  uint32_t lastUi_ = 0;

  // Replay
  CsvReplay player_;
  uint8_t   replayRateHz_ = 100;
  bool      replayReady_ = false;
};
