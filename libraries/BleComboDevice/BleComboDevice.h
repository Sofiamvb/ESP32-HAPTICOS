#ifndef ESP32_BLE_COMBO_DEVICE_H
#define ESP32_BLE_COMBO_DEVICE_H

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "BLEHIDDevice.h"
#include "BLECharacteristic.h"
#include "BleConnectionStatus.h"
#include "Print.h"

#define MOUSE_LEFT 1
#define MOUSE_RIGHT 2
#define MOUSE_MIDDLE 4
#define MOUSE_BACK 8
#define MOUSE_FORWARD 16

typedef struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;

class BleComboDevice : public Print {
public:
  BleComboDevice(std::string deviceName = "ESP32 BLE Combo", std::string deviceManufacturer = "Espressif", uint8_t batteryLevel = 100);
  void begin(void);
  void end(void) {}

  bool isConnected(void);
  void setBatteryLevel(uint8_t level);

  size_t press(uint8_t k);
  size_t release(uint8_t k);
  void releaseAll(void);

  void move(signed char x, signed char y, signed char wheel = 0, signed char hWheel = 0);
  void click(uint8_t b = MOUSE_LEFT);
  void pressMouse(uint8_t b = MOUSE_LEFT);
  void releaseMouse(uint8_t b = MOUSE_LEFT);
  bool isMousePressed(uint8_t b = MOUSE_LEFT);

  size_t write(uint8_t c) override;

private:
  static void taskServer(void* pvParameter);
  void sendKeyboardReport(KeyReport* keys);

  BLEHIDDevice* hid;
  BleConnectionStatus* connectionStatus;
  BLECharacteristic* inputKeyboard;
  BLECharacteristic* inputMouse;

  KeyReport _keyReport;
  uint8_t _mouseButtons;
  uint8_t batteryLevel;
  std::string deviceName;
  std::string deviceManufacturer;
};

#endif
#endif