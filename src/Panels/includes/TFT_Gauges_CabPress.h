// TFT_Gauges_CabPress.h
#pragma once

// Gauge
void CabinPressureGauge_init();
void CabinPressureGauge_loop();
void CabinPressureGauge_deinit();
void CabinPressureGauge_bitTest(); // Optional: self-test
void CabinPressureGauge_notifyMissionStart(); // Call this once at every mission start
