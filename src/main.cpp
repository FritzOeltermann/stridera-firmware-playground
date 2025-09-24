#include <M5Unified.h>
#include <NimBLEDevice.h>

// ==== GATT UUIDs (random but fixed) ====
static const char* kServiceUUID = "7b9d1f00-8d2a-4b3a-94c1-6b8a1a9b7c10";
static const char* kCharUUID    = "7b9d1f01-8d2a-4b3a-94c1-6b8a1a9b7c10";

// ==== On-air packet (12 bytes) ====
#pragma pack(push, 1)
struct AccelPkt {
  uint32_t ts_ms;  // millis since boot
  int16_t  ax_mg;
  int16_t  ay_mg;
  int16_t  az_mg;
  uint8_t  rate_hz;   // nominal loop rate
  uint8_t  reserved;
};
#pragma pack(pop)
static_assert(sizeof(AccelPkt) == 12, "AccelPkt must be 12 bytes");

// ==== Globals ====
NimBLEServer*         g_server = nullptr;
NimBLECharacteristic* g_char   = nullptr;
NimBLEDescriptor*     g_cccd   = nullptr;  // Client Characteristic Configuration (0x2902)

volatile bool g_connected  = false;
volatile bool g_subscribed = false;

static inline int16_t g_to_mg(float g) {
  float mg = g * 1000.0f;
  if (mg > 32767)  mg = 32767;
  if (mg < -32768) mg = -32768;
  return (int16_t)mg;
}

// Send one packet immediately (helpful when notifications just got enabled)
static void send_now_if_possible() {
  if (!g_char || !g_connected || !g_subscribed) return;
  auto d = M5.Imu.getImuData();
  AccelPkt pkt{};
  pkt.ts_ms   = (uint32_t)millis();
  pkt.ax_mg   = g_to_mg(d.accel.x);
  pkt.ay_mg   = g_to_mg(d.accel.y);
  pkt.az_mg   = g_to_mg(d.accel.z);
  pkt.rate_hz = 50;
  g_char->setValue((uint8_t*)&pkt, sizeof(pkt));
  g_char->notify();
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // Simple UI
  M5.Display.setRotation(1);
  M5.Display.fillScreen(TFT_WHITE);
  M5.Display.setFont(&fonts::Font4);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.println("Core2 BLE IMU");

  // Initialize IMU
  if (!M5.Imu.begin()) {
    M5.Display.setCursor(0, 40);
    M5.Display.println("IMU init failed");
  }

  // ==== BLE init ====
  NimBLEDevice::init("Stridera-Core2");
  NimBLEDevice::setDeviceName("Stridera-Core2");
#ifdef STRIDERA_BLE_MTU
  NimBLEDevice::setMTU(STRIDERA_BLE_MTU);
#endif
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  g_server = NimBLEDevice::createServer();

  NimBLEService* svc = g_server->createService(kServiceUUID);

  // Characteristic: READ | NOTIFY (no indicate)
  g_char = svc->createCharacteristic(
    kCharUUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );

  // CCCD (0x2902): explicitly create and default to "notifications off"
  g_cccd = g_char->createDescriptor(NimBLEUUID((uint16_t)0x2902));
  if (g_cccd) {
    g_cccd->setValue(std::string("\x00\x00", 2));  // [notify=0, indicate=0]
    // No setPermissions() here — not available in this NimBLE build.
  }

  // Seed a readable value so "Read" works even before subscribing
  AccelPkt zero{};
  g_char->setValue((uint8_t*)&zero, sizeof(zero));

  svc->start();

  // ---- Advertising ----
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();

  NimBLEAdvertisementData advData;
  advData.setFlags(0x06);                 // LE General Discoverable + BR/EDR not supported
  advData.addServiceUUID(kServiceUUID);
  advData.setName("Stridera-Core2");      // name in ADV
  adv->setAdvertisementData(advData);

  NimBLEAdvertisementData srData;         // name again in scan response
  srData.setName("Stridera-Core2");
  adv->setScanResponseData(srData);

  adv->start(0);                          // 0 = advertise forever
  M5_LOGI("Advertising started");
}

void loop() {
  // ----- POLL CONNECTION STATE -----
  bool wasConnected = g_connected;
  g_connected = (g_server && g_server->getConnectedCount() > 0);

  if (g_connected != wasConnected) {
    M5_LOGI("Conn state -> %s", g_connected ? "CONNECTED" : "DISCONNECTED");
  }

  // Auto-restart advertising when not connected (robust reconnects)
  if (!g_connected) {
    auto* adv = NimBLEDevice::getAdvertising();
    if (adv && !adv->isAdvertising()) {
      adv->start(0);
      M5_LOGI("Advertising (re)started");
    }
  }

  // ----- POLL SUBSCRIPTION STATE (read CCCD 0x2902) -----
  bool wasSubscribed = g_subscribed;
  if (g_cccd) {
    // CCCD: bit0=Notify, bit1=Indicate (we only care about bit0)
    std::string v = g_cccd->getValue();
    g_subscribed  = (v.size() >= 2) && (static_cast<uint8_t>(v[0]) & 0x01);
  } else {
    g_subscribed = false;
  }

  if (g_subscribed && !wasSubscribed) {
    M5_LOGI("Subscription ENABLED");
    send_now_if_possible();
  } else if (!g_subscribed && wasSubscribed) {
    M5_LOGI("Subscription DISABLED");
  }

  // ----- IMU/UI + Streaming -----
  if (M5.Imu.update()) {
    auto d = M5.Imu.getImuData();

    // Draw to screen
    M5.Display.fillRect(0, 40, M5.Display.width(), M5.Display.height()-40, TFT_WHITE);
    M5.Display.setCursor(0, 44);
    M5.Display.printf("ax:%6.2f ay:%6.2f az:%6.2f\n", d.accel.x, d.accel.y, d.accel.z);
    M5.Display.printf("gx:%6.2f gy:%6.2f gz:%6.2f\n", d.gyro.x,  d.gyro.y,  d.gyro.z);

    // Show BLE state on-screen
    M5.Display.setCursor(0, 100);
    M5.Display.printf("BLE: %s / %s\n",
                      g_connected ? "conn" : "disc",
                      g_subscribed ? "subs" : "no-subs");

    // Notify when possible
    static uint32_t s_tx_count = 0;
    if (g_connected && g_subscribed && g_char) {
      AccelPkt pkt{};
      pkt.ts_ms   = (uint32_t)millis();
      pkt.ax_mg   = g_to_mg(d.accel.x);
      pkt.ay_mg   = g_to_mg(d.accel.y);
      pkt.az_mg   = g_to_mg(d.accel.z);
      pkt.rate_hz = 50; // approx.

      g_char->setValue((uint8_t*)&pkt, sizeof(pkt));
      g_char->notify();
      s_tx_count++;
      M5_LOGI("notify #%lu", (unsigned long)s_tx_count);

      M5.Display.setCursor(0, 120);
      M5.Display.printf("TX: %lu\n", (unsigned long)s_tx_count);
    }
  }

  delay(50); // ~20 Hz UI refresh cadence
}
