#ifndef BLE_CONNECTION_STATUS_H
#define BLE_CONNECTION_STATUS_H

#include <BLEServer.h>
#include <BLECharacteristic.h>

class BleConnectionStatus : public BLEServerCallbacks {
public:
  BleConnectionStatus();
  bool connected = false;
  BLECharacteristic* inputKeyboard = nullptr;
  BLECharacteristic* inputMouse = nullptr;
  void onConnect(BLEServer* pServer);
  void onDisconnect(BLEServer* pServer);
};

#endif