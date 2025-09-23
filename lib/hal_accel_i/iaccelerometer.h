#pragma once
#include <stdint.h>

class IAccelerometer {
public:
  virtual bool begin() = 0;
  virtual void read_mg(int16_t& ax, int16_t& ay, int16_t& az) = 0;
  virtual uint8_t sample_rate_hz() const = 0;
};
