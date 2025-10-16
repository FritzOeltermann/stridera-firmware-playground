#include <Arduino.h>
#include <NimBLEDevice.h>
#include <M5Unified.h>
#include "accel_m5_core2.h"
#include "stridera_packet.h"

// ---- Fixed GATT UUIDs (same as your other demo) ----
static const char* kServiceUUID = "7b9d1f00-8d2a-4b3a-94c1-6b8a1a9b7c10";
static const char* kCharUUID    = "7b9d1f01-8d2a-4b3a-94c1-6b8a1a9b7c10";

// ---- Globals ----
static AccelM5Core2        g_accel;
static NimBLECharacteristic* g_char = nullptr;
static volatile bool       g_connected  = false;
static volatile bool       g_subscribed = false;

// Track subscribe/unsubscribe (CCCD writes)
class AccelChrCallbacks : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic* c, NimBLEConnInfo& info, uint16_t subVal) override {
    const bool notifyOn = (subVal & 0x0001);
    g_subscribed = notifyOn;
    Serial.printf("[BLE] notify=%s (conn=%u)\n",
                  notifyOn ? "on" : "off",
                  info.getConnHandle());
  }
};

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* s, NimBLEConnInfo& info) override {
    g_connected = true;
    Serial.println("[BLE] connected");
  }
  void onDisconnect(NimBLEServer* s, NimBLEConnInfo& info, int reason) override {
    g_connected = false;
    g_subscribed = false; // CCCD resets across connections
    Serial.println("[BLE] disconnected; advertising");
    NimBLEDevice::startAdvertising();
  }
};

static void stream_task(void*) {
  const TickType_t tick = pdMS_TO_TICKS(10); // ~100 Hz
  const uint8_t rate = g_accel.sample_rate_hz();
  uint32_t lastUi = 0;

  for (;;) {
    // Read IMU each tick
    int16_t ax, ay, az;
    g_accel.read_mg(ax, ay, az);
    StrideraAccelPacket pkt{};
    pkt.ts_ms   = millis();
    pkt.ax_mg   = ax;
    pkt.ay_mg   = ay;
    pkt.az_mg   = az;
    pkt.rate_hz = rate;
    pkt.reserved = 0;

    // Always keep the characteristic's stored value updated so "Read" shows real data
    if (g_char) g_char->setValue((uint8_t*)&pkt, sizeof(pkt));

    // Notify only if client subscribed
    if (g_connected && g_subscribed && g_char) {
      g_char->notify((uint8_t*)&pkt, sizeof(pkt));
    }

    // --- Display + Serial: lightweight HUD once per second ---
    if (millis() - lastUi > 1000) {
      lastUi = millis();
      // refresh only the text area (faster than clearing full screen)
      M5.Display.fillRect(0, 40, 320, 80, TFT_WHITE);
      M5.Display.setCursor(0, 40);
      M5.Display.printf("IMU mg: %6d %6d %6d\n", ax, ay, az);
      M5.Display.printf("BLE: %s / %s\n",
        g_connected ? "connected" : "idle",
        g_subscribed ? "notifying" : "no sub");
      Serial.printf("IMU mg: %6d %6d %6d | BLE: %s / %s\n",
        ax, ay, az,
        g_connected ? "connected" : "idle",
        g_subscribed ? "notifying" : "no sub");
    }

    vTaskDelay(tick);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[BOOT] Core2 IMU + NimBLE streamer");

  // 1) Init IMU HAL
  if (!g_accel.begin()) {
    Serial.println("ERROR: IMU begin failed");
    for(;;) delay(1000);
  }
  g_accel.set_sample_rate_hz(100);

  // --- Display: init + boot banner ---
  // (AccelM5Core2::begin() calls M5.begin(); calling it again is harmless,
  // but if you prefer, remove this if your HAL already did M5.begin().)
  M5.begin();  // ensures display is up
  M5.Display.setFont(&fonts::Font4);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.fillScreen(TFT_WHITE);
  M5.Display.setCursor(0, 0);
  M5.Display.println("Stridera Core2 IMU BLE");
  M5.Display.println("Waiting for BLE client...");

  // 2) Init NimBLE (device + server)
  NimBLEDevice::init("Stridera-Core2");
  NimBLEDevice::setDeviceName("Stridera-Core2");
#ifdef STRIDERA_BLE_MTU
  NimBLEDevice::setMTU(STRIDERA_BLE_MTU);
#endif
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  // 3) GATT: 1 Service, 1 Characteristic (READ | NOTIFY)
  NimBLEService* svc = server->createService(kServiceUUID);

  g_char = svc->createCharacteristic(
    kCharUUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  g_char->setCallbacks(new AccelChrCallbacks());

  // Seed value so a pre-subscription "Read" returns a valid 12-byte frame
  StrideraAccelPacket zero{};
  g_char->setValue((uint8_t*)&zero, sizeof(zero));

  svc->start();

  // 4) Advertising: general discoverable, include service UUID + name
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  NimBLEAdvertisementData advData, srData;
  advData.setFlags(0x06);              // LE General Discoverable, BR/EDR not supported
  advData.addServiceUUID(kServiceUUID);
  advData.setName("Stridera-Core2");
  adv->setAdvertisementData(advData);
  srData.setName("Stridera-Core2");
  adv->setScanResponseData(srData);
  adv->start(0); // advertise forever
  Serial.println("[BLE] Advertising started");

  // 5) Start the 100 Hz streamer task on core 1
  xTaskCreatePinnedToCore(stream_task, "imu_ble_stream", 4096, nullptr, 2, nullptr, 1);
}

void loop() {
  // Nothing here; task does the work. Keep a small sleep to yield.
  delay(1000);
}
