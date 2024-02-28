// import comman libraries
#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

#include <WiFi.h>
// SD Card
#include <SD.h>
#include <SPI.h>

#define SD_CS 15
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18

#define RGB_R 14
#define RGB_G 12
#define RGB_B 13
#define DATA_PIN 33
#define LATCH_PIN 26
#define CLOCK_PIN 27
#define NUM_LEDS 8

#define SW2_PIN 35     // onlice switch
#define SW3_PIN 25     // Mode Switch
#define SW4_PIN_SOS 34 // SOS Switch
#define SW5_PIN 39     // activity Switch

#define BTRY_V_IN 36

#define EEPROM_SIZE 512
#define MODE_MAX 6

// Firebase client libraries
// // #include <FirebaseESP32.h>
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

// Network and Firebase credentials
String SSID = "SLT-4G_163BEA";
String WIFI_PASS = "751FCEED";

#define FIREBASE_HOST "https://elec-research-0-default-rtdb.asia-southeast1.firebasedatabase.app" // "<YOUR_FIREBASE_HOST>"
#define FIREBASE_AUTH "AIzaSyCvhDMEJ9gnl8lqyV282-8pdHOVmZqyVPs"                                   //"<YOUR_FIREBASE_AUTH>"

FirebaseAuth auth;
FirebaseConfig config;
FirebaseData fbdo;

const char *DEVICE_ID = "10001";
String USER_EMAIL = "device@gmail.com";
String USER_PASSWORD = "123456";
String USER_ID = "ltMvLzncQ6et1HDFmrDffP4r8NE3";

// state of
bool onlineMode = false;
bool stateOfSOS = false;
bool stateOfACTIVITY = false;

// RGB LED
int red = 0;
int green = 0;
int blue = 0;
int MODE = 0;

// Error
int errorCode = 0;

void setup()
{

    Serial.begin(115200);
    initializeSwitches();
    initializeRGBLEDs();
    initializeIndicationLEDs();
    EEPROM.begin(EEPROM_SIZE);
    onlineModeAction();

    xTaskCreate(
        TaskRGB,      // Task function
        "RGBControl", // Name of the task
        2048,         // Stack size (bytes)
        NULL,         // Task input parameter
        1,            // Priority of the task
        NULL);        // Task handle
    xTaskCreate(
        TaskStepCounter, // Task function
        "MPU6050Read",   // Name of the task
        2048,            // Stack size (bytes)
        NULL,            // Task input parameter
        3,               // Priority of the task
        NULL);           // Task handle
    xTaskCreate(
        ListningSwitches,   // Task function
        "ListningSwitches", // Name of the task
        2048,               // Stack size (bytes)
        NULL,               // Task input parameter
        2,                  // Priority of the task
        NULL);              // Task handle
}
void onlineModeAction()
{
    String fileDoc = "";
    File file;
    if (initializeSDCard())
    {
        Serial.println("initializeSDCard");
        fileDoc = readDeviceJson(file);
        if (fileDoc != "")
        {
            // this doc is the json string, get the data from the json string
            StaticJsonDocument<256> docjson;
            DeserializationError error = deserializeJson(docjson, fileDoc); // Convert string to JSON object
            if (error)
            {
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
                return;
            }
            // const char *device = docjson["device"];     // "1002"
            USER_EMAIL = docjson["email"].as<String>();           // "dn@rb.com"
            USER_PASSWORD = docjson["password"].as<String>();     // "123455"
            USER_ID = docjson["userId"].as<String>();             // "ltMvLzncQ6et1HDFmrDffP4r8NE3"
            SSID = docjson["wifi"]["ssid"].as<String>();          // "SLT-4G_163BEA"
            WIFI_PASS = docjson["wifi"]["password"].as<String>(); // "751FCEED"
            connectToWiFi();
            initializeFirebase();
            onlineMode = true;
        }
    }
    else
    {
        Serial.println("Non-initializeSDCard");
        onlineMode = false;
    }
}

void connectToWiFi()
{
    WiFi.begin(SSID, WIFI_PASS);
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

void TaskRGB(void *pvParameters)
{
    for (;;)
    { // Infinite loop
        Serial.println("Task is running");
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for a second
        if (onlineMode)
        {
            Serial.println("onlineMode");
            if (!stateOfSOS)
            {
                readRGBFromFirebse();
                rgb(red, green, blue);
            }
            else
            {
                sos(true);
            }
        }
        else
        {
            Serial.println("offlineMode");
            if (stateOfSOS)
            {
                sos(true);
            }
            else
            {
                rgbLighting(MODE);
            }
        }
    }
    delay(3000);
}
void TaskStepCounter(void *pvParameters)
{
    for (;;)
    { // Infinite loop
        Serial.println("Task is running");
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for a second
        // Serial.println("TaskRGB");
        Serial.println("detectStep");
    }
}
void ListningSwitches(void *pvParameters)
{
    for (;;)
    { // Infinite loop
        Serial.println("Task is running: ListningSwitches...");
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for a second
        // Serial.println("TaskRGB");
        int buttonState_SW2 = digitalRead(SW2_PIN);
        int buttonState_SW3 = digitalRead(SW3_PIN);
        int buttonState_SW4 = digitalRead(SW4_PIN_SOS);
        int buttonState_SW5 = digitalRead(SW5_PIN);
        if (buttonState_SW2 == HIGH)
        {
            // The button is pressed; perform actions
            Serial.println("SW2 is pressed");
            if (onlineMode)
            {
                onlineMode = false;
            }
            else
            {
                onlineModeAction();
            }
        }
        if (buttonState_SW3 == HIGH)
        {
            // The button is pressed; perform actions
            Serial.println("SW3 is pressed");
            if (++MODE > MODE_MAX)
            {
                MODE = 0;
            }
        }
        if (buttonState_SW4 == HIGH)
        {
            // The button is pressed; perform actions
            Serial.println("SW4 is pressed");
            sosAction();
        }
        if (buttonState_SW5 == HIGH)
        {
            // The button is pressed; perform actions
            Serial.println("SW5 is pressed");
        }
    }
}

void loop()
{
    // put your main code here, to run repeatedly:
    // Serial.println("loop");
    // delay(1000);
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

String readDeviceJson(File file)
{ // Adjust size as needed
    file = SD.open("/device.txt", FILE_READ);
    String doc = "";
    if (!file)
    {
        Serial.println("Failed to open device.json for reading");
        return doc;
        // Return empty document in case of failure
    }
    doc += file.readString();
    Serial.println(doc);
    file.close();
    return doc;
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
    // pinMode(OUT_LED, OUTPUT);
}

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

void initializeSwitches()
{
    pinMode(SW2_PIN, INPUT_PULLUP);
    pinMode(SW3_PIN, INPUT_PULLUP);
    pinMode(SW4_PIN_SOS, INPUT_PULLUP);
    pinMode(SW5_PIN, INPUT_PULLUP);
}

void sosAction()
{
    stateOfSOS = true;
}

void activityAction()
{
    stateOfACTIVITY = true;
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