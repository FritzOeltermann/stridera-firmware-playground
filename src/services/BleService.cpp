#include "BleService.h"

class _BleServerCallbacks : public NimBLEServerCallbacks {
public:
  _BleServerCallbacks(BleService* p): owner(p) {}
  void onConnect(NimBLEServer* s, NimBLEConnInfo& c) override {
    owner->connected_ = true;
    Serial.printf("[BLE] onConnect: %s\n", c.getAddress().toString().c_str());
  }

  void onDisconnect(NimBLEServer* s, NimBLEConnInfo& c, int reason) override {
    owner->connected_ = false;
    owner->subscribed_ = false;
    owner->ev_stop_ = true;                      // if streaming, System will transition to IDLE
    Serial.printf("[BLE] onDisconnect: reason=0x%02X (%d) peer=%s\n",
                  reason, reason, c.getAddress().toString().c_str());
    owner->startAdvertising();
  }
private:
  BleService* owner;
};

class _BleCharCallbacks : public NimBLECharacteristicCallbacks {
public:
  _BleCharCallbacks(BleService* p): owner(p) {}
  void onSubscribe(NimBLECharacteristic*, NimBLEConnInfo& info, uint16_t subVal) override {
    const bool notifyOn = (subVal & 0x0001);
    if (notifyOn) {
      owner->subscribed_ = true;
      owner->ev_start_   = true;
    } else {
      owner->subscribed_ = false;
      owner->ev_stop_    = true;
    }
    Serial.printf("[BLE] notify=%s (conn=%u)\n", notifyOn ? "on" : "off", info.getConnHandle());
  }
private:
  BleService* owner;
};

void BleService::begin() {
  NimBLEDevice::init(STRIDERA_DEVICE_NAME);
  NimBLEDevice::setMTU(247);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setSecurityAuth(false, false, false);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  server_ = NimBLEDevice::createServer();
  server_->setCallbacks(new _BleServerCallbacks(this));

  service_ = server_->createService(STRIDERA_SERVICE_UUID);
  chr_ = service_->createCharacteristic(
      STRIDERA_CHAR_UUID,
      NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
  );
  chr_->setCallbacks(new _BleCharCallbacks(this));

  service_->start();
  startAdvertising();
}

void BleService::end() {
  stopAdvertising();
  if (server_) server_->removeService(service_);
  server_ = nullptr;
  service_ = nullptr;
  chr_ = nullptr;
}

void BleService::reset() {
  // clear only volatile/runtime flags (not GATT)
  subscribed_ = false;
  ev_start_ = false;
  ev_stop_ = false;
}

bool BleService::shouldStartStreaming() {
  if (ev_start_) { ev_start_ = false; return true; }
  return false;
}
bool BleService::shouldStopStreaming() {
  if (ev_stop_)  { ev_stop_  = false; return true; }
  return false;
}

void BleService::poll() {
  // nothing needed currently
}

void BleService::startAdvertising() {
  auto adv = NimBLEDevice::getAdvertising();

  if (!advConfigured_) {
    NimBLEAdvertisementData advData, srData;

    // Flags: General Discoverable + BR/EDR not supported
    advData.setFlags(0x06);

    // Primary service in the advertising packet
    advData.setCompleteServices(NimBLEUUID(STRIDERA_SERVICE_UUID));

    // Device name in scan response
    srData.setName(STRIDERA_DEVICE_NAME);

    adv->setAdvertisementData(advData);
    adv->setScanResponseData(srData);

    advConfigured_ = true;
  }

  if (!adv->isAdvertising()) {
    adv->start(0); // advertise indefinitely
    Serial.println("[BLE] Advertising started");
  } else {
    Serial.println("[BLE] Advertising already running");
  }
}

void BleService::stopAdvertising() {
  NimBLEDevice::stopAdvertising();
}

void BleService::stopNotifications() {
  subscribed_ = false;
}

void BleService::flush() {
  // add ring buffer flush later if you buffer packets
}

void BleService::sendImu(const StrideraAccelPacket& pkt) {
  if (!connected_ || !subscribed_ || !chr_) return;
  chr_->setValue((uint8_t*)&pkt, sizeof(pkt));
  chr_->notify((uint8_t*)&pkt, sizeof(pkt));
}
