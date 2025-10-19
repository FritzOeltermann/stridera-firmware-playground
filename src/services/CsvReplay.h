#pragma once
#include <Arduino.h>
#include <SD.h>
#include "stridera_packet.h"

class CsvReplay {
public:
  bool begin(const char* path) {
    if (!SD.begin(4)) {   // Core2 SD-CS is typically GPIO 4
      return false;
    }
    file_ = SD.open(path, FILE_READ);
    if (!file_) return false;

    // Skip header line
    String header = file_.readStringUntil('\n');
    (void)header;
    return true;
  }

  bool readNext(StrideraAccelPacket& out, uint8_t nominal_rate_hz = 100) {
    if (!file_) return false;

    String line = file_.readStringUntil('\n');
    if (line.length() == 0) {
      // end-of-file -> loop from start (skip header)
      file_.close();
      return begin(path_.c_str());
    }

    // Very tolerant CSV parsing (tabs or commas)
    float gx=0, gy=0, gz=0;
    // We only need the last three numeric fields in the expected order.
    // Split once, then scan from right to left for the 3 floats.
    int lastComma = line.lastIndexOf(',');
    if (lastComma < 0) lastComma = line.lastIndexOf('\t');
    if (lastComma < 0) return false;

    String z_str = line.substring(lastComma + 1);       // z (g)
    String rest  = line.substring(0, lastComma);

    lastComma = rest.lastIndexOf(',');
    if (lastComma < 0) lastComma = rest.lastIndexOf('\t');
    if (lastComma < 0) return false;

    String y_str = rest.substring(lastComma + 1);       // y (g)
    String rest2 = rest.substring(0, lastComma);

    lastComma = rest2.lastIndexOf(',');
    if (lastComma < 0) lastComma = rest2.lastIndexOf('\t');
    if (lastComma < 0) return false;

    String x_str = rest2.substring(lastComma + 1);      // x (g)

    gx = x_str.toFloat();
    gy = y_str.toFloat();
    gz = z_str.toFloat();

    // Fill packet (mg + monotonic millis)
    out.ts_ms   = millis();
    out.ax_mg   = (int16_t)roundf(gx * 1000.0f);
    out.ay_mg   = (int16_t)roundf(gy * 1000.0f);
    out.az_mg   = (int16_t)roundf(gz * 1000.0f);
    out.rate_hz = nominal_rate_hz;
    out.reserved = 0;
    return true;
  }

  void setPath(const char* p) { path_ = p; }

private:
  File file_;
  String path_ = "/snapchat.csv";
};
