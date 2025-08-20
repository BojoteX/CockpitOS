// TFT_Gauges_RadarAlt.h
#pragma once

// Gauge
void RadarAlt_init();
void RadarAlt_loop();
void RadarAlt_deinit();
void RadarAlt_bitTest();            // Optional: self-test
void RadarAlt_notifyMissionStart(); // Call once at every mission start
