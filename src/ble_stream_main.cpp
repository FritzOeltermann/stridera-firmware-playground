#include <Arduino.h>
#include "stridera_ble.h"
#include "mock_accelerometer.h"
#include "stridera_packet.h"

using namespace stridera;

BleServerModule g_ble;
MockAccelerometer g_accel;

TaskHandle_t g_streamTaskHandle = nullptr;

static void stream_task(void*) {
  const TickType_t delayTicks = pdMS_TO_TICKS(10); // ~100 Hz
  const uint8_t rate = g_accel.sample_rate_hz();

  for (;;) {
    if (g_ble.isConnected() && g_ble.hasSubscribers()) {
      StrideraAccelPacket pkt{};
      pkt.ts_ms = (uint32_t)millis();
      int16_t ax, ay, az;
      g_accel.read_mg(ax, ay, az);
      pkt.ax_mg = ax; pkt.ay_mg = ay; pkt.az_mg = az;
      pkt.rate_hz = rate;
      g_ble.notifyAccel(pkt);

      static uint32_t lastLog = 0;
      if (millis() - lastLog > 1000) {
        lastLog = millis();
        Serial.printf("mg: %d, %d, %d\n", ax, ay, az);
      }
    }
    vTaskDelay(delayTicks);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("Stridera BLE Accelerometer firmware");

  if (!g_accel.begin()) {
    Serial.println("Accel init failed");
  }

  g_ble.setAccelerometer(&g_accel);
  if (!g_ble.begin()) {
    Serial.println("BLE init failed");
    while (true) { delay(1000); }
  }

  xTaskCreatePinnedToCore(stream_task, "stream", 4096, nullptr, 1, &g_streamTaskHandle, 1);
}

void loop() {
  delay(1000);
}
