#pragma once
#include <stdint.h>

#pragma pack(push, 1)
struct StrideraAccelPacket {
  uint32_t ts_ms;   // ms since boot
  int16_t  ax_mg;   // milli-g
  int16_t  ay_mg;
  int16_t  az_mg;
  uint8_t  rate_hz; // nominal sample rate
  uint8_t  reserved;
};
#pragma pack(pop)

static_assert(sizeof(StrideraAccelPacket) == 12, "Unexpected packet size");
