// TFT_Gauges.h
#pragma once

// Gauge
void BatteryGauge_init();
void BatteryGauge_loop();
void BatteryGauge_deinit();
void BatteryGauge_bitTest(); // Optional: self-test
void BatteryGauge_notifyMissionStart(); // Call this once at every mission start
