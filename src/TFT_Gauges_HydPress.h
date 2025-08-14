// TFT_Gauges_HydPress.h
#pragma once

// Gauge
void HydPressureGauge_init();
void HydPressureGauge_loop();
void HydPressureGauge_deinit();
void HydPressureGauge_bitTest();          // Optional: self-test
void HydPressureGauge_notifyMissionStart(); // Call once at every mission start
