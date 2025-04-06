#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLEHIDDevice.h>
#include "sdkconfig.h"
#include "HIDTypes.h"
#include "BleConnectionStatus.h"
#include "BleComboDevice.h"

#define KEYBOARD_ID 0x01
#define MOUSE_ID    0x02

static const uint8_t _hidReportDescriptor[] = {
  // Keyboard
  USAGE_PAGE(1), 0x01,
  USAGE(1), 0x06,
  COLLECTION(1), 0x01,
    REPORT_ID(1), KEYBOARD_ID,
    USAGE_PAGE(1), 0x07,
    USAGE_MINIMUM(1), 0xE0,
    USAGE_MAXIMUM(1), 0xE7,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1), 0x01,
    REPORT_COUNT(1), 0x08,
    HIDINPUT(1), 0x02,
    REPORT_COUNT(1), 0x01,
    REPORT_SIZE(1), 0x08,
    HIDINPUT(1), 0x01,
    REPORT_COUNT(1), 0x06,
    REPORT_SIZE(1), 0x08,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x65,
    USAGE_MINIMUM(1), 0x00,
    USAGE_MAXIMUM(1), 0x65,
    HIDINPUT(1), 0x00,
  END_COLLECTION(0),

  // Mouse
  USAGE_PAGE(1), 0x01,
  USAGE(1), 0x02,
  COLLECTION(1), 0x01,
    REPORT_ID(1), MOUSE_ID,
    USAGE(1), 0x01,
    COLLECTION(1), 0x00,
      USAGE_PAGE(1), 0x09,
      USAGE_MINIMUM(1), 0x01,
      USAGE_MAXIMUM(1), 0x03,
      LOGICAL_MINIMUM(1), 0x00,
      LOGICAL_MAXIMUM(1), 0x01,
      REPORT_COUNT(1), 0x03,
      REPORT_SIZE(1), 0x01,
      HIDINPUT(1), 0x02,
      REPORT_COUNT(1), 0x01,
      REPORT_SIZE(1), 0x05,
      HIDINPUT(1), 0x01,
      USAGE_PAGE(1), 0x01,
      USAGE(1), 0x30,
      USAGE(1), 0x31,
      LOGICAL_MINIMUM(1), 0x81,
      LOGICAL_MAXIMUM(1), 0x7f,
      REPORT_SIZE(1), 0x08,
      REPORT_COUNT(1), 0x02,
      HIDINPUT(1), 0x06,
    END_COLLECTION(0),
  END_COLLECTION(0)
};

BleComboDevice::BleComboDevice(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel)
{
  this->deviceName = deviceName;
  this->deviceManufacturer = deviceManufacturer;
  this->batteryLevel = batteryLevel;
  this->connectionStatus = new BleConnectionStatus();
  this->_mouseButtons = 0;
}

void BleComboDevice::begin(void)
{
  xTaskCreate(this->taskServer, "server", 20000, (void *)this, 5, NULL);
}

void BleComboDevice::taskServer(void* pvParameter)
{
  BleComboDevice* dev = (BleComboDevice *) pvParameter;
  BLEDevice::init(String(dev->deviceName.c_str()));
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(dev->connectionStatus);

  dev->hid = new BLEHIDDevice(pServer);
  dev->inputKeyboard = dev->hid->inputReport(KEYBOARD_ID);
  dev->inputMouse = dev->hid->inputReport(MOUSE_ID);

  dev->connectionStatus->inputKeyboard = dev->inputKeyboard;
  dev->connectionStatus->inputMouse = dev->inputMouse;

  dev->hid->manufacturer()->setValue(String(dev->deviceManufacturer.c_str()));
  dev->hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  dev->hid->hidInfo(0x00, 0x01);

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

  dev->hid->reportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
  dev->hid->startServices();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_KEYBOARD);
  pAdvertising->addServiceUUID(dev->hid->hidService()->getUUID());
  pAdvertising->start();

  dev->hid->setBatteryLevel(dev->batteryLevel);

  vTaskDelay(portMAX_DELAY);
}

bool BleComboDevice::isConnected(void)
{
  return this->connectionStatus->connected;
}

void BleComboDevice::setBatteryLevel(uint8_t level)
{
  this->batteryLevel = level;
  if (hid)
    this->hid->setBatteryLevel(this->batteryLevel);
}

// =====================
// KEYBOARD
// =====================

size_t BleComboDevice::press(uint8_t k)
{
  if (k >= 128) {
    _keyReport.modifiers |= (1 << (k - 128));
    k = 0;
  }

  for (int i = 0; i < 6; i++) {
    if (_keyReport.keys[i] == 0x00) {
      _keyReport.keys[i] = k;
      break;
    }
  }

  sendKeyboardReport(&_keyReport);
  return 1;
}

size_t BleComboDevice::release(uint8_t k)
{
  if (k >= 128) {
    _keyReport.modifiers &= ~(1 << (k - 128));
    k = 0;
  }

  for (int i = 0; i < 6; i++) {
    if (_keyReport.keys[i] == k) {
      _keyReport.keys[i] = 0x00;
    }
  }

  sendKeyboardReport(&_keyReport);
  return 1;
}

void BleComboDevice::releaseAll(void)
{
  memset(&_keyReport, 0, sizeof(KeyReport));
  sendKeyboardReport(&_keyReport);
}

void BleComboDevice::sendKeyboardReport(KeyReport* keys)
{
  if (isConnected()) {
    inputKeyboard->setValue((uint8_t*)keys, sizeof(KeyReport));
    inputKeyboard->notify();
  }
}

size_t BleComboDevice::write(uint8_t c)
{
  press(c);
  release(c);
  return 1;
}

// =====================
// MOUSE
// =====================

void BleComboDevice::move(signed char x, signed char y, signed char wheel, signed char hWheel)
{
  if (!isConnected()) return;

  uint8_t report[5];
  report[0] = _mouseButtons;
  report[1] = x;
  report[2] = y;
  report[3] = wheel;
  report[4] = hWheel;

  inputMouse->setValue(report, sizeof(report));
  inputMouse->notify();
}

void BleComboDevice::click(uint8_t b)
{
  _mouseButtons = b;
  move(0, 0);
  _mouseButtons = 0;
  move(0, 0);
}

void BleComboDevice::pressMouse(uint8_t b)
{
  _mouseButtons |= b;
  move(0, 0);
}

void BleComboDevice::releaseMouse(uint8_t b)
{
  _mouseButtons &= ~b;
  move(0, 0);
}

bool BleComboDevice::isMousePressed(uint8_t b)
{
  return (_mouseButtons & b) > 0;
}