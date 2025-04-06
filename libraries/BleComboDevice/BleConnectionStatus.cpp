#include "BleConnectionStatus.h"

BleConnectionStatus::BleConnectionStatus() {}

void BleConnectionStatus::onConnect(BLEServer* pServer) {
  connected = true;
}

void BleConnectionStatus::onDisconnect(BLEServer* pServer) {
  connected = false;
}