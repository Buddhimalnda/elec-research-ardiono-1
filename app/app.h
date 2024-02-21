#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <Wire.h>

// SD Card
#include <SD.h>
#include <SPI.h>

// Firebase
// #include <FirebaseESP32.h>
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>
// Bluetooth
// #include <BluetoothSerial.h>