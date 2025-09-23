#pragma once
#include "iaccelerometer.h"
#include <math.h>

class MockAccelerometer : public IAccelerometer {
public:
  bool begin() override { t_ = 0; return true; }
  void read_mg(int16_t& ax, int16_t& ay, int16_t& az) override {
    float x = 200.0f * sinf(t_ * 0.1f);
    float y = 150.0f * sinf(t_ * 0.07f + 1.0f);
    float z = 1000.0f + 50.0f * sinf(t_ * 0.05f + 2.0f);
    ax = (int16_t)x; ay = (int16_t)y; az = (int16_t)z;
    t_ += 1.0f;
  }
  uint8_t sample_rate_hz() const override { return 100; }
private:
  float t_ = 0.0f;
};
