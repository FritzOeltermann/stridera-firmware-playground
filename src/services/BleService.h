#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include "stridera_packet.h"
#include "config.h"

class BleService {
public:
  void begin();
  void end();
  void reset();                                  // clear runtime flags/queues

  // Event edges (latched until read; System should poll and clear them)
  bool shouldStartStreaming();                   // true once when client subscribes
  bool shouldStopStreaming();                    // true once when client unsubscribes or disconnects

  // Runtime status
  bool connected() const { return connected_; }
  bool subscribed() const { return subscribed_; }

  // Operations
  void poll();                                   // keep alive if needed
  void startAdvertising();
  void stopAdvertising();
  void stopNotifications();
  void flush();
  void sendImu(const StrideraAccelPacket& pkt);  // notify if subscribed

private:
  NimBLEServer*        server_ = nullptr;
  NimBLEService*       service_ = nullptr;
  NimBLECharacteristic* chr_   = nullptr;

  volatile bool connected_  = false;
  volatile bool subscribed_ = false;

  // Latched edge flags
  volatile bool ev_start_ = false;
  volatile bool ev_stop_  = false;

  bool advConfigured_ = false; 

  friend class _BleServerCallbacks;
  friend class _BleCharCallbacks;
};
