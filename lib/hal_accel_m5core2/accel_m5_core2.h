#pragma once
#include <M5Unified.h>
#include "iaccelerometer.h"

/**
 * AccelM5Core2 — IAccelerometer implementation for M5Stack Core2 built-in IMU.
 * - Uses M5Unified; no chip-specific code needed (MPU6886/SH200Q abstracted).
 * - Outputs accelerations in milli-g (mg) rounded to int16.
 * - The "rate" is a target for your app loop timing (no HW ODR control here).
 */
class AccelM5Core2 : public IAccelerometer {
  public:
    AccelM5Core2() = default;

    // IAccelerometer
    bool begin() override;
    void read_mg(int16_t& ax_mg, int16_t& ay_mg, int16_t& az_mg) override;
    uint8_t sample_rate_hz() const override { return _rate_hz; }
    // Not part of the interface; provide as a convenience (no override).
    inline void set_sample_rate_hz(uint16_t hz) { _rate_hz = (uint8_t)hz; }

  private:
    static inline int16_t mg_from_g(float g);
    static inline int16_t clamp16(int32_t v) {
      if (v >  32767) return  32767;
      if (v < -32768) return -32768;
      return (int16_t)v;
    }

    uint8_t _rate_hz = 100;  // app loop target (Hz)
};
