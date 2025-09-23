#include "stridera_ble.h"
#include <Arduino.h>

namespace stridera {

bool BleServerModule::begin() {
  NimBLEDevice::init("Stridera-Tracker");
#ifdef STRIDERA_BLE_MTU
  NimBLEDevice::setMTU(STRIDERA_BLE_MTU);
#endif
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  server_ = NimBLEDevice::createServer();
  server_->setCallbacks(this);

  svc_ = server_->createService(kServiceTelemetry);

  chr_accel_ = svc_->createCharacteristic(
      kCharAccelNotify,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );

  StrideraAccelPacket zero{};
  chr_accel_->setValue((uint8_t*)&zero, sizeof(zero));

  svc_->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(kServiceTelemetry);
  adv->start();

  return (server_ && svc_ && chr_accel_);
}

void BleServerModule::onConnect(NimBLEServer* s) {
  connected_ = true;
  Serial.println("[BLE] Central connected");
}

void BleServerModule::onDisconnect(NimBLEServer* s) {
  connected_ = false;
  Serial.println("[BLE] Central disconnected");
  NimBLEDevice::startAdvertising();
}

bool BleServerModule::hasSubscribers() const {
  if (!chr_accel_) return false;
  return chr_accel_->getSubscribedCount() > 0;
}

void BleServerModule::notifyAccel(const StrideraAccelPacket& pkt) {
  if (!chr_accel_) return;
  chr_accel_->setValue((uint8_t*)&pkt, sizeof(pkt));
  chr_accel_->notify();
}

} // namespace stridera
