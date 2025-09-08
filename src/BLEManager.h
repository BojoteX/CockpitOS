// BLEManager.h
#pragma once

void BLEManager_send(const uint8_t* data, size_t len);
void BLEManager_dispatchReport(bool force);
void BLE_setup();
void BLE_loop();
