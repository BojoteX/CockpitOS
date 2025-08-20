// TFT_Gauges_BrakePress.h
#pragma once

// Gauge
void BrakePressureGauge_init();
void BrakePressureGauge_loop();
void BrakePressureGauge_deinit();
void BrakePressureGauge_bitTest(); // Optional: self-test
void BrakePressureGauge_notifyMissionStart(); // Call this once at every mission start
