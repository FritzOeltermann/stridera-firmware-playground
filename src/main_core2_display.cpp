#include <M5Unified.h>

void setup(void) {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setTextFont(&fonts::Font4);
    M5.Display.setTextSize(1);
}

void loop(void) {
    auto imu_update = M5.Imu.update();
    if (imu_update) {
        M5.Display.setCursor(0, 24);
        M5.Display.clear(TFT_WHITE);

        auto ImuData = M5.Imu.getImuData();

        // The ImuData obtained by getImuData can be used as follows.
        ImuData.accel.x;      // accel x-axis value.
        ImuData.accel.y;      // accel y-axis value.
        ImuData.accel.z;      // accel z-axis value.
        ImuData.accel.value;  // accel 3values array [0]=x / [1]=y / [2]=z.

        ImuData.gyro.x;      // gyro x-axis value.
        ImuData.gyro.y;      // gyro y-axis value.
        ImuData.gyro.z;      // gyro z-axis value.
        ImuData.gyro.value;  // gyro 3values array [0]=x / [1]=y / [2]=z.

        M5.Display.setTextColor(TFT_BLACK);
        M5.Display.printf("IMU:\r\n");
        M5.Display.printf("ax:%6.2f  ay:%6.2f  az:%6.2f\r\n", ImuData.accel.x, ImuData.accel.y, ImuData.accel.z);
        M5.Display.printf("gx:%6.2f  gy:%6.2f  gz:%6.2f\r\n", ImuData.gyro.x, ImuData.gyro.y, ImuData.gyro.z);
    }
    delay(500);
}            