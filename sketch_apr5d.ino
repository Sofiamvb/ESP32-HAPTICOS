#include <Wire.h>
#include <MPU6050_tockn.h>
#include "BleComboDevice.h"

#define KEY_BACKSPACE 0x2C
#define MOUSE_LEFT 0x01

MPU6050 mpu(Wire);
BleComboDevice bleCombo("PistolaMaravilla", "EquipoMaravilla", 100);

const int motorpin = 13;
unsigned long motorStartTime = 0;
bool motorOn = false;
const int joyX = 34;
const int joyY = 35;
const int btnJoy = 32;
const int botonDisparo = 26;
const int botonBandera = 25;
const int botonPistola = 27;

void setup() {


  Serial.begin(115200);
  Wire.begin(21, 22);
  mpu.begin();
  mpu.calcGyroOffsets();

  pinMode (motorpin,OUTPUT);
  pinMode(botonDisparo, INPUT_PULLUP);
  pinMode(botonBandera, INPUT_PULLUP);
  pinMode(botonPistola, INPUT_PULLUP);
  pinMode(btnJoy, INPUT_PULLUP);

  bleCombo.begin();
  Serial.println("BLE Combo iniciado");
}

void loop() {
  if (!bleCombo.isConnected()) {
    Serial.println("[BLE] Aún no conectado a host...");
    delay(500);
    return;
  }else{
    Serial.println("Haptico conectado");
  }

  mpu.update();
  float gx = mpu.getGyroX();
  float gy = mpu.getGyroY();

  static float prev_gx = 0, prev_gy = 0;
  float alpha = 0.8;
  gx = alpha * prev_gx + (1 - alpha) * gx;
  gy = alpha * prev_gy + (1 - alpha) * gy;
  prev_gx = gx;
  prev_gy = gy;

  float deadZone = 1.0;
  int movX = (abs(gx) > deadZone) ? (int)(gx * -2.5) : 0;
  int movY = (abs(gy) > deadZone) ? (int)(gy * 2.5) : 0;
  if (movX != 0 || movY != 0) {
  Serial.print("Mouse move → X: ");
  Serial.print(movX);
  Serial.print(" | Y: ");
  Serial.println(movY);

  bleCombo.move(movX, movY);
}

  if (digitalRead(botonDisparo) == LOW && !motorOn) {
    Serial.println("Boton presionado");
    bleCombo.pressMouse(MOUSE_LEFT);
    digitalWrite(motorpin, HIGH);
    motorStartTime = millis();
    motorOn = true;
  }

  if (motorOn && millis() - motorStartTime >= 300) { // 300ms de vibración
    digitalWrite(motorpin, LOW);
    motorOn = false;
    bleCombo.releaseMouse(MOUSE_LEFT);
  }

  int xValue = analogRead(joyX);
  int yValue = analogRead(joyY);
  bool buttonPressed = digitalRead(btnJoy) == LOW;
  const int deadzone = 500;

  if (xValue < 2048 - deadzone) {
  Serial.println(" Joystick: A PRESSED");
  bleCombo.press(0x04);
} else if (xValue > 2048 + deadzone) {
  Serial.println("Joystick: D PRESSED");
  bleCombo.press(0x07);
} else {
  bleCombo.release(0x04);
  bleCombo.release(0x07);
}

if (yValue < 2048 - deadzone) {
  Serial.println(" Joystick: W PRESSED");
  bleCombo.press(0x1A);
} else if (yValue > 2048 + deadzone) {
  Serial.println(" Joystick: S PRESSED");
  bleCombo.press(0x16);
} else {
  bleCombo.release(0x1A);
  bleCombo.release(0x16);
}

  if (buttonPressed) {
  Serial.println("Botón Joystick: SPACE PRESSED");
  bleCombo.press(KEY_BACKSPACE);
} else {
  bleCombo.release(KEY_BACKSPACE);
}
  if (digitalRead(botonBandera) == LOW) bleCombo.press(0x09);
  else bleCombo.release(0x09);

  if (digitalRead(botonPistola) == LOW) bleCombo.press(0x1E);
  else bleCombo.release(0x1E);

  delay(25);
}

