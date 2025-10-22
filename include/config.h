#pragma once

// ===== BLE UUIDs (kept same as your current firmware) =====
#define STRIDERA_SERVICE_UUID "7b9d1f00-8d2a-4b3a-94c1-6b8a1a9b7c10"
#define STRIDERA_CHAR_UUID    "7b9d1f01-8d2a-4b3a-94c1-6b8a1a9b7c10"

// ===== App identity =====
#ifndef STRIDERA_DEVICE_NAME
  #define STRIDERA_DEVICE_NAME "Stridera-Unknown"
#endif

// ===== Tasking =====
#define IMU_TASK_STACK   4096
#define IMU_TASK_PRIO    2
#define IMU_TASK_CORE    1

// ===== Power / input (we’ll wire deep sleep later) =====
#define POWER_LONG_PRESS_MS 1500

// ===== UI =====
#define UI_REFRESH_MS 1000
