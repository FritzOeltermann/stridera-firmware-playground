#pragma once
#include <NimBLEDevice.h>
#include "stridera_packet.h"
#include "iaccelerometer.h"

namespace stridera {

// UUIDs fixed for Stridera telemetry
static const char* kServiceTelemetry = "7b9d1f00-8d2a-4b3a-94c1-6b8a1a9b7c10";
static const char* kCharAccelNotify  = "7b9d1f01-8d2a-4b3a-94c1-6b8a1a9b7c10";

class BleServerModule : public NimBLEServerCallbacks {
public:
  void setAccelerometer(IAccelerometer* accel) { accel_ = accel; }

  // Init BLE stack, create GATT, start advertising
  bool begin();

  // Link state / subscription state
  bool isConnected() const { return connected_; }
  bool hasSubscribers() const;

  // Push a 12-byte accel packet to subscribers
  void notifyAccel(const StrideraAccelPacket& pkt);

  // Callbacks — no 'override' to keep compatible with multiple NimBLE versions
  void onConnect(NimBLEServer* s);
  void onDisconnect(NimBLEServer* s);

private:
  NimBLEServer*        server_   = nullptr;
  NimBLEService*       svc_      = nullptr;
  NimBLECharacteristic* chr_accel_ = nullptr;
  volatile bool        connected_ = false;
  IAccelerometer*      accel_     = nullptr; // not owned
};

} // namespace stridera
