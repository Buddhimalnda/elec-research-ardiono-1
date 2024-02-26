#include <Arduino.h>
#include <ArduinoJson.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include <WebServer.h>
#include <EEPROM.h>
#include <WiFi.h>

// Firebase
// #include <FirebaseESP32.h>
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>
// RTC
#include <ThreeWire.h>
#include <RtcDS1302.h>

// SD Card
#include <SD.h>
#include <SPI.h>
// Bluetooth
// #include "BluetoothSerial.h"

// // Check if BluetoothSerial library is compiled
// #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
// #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
// #endif

// MPU
#define MPU_SDA 21
#define MPU_SCL 22
// RTC
#define RTC_CLK 5
#define RTC_RST 2
#define RTC_IO 4
// SD Card
#define SD_CS 15
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18
// LED & Switch
#define SW2_PIN 35     // wifi switch
#define SW3_PIN 32     // Mode Switch
#define SW4_PIN_SOS 34 // SOS Switch

#define DATA_PIN 33
#define LATCH_PIN 26
#define CLOCK_PIN 27
#define NUM_LEDS 8

#define RGB_R 14
#define RGB_G 12
#define RGB_B 13
#define OUT_LED 25

#define BTRY_V_IN 36

// now time
RtcDateTime now;
ThreeWire myWire(RTC_IO, RTC_CLK, RTC_RST); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);
bool stateOfWIFI = true;

// firebase
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData fbdo;
// MPU
Adafruit_MPU6050 mpu;
float accelX, accelY, accelZ;

float accMagnitudePrev = 0;

unsigned long lastStepTime = 0;
int stepCount = 0;
float stepThreshold = 1.2; // Experimentally determined threshold for step detection
float debounceTime = 250;  // Minimum time between steps (milliseconds)
int lastIndexInt = 0;

// Network and Firebase credentials
String ssid = "SLT-4G_163BEA";
String password = "751FCEED";

#define FIREBASE_HOST "https://elec-research-0-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "AIzaSyCvhDMEJ9gnl8lqyV282-8pdHOVmZqyVPs"
String USER_EMAIL = "malinda@gmail.com";
String USER_PASSWORD = "123456";
const String DEVICE_ID = "10001";
String USER_ID = "jw6atlW1Vgg1xW7umYk6Z3AdlBp2";

// web server
WebServer server(80);      // Object to hold the HTTP server
String logDataServer = ""; // String to store log data

// Bluetooth
// BluetoothSerial ESP_BT;
// LED & Switch
int stateOfMode = 0;
bool stateOfSOS = false;
int red = 0;
int green = 0;
int blue = 0;
bool stateOfSW2 = false;
bool stateOfSW3 = false;
bool stateOfSW4 = false;

bool stateOfFirebase = false;
bool deviceState = true; // ON/OFF state of the device
//-------------------------
// errors
bool error = false;
int errorCode = 0;
uint8_t patterns[] = {
    0b00011000, // Middle LEDs
    0b00111100, // Middle to outer LEDs
    0b01111110, // All except first and last
    0b11111111, // All LEDs on
    0b01111110, // All except first and last
    0b00111100, // Middle to outer LEDs
    0b00011000  // Middle LEDs
};
//-------------------------
void setup()
{
    Serial.begin(115200);
    deviceState = true;
    connectToWiFi();
    initializeFirebase();
    initializeMPU6050();
    initializeRTC();
    initializeSDCard();
    findDeviceFile();
    // bluetoothConfig();
    initializeSwitches();
    initializeIndicationLEDs();
    initializeRGBLEDs();
    Serial.println(WiFi.localIP());
    server.on("/", HTTP_GET, []()
              {
                  server.send(200, "text/html", getHtmlPage()); // Send web page with log data
              });

    server.begin();
}
void loop()
{
    // readBLE();
    // delay(100);
    // Serial.println("deviceState : " + String(deviceState));
    // if (deviceState)
    // {
    server.handleClient(); // Handle client requests
    listningSwitch();
    detectStep();
    displayStepCount();

    if (readRGBFromFirebse() && stateOfMode == 0 && !stateOfSOS)
    {
        rgb(red, green, blue);
    }
    else
    {
        rgbLighting(stateOfMode - 1);
    }
    delay(10);
}
void listningSwitch()
{
    if (digitalRead(SW2_PIN) == HIGH)
    {
        if (stateOfSW2)
        {
            stateOfSW2 = false;
        }
        else
        {
            Serial.println("Active SW2_PIN");
            stateOfSW2 = true;
        }
    }
    if (digitalRead(SW3_PIN) == HIGH)
    {
        Serial.println("Mode change" + String(stateOfMode));
        if (stateOfMode >= 6)
        {
            stateOfMode = 0;
        }
        else
        {
            stateOfMode++;
        }
    }
    else
    {
        stateOfSW3 = false;
    }
    if (digitalRead(SW4_PIN_SOS) == HIGH)
    {
        stateOfSW4 = true;
        if (!stateOfSOS)
        {
            stateOfSOS = true;
            changeStateOfSOS();
        }
        else
        {
            stateOfSOS = false;
            changeStateOfSOS();
        }
    }
    else
    {
        stateOfSW4 = false;
    }
    delay(100);
}

void initializeSwitches()
{
    pinMode(SW2_PIN, INPUT_PULLUP);
    pinMode(SW3_PIN, INPUT_PULLUP);
    pinMode(SW4_PIN_SOS, INPUT_PULLUP);
}

void initializeIndicationLEDs()
{
    pinMode(DATA_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
}

void initializeRGBLEDs()
{
    pinMode(RGB_R, OUTPUT);
    pinMode(RGB_G, OUTPUT);
    pinMode(RGB_B, OUTPUT);
    pinMode(OUT_LED, OUTPUT);
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
void connectToWiFi()
{
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
        addLogData("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    addLogData("Connected to WiFi");
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
    // if any error was found
    if (!Firebase.ready())
    {
        Serial.print("setting firebase failed:");
        Serial.println(fbdo.errorReason());
        stateOfFirebase = false;
    }
    else
    {
        Serial.println("setting firebase...ok");
        stateOfFirebase = true;
    }
}
void initializeMPU6050()
{
    Wire.begin(MPU_SDA, MPU_SCL);
    if (!mpu.begin())
    {
        Serial.println("Failed to find MPU6050 chip");
        while (1)
        {
            delay(10);
        }
    }
    Serial.println("MPU6050 Found!");
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}
void initializeRTC()
{
    Serial.println("Initializing RTC");
    Rtc.Begin();
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }
    now = Rtc.GetDateTime();
    if (now < compiled)
    {
        Serial.println("RTC is older than compile time! Updating");
        Rtc.SetDateTime(compiled);
    }
    else if (now > compiled)
    {
        Serial.println("RTC is newer than compile time. Not updating");
    }
    else if (now == compiled)
    {
        Serial.println("RTC is the same as compile time! Not updating");
    }
}
String getDate()
{
    now = Rtc.GetDateTime();
    String dateTime = String(now.Year(), DEC) + "/" + String(now.Month(), DEC) + "/" + String(now.Day(), DEC);
    return dateTime;
}
String getTime()
{
    now = Rtc.GetDateTime();
    String dateTime = String(now.Hour(), DEC) + ":" + String(now.Minute(), DEC) + ":" + String(now.Second(), DEC);
    return dateTime;
}
bool readMpuData()
{
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Serial.print("Acceleration X: ");
    // Serial.print(a.acceleration.x);
    // Serial.print(", Y: ");
    // Serial.print(a.acceleration.y);
    // Serial.print(", Z: ");
    // Serial.println(a.acceleration.z);

    accelX = a.acceleration.x;
    accelY = a.acceleration.y;
    accelZ = a.acceleration.z;
    return true;
}
void detectStep()
{
    if (readMpuData())
    {
        float accX = accelX / 10;
        float accY = accelY / 10;
        float accZ = accelZ / 10;

        float accMagnitude = sqrt(accX * accX + accY * accY + accZ * accZ);

        if ((accMagnitudePrev > accMagnitude + 0.1) && (accMagnitudePrev > stepThreshold))
        {
            stepCount++;
            saveStepCount();
            addLogData("stepCount:");
            addLogData(String(stepCount));
            updateLEDs(patterns[2]);
            digitalWrite(OUT_LED, HIGH);
            delay(200);
            digitalWrite(OUT_LED, LOW);
            delay(200);
        }
        accMagnitudePrev = accMagnitude;
    }
    else
    {
        Serial.println("Error reading MPU data");
    }
}
bool findLastDate()
{
    FirebaseJson json;
    FirebaseData fbdoLastIndex;
    // FirebaseJsonArray arr;

    String path = "/" + USER_ID + "/count/step";
    Serial.printf("Get Array... %s\n\n", Firebase.RTDB.getJSON(&fbdo, path) ? "ok" : fbdo.errorReason().c_str());
    if (fbdo.dataType() == "json")
    {
        json = fbdo.jsonObject();
        Serial.println("Data type: " + fbdo.dataType());
        FirebaseJsonData result;

        json.get(result, "/list");
        if (result.success)
        {
            Serial.println(result.type);
        }
        FirebaseJsonArray arr;
        result.get<FirebaseJsonArray>(arr);
        Serial.printf("Get Array... %s\n\n", Firebase.RTDB.getJSON(&fbdoLastIndex, path + "/list/" + (arr.size() - 1) + "/date") ? "ok" : fbdoLastIndex.errorReason().c_str());
        if (fbdoLastIndex.dataType() == "string")
        {
            lastIndexInt = arr.size() - 1;
            String date = fbdoLastIndex.stringData();
            Serial.println("Last date: " + date);
            if (date == getDate())
            {
                Serial.println("Same date");
                return true;
            }
            else
            {
                Serial.println("Different date");
                return false;
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
        Serial.println("Invalid data type");
        return false;
    }
}
void saveStepCount()
{
    // Save stepCount to EEPROM
    EEPROM.put(0, stepCount);
    EEPROM.commit();
    if (findLastDate())
    {
        updateStepCount();
    }
    else
    {
        addStepCount();
    }
}
void updateStepCount()
{
    FirebaseJson json;
    String path = "/" + USER_ID + "/count/step/list/" + lastIndexInt;
    json.set("/step", stepCount);
    json.set("/date", getDate());
    json.set("/time", getTime());
    // update json
    Serial.printf("Update json... %s\n\n", Firebase.RTDB.updateNode(&fbdo, path, &json) ? "ok" : fbdo.errorReason().c_str());
    // logData();
    // delay(100);
}
void addStepCount()
{

    // FirebaseJsonArray arr;
    // get old data
    FirebaseJson json;
    json.set("/step", stepCount);
    json.set("/date", getDate());
    json.set("/time", getTime());
    String path = "/" + USER_ID + "/count/step";
    // arr.add(json);
    // update json
    Serial.printf("Update json... %s\n\n", Firebase.RTDB.setJSON(&fbdo, path + "/list/" + (lastIndexInt + 1), &json) ? "ok" : fbdo.errorReason().c_str());
    // logData();
    // delay(100);
}
void displayStepCount()
{
    Serial.print("Steps: ");
    Serial.println(stepCount);
}
// // log data in SD
void logData()
{
    // log data in SD
    // open file for writing
    File file = SD.open("/step.txt", FILE_APPEND);
    if (file)
    {
        String data = getDate() + " " + getTime() + " " + String(stepCount);
        file.println(data);
        file.close();
        Serial.println("Data logged in SD " + data);
    }
    else
    {
        Serial.println("Failed to open file for writing");
    }
}
void findDeviceFile()
{
    // find device file
    if (!SD.exists("/device.txt"))
    {
        Serial.println("Device file not found");
        createDeviceFile(getDeviceFirebase());
    }
    else
    {
        Serial.println("Device file found");
        Serial.println(readDeviceJson());
    }
}

void createDeviceFile(String data)
{
    // create device file
    File userFile = SD.open("/device.txt", FILE_WRITE);
    if (!userFile)
    {
        Serial.println("Failed to open device.txt for writing");
    }
    else
    {
        // getDeviceFirebase();
        userFile.print(data);
        userFile.close();
        Serial.println("Device file created");
    }
}

String getDeviceFirebase()
{
    // get device data from firebase
    String path = "/device/" + DEVICE_ID + "/user_id";
    Serial.printf("Get... %s\n\n", Firebase.RTDB.getString(&fbdo, path) ? "ok" : fbdo.errorReason().c_str());
    if (fbdo.dataType() == "string")
    {
        String uid = fbdo.stringData();
        Serial.println("Data type: " + fbdo.dataType());
        String path2 = uid + "/";
        Serial.printf("Get... %s\n\n", Firebase.RTDB.getJSON(&fbdo, path2) ? "ok" : fbdo.errorReason().c_str());
        if (fbdo.dataType() == "json")
        {
            FirebaseJson &json = fbdo.jsonObject();
            FirebaseJsonData result;
            json.get(result, "/user");
            if (result.success)
            {
                String doc = result.stringValue;
                Serial.println(doc);
                return doc;
            }
            else
            {
                Serial.println("Failed to get data");
                return "";
            }
        }
    }
    else
    {
        Serial.println("Invalid data type");
        return "";
    }
}
String readDeviceJson()
{
    // read device file
    File userFile = SD.open("/device.txt", FILE_READ);
    if (!userFile)
    {
        Serial.println("Failed to open device.txt for reading");
        return "";
    }
    else
    {
        String data = userFile.readString();
        userFile.close();
        Serial.println("Device file read");
        return data;
    }
}

// void bluetoothConfig()
// {
//     // bluetooth
//     ESP_BT.begin("ESP32-" + DEVICE_ID); // Bluetooth device name
//     Serial.println("The device started, now you can pair it with bluetooth!");
// }
// void readBLE()
// {
//     // read data from BLE
//     if (ESP_BT.available())
//     {
//         String data = ESP_BT.readString();
//         Serial.println(data);
//         // string to ArduinoJson
//         // DynamicJsonDocument doc(1024);
//         // deserializeJson(doc, data);
//         // // get data from json
//         // String command = doc["command"];
//         // if (command == "SETUP")
//         // {
//         //     Serial.println("Comand received: SETUP");
//         //     createDeviceFile(doc["data"]);
//         // }
//         // // send data to BLE
//         // SerialBT.println("Data received");
//         // Serial.println("Searching...");
//         delay(1000);
//     }
// }

void rgbLighting(int mode)
{
    switch (mode)
    {
    case 0:
        rgb(255, 255, 255);
        break;
    case 1:
        rgb(255, 0, 0);
        break;
    case 2:
        rgb(0, 255, 0);
        break;
    case 3:
        rgb(0, 0, 255);
        break;
    case 4:
        rgb(255, 0, 255);
        break;
    case 5:
        rgb(255, 255, 255);
        break;
    case 6:
        rgb(0, 0, 0);
        break;

    default:
        rgb(255, 255, 255);
        break;
    }
}

void sos(bool state)
{
    int i = 0;
    while (state && i < 5)
    {
        rgb(255, 255, 255);
        delay(300);
        rgb(0, 0, 0);
        delay(300);
        rgb(255, 255, 255);
        delay(1000);
        rgb(255, 255, 255);
        delay(300);
        rgb(0, 0, 0);
        delay(300);
        rgb(255, 255, 255);
        delay(1000);
        i++;
    }
}

void rgb(int r, int g, int b)
{
    analogWrite(RGB_R, r);
    analogWrite(RGB_G, g);
    analogWrite(RGB_B, b);
}

bool readRGBFromFirebse()
{
    FirebaseData fbdo;
    if (Firebase.RTDB.getInt(&fbdo, "/" + USER_ID + "/led/red"))
    {
        if (fbdo.dataType() == "int")
        {
            red = fbdo.intData();
            Serial.println(red);
        }
    }
    else
    {
        Serial.println(fbdo.errorReason());
        return false;
    }
    if (Firebase.RTDB.getInt(&fbdo, "/" + USER_ID + "/led/green"))
    {
        if (fbdo.dataType() == "int")
        {
            green = fbdo.intData();
            Serial.println(green);
        }
    }
    else
    {
        Serial.println(fbdo.errorReason());
        return false;
    }
    if (Firebase.RTDB.getInt(&fbdo, "/" + USER_ID + "/led/blue"))
    {
        if (fbdo.dataType() == "int")
        {
            blue = fbdo.intData();
            Serial.println(blue);
        }
    }
    else
    {
        Serial.println(fbdo.errorReason());
        return false;
    }
    return true;
}
bool readDeviceStateFormFirebase()
{
    if (Firebase.RTDB.getBool(&fbdo, "/" + USER_ID + "/state"))
    {
        if (fbdo.dataType() == "bool")
        {
            Serial.println(deviceState);
            return fbdo.boolData();
        }
    }
    else
    {
        Serial.println(fbdo.errorReason());
        return true;
    }
}

bool listningSOS()
{
    if (Firebase.RTDB.getBool(&fbdo, "/" + USER_ID + "/sos"))
    {
        if (fbdo.dataType() == "bool")
        {
            Serial.println(deviceState);
            return fbdo.boolData();
        }
    }
    else
    {
        Serial.println(fbdo.errorReason());
        return true;
    }
}

void activityStartSOSSignal()
{
    sos(true);
    if (Firebase.RTDB.getBool(&fbdo, "/" + USER_ID + "/sos"))
    {
        if (fbdo.dataType() == "bool")
        {
            Serial.println(deviceState);
            stateOfSOS = fbdo.boolData();
            if (!fbdo.boolData())
            {
                sos(false);
            }
        }
    }
    else
    {
        Serial.println(fbdo.errorReason());
        stateOfSOS = true;
    }
    delay(100);
}
void changeStateOfSOS()
{
    // change state of SOS in firebase
    FirebaseJson json3;
    json3.set("/sos", stateOfSOS);
    Serial.printf("Update Sos... %s\n\n", Firebase.RTDB.updateNode(&fbdo, "/" + USER_ID, &json3) ? "ok" : fbdo.errorReason().c_str());
}

// HTML code for the web page
String getHtmlPage()
{
    String page = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <h1>ESP32 Web Server</h1>
  <h2>Console Output:</h2>
  <pre>)rawliteral";
    page += logDataServer;
    page += R"rawliteral(</pre>
</body>
</html>
)rawliteral";
    return page;
}

void addLogData(const String &message)
{
    // Add new log message to logData
    logDataServer += message + "\n"; // Append new message with a newline
    if (logDataServer.length() > 1000)
    {                                                 // Prevent string from becoming too large
        logDataServer = logDataServer.substring(500); // Keep the last part if it's too long
    }
}
void updateLEDs(uint8_t leds)
{
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, leds);
    digitalWrite(LATCH_PIN, HIGH);
}
