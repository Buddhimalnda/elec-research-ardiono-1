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

#define SD_CS 15
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18

// Firebase
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData fbdo;
FirebaseData userData;
// Network and Firebase credentials
String ssid = "SLT-4G_163BEA";
String password = "751FCEED";

#define FIREBASE_HOST "https://elec-research-0-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "AIzaSyCvhDMEJ9gnl8lqyV282-8pdHOVmZqyVPs"
String USER_EMAIL = "device@gmail.com";
String USER_PASSWORD = "123456";
const String DEVICE_ID = "10001";
String USER_ID = "ltMvLzncQ6et1HDFmrDffP4r8NE3";
// SD Card
File userFile;
void setup()
{
  Serial.begin(115200);
  connectToWiFi();
  initializeFirebase();
  if (initializeSDCard())
  {
    Serial.println("initializeSDCard");

    if (checkAndCreateDeviceJson())
    {
      String jsonString = readDeviceJson();
      JsonDocument json;
      deserializeJson(json, jsonString);
      Serial.println(jsonString);
    }
    else
    {
      Serial.println("non");
    }
  }
}
void loop()
{
  // put your main code here, to run repeatedly:
  // Serial.println("loop");
}

void connectToWiFi()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void initializeFirebase()
{
  Serial.println("Initializing Firebase");
  // Firebase.begin("FIREBASE_HOST", "FIREBASE_AUTH");
  config.api_key = FIREBASE_AUTH;
  config.database_url = FIREBASE_HOST;
  fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  // Or use legacy authenticate method
  // config.database_url = DATABASE_URL;
  // config.signer.tokens.legacy_token = "<database secret>";
  /* Assign the api key (required) */
  config.api_key = FIREBASE_AUTH;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);
  Firebase.setDoubleDigits(5);
}
bool initializeSDCard()
{
  Serial.println("Initializing SD Card");
  if (!SD.begin(SD_CS))
  {
    Serial.println("SD Card initialization failed"); // SD Card initialization failed
    return false;
  }
  Serial.println("SD Card initialization successful");
  return true;
}

bool checkAndCreateDeviceJson()
{
  if (!SD.exists("/device.txt"))
  {
    // Device.json doesn't exist, fetch data from Firebase
    if (!fetchDataFromFirebase())
    {
      return false; // Failed to fetch data
    }
    else
    {
      Serial.println("failtto");
    }
  }
  else
  {
    Serial.println("device.json already exists");
  }
  return true; // File exists or was successfully created
}

bool fetchDataFromFirebase()
{
  String doc = "";
  if (Firebase.RTDB.getString(&fbdo, "/device/" + DEVICE_ID + "/user_id"))
  {
    if (fbdo.dataType() == "string")
    {
      FirebaseJson firebaseJson;
      Serial.println(fbdo.stringData());
      USER_ID = fbdo.stringData();
      FirebaseJsonData result;
      if (Firebase.RTDB.getJSON(&userData, "/" + USER_ID + "/user", &firebaseJson))
      {
        if (userData.dataType() == "json")
        {
          firebaseJson.get(result, "/");
          if (result.success)
          {
            doc = result.stringValue;
            if (writeDeviceJson(doc))
            {
              return true;
            }
            else
            {
              Serial.println("Writting error");
              return false;
            }
          }
        }
        else
        {
          Serial.println("Invalid data type");
          return false;
        }
      }
      else
      {
        Serial.println(userData.errorReason());
        return false;
      }
      return true;
    }
    else
    {
      Serial.println("Invalid data type");
      return false;
    }
  }
  else
  {
    Serial.println(fbdo.errorReason());
    return false;
  }
  return true;
}

String readDeviceJson()
{ // Adjust size as needed
  userFile = SD.open("/device.txt", FILE_READ);
  if (!userFile)
  {
    Serial.println("Failed to open device.json for reading");
    return "";
    // Return empty document in case of failure
  }
  doc += userFile.read();
  Serial.println(doc);
  userFile.close();
  return doc;
}

bool writeDeviceJson(const String doc)
{
  userFile = SD.open("/device.txt", FILE_WRITE);
  if (!userFile)
  {
    Serial.println("Failed to open device.txt for writing");
    return false;
  }
  userFile.print(doc);
  userFile.close();
  return true;
}

// delete file
void deleteFile(const char *path)
{
  Serial.printf("Deleting file: %s\n", path);
  if (SD.exists(path))
  {
    SD.remove(path);
    Serial.println("File deleted");
  }
  else
  {
    Serial.println("File doesn't exist");
  }
}

// DynamicJsonDocument toJson(String jsonString)
// {
//   DynamicJsonDocument docs(1024);
//   DeserializationError error = deserializeJson(docs, jsonString);

//   // Test if parsing succeeds
//   if (error)
//   {
//     Serial.print(F("deserializeJson() failed: "));
//     Serial.println(error.c_str());
//   }
//   return docs;
// }