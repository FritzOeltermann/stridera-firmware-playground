#include "accel_m5_core2.h"
#include <math.h>  // lrintf

bool AccelM5Core2::begin() {
  M5.begin();
  return true;
}

void AccelM5Core2::read_mg(int16_t& ax_mg, int16_t& ay_mg, int16_t& az_mg) {
  float ax_g, ay_g, az_g;
  if (!M5.Imu.getAccel(&ax_g, &ay_g, &az_g)) {
    ax_mg = ay_mg = az_mg = 0;
    return;
  }
  ax_mg = mg_from_g(ax_g);
  ay_mg = mg_from_g(ay_g);
  az_mg = mg_from_g(az_g);
  return;
}

int16_t AccelM5Core2::mg_from_g(float g) {
  // Round to nearest mg, clamp into int16 range
  const int32_t mg = (int32_t)lrintf(g * 1000.0f);
  return clamp16(mg);
}
