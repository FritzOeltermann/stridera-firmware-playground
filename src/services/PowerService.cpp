#include "PowerService.h"

// Detect a *release after hold* to avoid "instant reboot" when PMU sees key still down.
// See M5Unified Button_Class: pressedFor / wasReleased require M5.update() every loop.
bool PowerService::longPressToggled() {
  M5.update();  // refresh BtnPWR state (required by M5Unified Button API)

  // 1) Latch when the button has been held long enough
  if (!holdSeen_ && M5.BtnPWR.pressedFor(kLongPressMs)) {
    holdSeen_ = true;
  }

  // 2) Emit a single edge on *release after a valid hold*
  if (holdSeen_ && M5.BtnPWR.wasReleasedAfterHold()) {
    holdSeen_ = false;
    return true;    // one-shot
  }

  return false;
}

void PowerService::powerOff() {
  // UX: blank and sleep the panel so the user sees a proper shutdown
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.sleep();

  // Small guard so the PMU definitely sees the key released
  // and to let USB CDC flush if connected to a host.
  Serial.flush();
  delay(150);

  // Official power cut for PMIC-equipped devices (Core2). 
  // Per M5 docs, true power-off requires running on battery.
  M5.Power.powerOff();

  // If powerOff() returns (edge cases), park the CPU.
  while (true) { delay(1000); }
}
