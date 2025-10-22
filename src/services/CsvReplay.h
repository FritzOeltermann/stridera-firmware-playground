// CsvReplay.h
#pragma once
#include <Arduino.h>
#include "stridera_packet.h"

#if STRIDERA_HAS_SD
  #include <SD.h>
#else
  #include <SPIFFS.h>
#endif

class CsvReplay {
public:
  bool begin(const char* path) {
  #if STRIDERA_HAS_SD
    // Core2: built-in SD (CS is typically GPIO 4)
    if (!SD.begin(4)) return false;  // Core2 default CS = 4
    file_ = SD.open(path, FILE_READ);
  #else
    // StickC Plus 2: no SD — use SPIFFS
    if (!SPIFFS.begin(true)) return false;
    if (!SPIFFS.exists(path)) return false;         // quick existence check
    file_ = SPIFFS.open(path, "r");
  #endif
    if (!file_) return false;

    // Skip header
    (void)file_.readStringUntil('\n');
    return true;
  }

  bool readNext(StrideraAccelPacket& out, uint8_t nominal_rate_hz = 100) {
    String line = file_.readStringUntil('\n');
    if (line.length() == 0) return false;

    // CSV: ts_ms, ax_g, ay_g, az_g
    char* endp = nullptr;
    const char* s = line.c_str();
    uint32_t ts = strtoul(s, &endp, 10);
    if (!endp) return false;

    float ax = strtof(endp+1, &endp);
    float ay = strtof(endp+1, &endp);
    float az = strtof(endp+1, &endp);

    out.ts_ms   = ts;
    out.ax_mg   = (int16_t)lrintf(ax * 1000.0f);
    out.ay_mg   = (int16_t)lrintf(ay * 1000.0f);
    out.az_mg   = (int16_t)lrintf(az * 1000.0f);
    out.rate_hz = nominal_rate_hz;
    out.reserved= 0;
    return true;
  }

  void setPath(const char* p) { path_ = p; }

private:
#if STRIDERA_HAS_SD
  File file_;
#else
  fs::File file_;
#endif
  String path_ = "/snapchat.csv";
};
