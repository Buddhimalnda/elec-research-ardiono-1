// import comman libraries
#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// RTC
#include <ThreeWire.h>
#include <RtcDS1302.h>
// MPU6050
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include <WiFi.h>
// SD Card
#include <SD.h>
#include <SPI.h>

// MPU
#define MPU_SDA 21
#define MPU_SCL 22
// RTC
#define RTC_CLK 5
#define RTC_RST 2
#define RTC_IO 4
// SD
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
// now time
RtcDateTime now;
ThreeWire myWire(RTC_IO, RTC_CLK, RTC_RST); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

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
bool connectedWifi = false;

// RGB LED
int red = 0;
int green = 0;
int blue = 0;
int MODE = 0;

// MPU
Adafruit_MPU6050 mpu;
float accelX, accelY, accelZ;

float accMagnitudePrev = 0;

unsigned long lastStepTime = 0;
int stepCount = 0;
float stepThreshold = 1.2; // Experimentally determined threshold for step detection
float debounceTime = 250;  // Minimum time between steps (milliseconds)
int lastIndexInt = 0;

// Error
int errorCode = 0;
typedef struct
{
    int errorCode;
    String errorMessage;
} ErrorIndicator;
typedef struct
{
    int successCode;
    String successMessage;
} SuccessIndicator;
typedef struct
{
    int warningCode;
    String warningMessage;
} WarningIndicator;

// indicator
ErrorIndicator errorIndicator = {7, ""};
SuccessIndicator successIndicator = {1, ""};
WarningIndicator warningIndicator = {1, ""};

// web server
#include <WebServer.h>
WebServer server(80); // Object to hold the HTTP server
String logDataServer = "";

void setup()
{

    Serial.begin(115200);
    initializeSwitches();
    initializeRGBLEDs();
    initializeIndicationLEDs();
    initializeMPU6050();
    EEPROM.begin(EEPROM_SIZE);
    // if (onlineMode)
    // {
    //     printSerial(WiFi.localIP());
    //     server.on("/", HTTP_GET, []()
    //               {
    //                   server.send(200, "text/html", getHtmlPage()); // Send web page with log data
    //               });
    //     server.begin();
    // }
    // xTaskCreate(
    //     ListningSwitches,   // Task function
    //     "ListningSwitches", // Name of the task
    //     2048,               // Stack size (bytes)
    //     NULL,               // Task input parameter
    //     1,                  // Priority of the task
    //     NULL);
    // if (onlineMode)
    onlineModeAction();

    xTaskCreate(
        TaskRGB,      // Task function
        "RGBControl", // Name of the task
        2048,         // Stack size (bytes)
        NULL,         // Task input parameter
        3,            // Priority of the task
        NULL);        // Task handle
    xTaskCreate(
        TaskStepCounter, // Task function
        "MPU6050Read",   // Name of the task
        2048,            // Stack size (bytes)
        NULL,            // Task input parameter
        4,               // Priority of the task
        NULL);           // Task handle
                         // Task handle
    xTaskCreate(
        ErrorIndicatorTask, // Task function
        "ErrorIndicator",   // Name of the task
        2048,               // Stack size (bytes)
        NULL,               // Task input parameter
        2,                  // Priority of the task
        NULL);              // Task handle
}
void onlineModeAction()
{
    printSerial("onlineModeAction");
    String fileDoc = "";
    File file;
    if (initializeSDCard())
    {
        fileDoc = readDeviceJson(file);
        if (fileDoc != "")
        {
            // this doc is the json string, get the data from the json string
            StaticJsonDocument<256> docjsonR;
            DeserializationError errorR = deserializeJson(docjsonR, fileDoc); // Convert string to JSON object
            if (errorR)
            {
                printSerial("deserializeJson() failed: ");
                printSerial(errorR.c_str());
                return;
            }
            // const char *device = docjson["device"];     // "1002"
            USER_EMAIL = docjsonR["email"].as<String>();           // "dn@rb.com"
            USER_PASSWORD = docjsonR["password"].as<String>();     // "123455"
            USER_ID = docjsonR["userId"].as<String>();             // "ltMvLzncQ6et1HDFmrDffP4r8NE3"
            SSID = docjsonR["wifi"]["ssid"].as<String>();          // "SLT-4G_163BEA"
            WIFI_PASS = docjsonR["wifi"]["password"].as<String>(); // "751FCEED"
            connectToWiFi();
            initializeFirebase();
            onlineMode = true;
        }
        int step = readLastStepcountFromSD(getDateForPath());
        printSerial(step);
        if (step > 0)
        {
            stepCount = step;
        }
        else
        {
            stepCount = 0;
        }
    }
    else
    {
        printSerial("Non-initializeSDCard");
        onlineMode = false;
    }
}
void initializeMPU6050()
{
    Wire.begin(MPU_SDA, MPU_SCL);
    if (!mpu.begin())
    {
        printSerial("Failed to find MPU6050 chip");

        while (1)
        {
            delay(10);
        }
    }
    printSerial("MPU6050 Found!");
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}
void connectToWiFi()
{
    WiFi.begin(SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        printSerial("Connecting to WiFi...");
    }
    printSerial("Connected to WiFi");
    connectedWifi = true;
}
void initializeFirebase()
{
    printSerial("Initializing Firebase");
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
        printSerial("Task is running: TaskRGB");
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for a second
        if (onlineMode)
        {
            printSerial("onlineMode");
            if (!stateOfSOS)
            {
                readRGBFromFirebse();
                rgb(red, green, blue);
                printSerial("RGB: " + String(red) + " " + String(green) + " " + String(blue));
            }
            else
            {
                sos(true);
                printSerial("SOS: ");
            }
        }
        else
        {
            printSerial("offlineMode");
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
        printSerial("Task is running: TaskStepCounter");
        vTaskDelay(150 / portTICK_PERIOD_MS); // Delay for a second
        printSerial("detectStep");
        detectStep();
        displayStepCount();
    }
}
void ErrorIndicatorTask(void *pvParameters)
{
    for (;;)
    { // Infinite loop
        printSerial("Task is running: ErrorIndicator");
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for a second
        uint8_t errorBinary = findErrorCode(errorIndicator.errorCode);
        uint8_t successBinary = findSuccessCode(successIndicator.successCode);
        uint8_t warningBinary = findWarningCode(warningIndicator.warningCode);
        uint8_t leds = errorBinary + successBinary + warningBinary;

        printSerial(errorBinary);
        // print
        printSerial("Error: " + errorIndicator.errorMessage);
        printSerial("Success: " + successIndicator.successMessage);
        printSerial("Warning: " + warningIndicator.warningMessage);
        updateFontPannel(errorBinary);
    }
}

void ListningSwitches(void *pvParameters)
{
    for (;;)
    { // Infinite loop
        printSerial("Task is running: ListningSwitches...");
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for a second
        int buttonState_SW2 = digitalRead(SW2_PIN);
        int buttonState_SW3 = digitalRead(SW3_PIN);
        int buttonState_SW4 = digitalRead(SW4_PIN_SOS);
        int buttonState_SW5 = digitalRead(SW5_PIN);
        if (buttonState_SW2 == LOW)
        {
            // The button is pressed; perform actions
            printSerial("SW2 is pressed");
            if (onlineMode)
            {
                onlineMode = false;
            }
            else
            {
                onlineModeAction();
            }
        }
        if (buttonState_SW3 == LOW)
        {
            // The button is pressed; perform actions
            printSerial("SW3 is pressed");
            if (++MODE > MODE_MAX)
            {
                MODE = 0;
            }
        }
        if (buttonState_SW4 == LOW)
        {
            // The button is pressed; perform actions
            printSerial("SW4 is pressed");
            sosAction();
        }
        if (buttonState_SW5 == LOW)
        {
            // The button is pressed; perform actions
            printSerial("SW5 is pressed");
        }
    }
}

void loop()
{
    // put your main code here, to run repeatedly:
    // printSerial("loop");
    // delay(1000);
    // if (onlineMode)
    // {
    //     printSerial(WiFi.localIP());
    //     server.on("/", HTTP_GET, []()
    //               {
    //                   server.send(200, "text/html", getHtmlPage()); // Send web page with log data
    //               });
    //     server.begin();
    //     server.handleClient();
    // }
    // else
    // {
    //     printSerial("offlineMode");
    //     printSerial("device is offline 3s");
    //     delay(3000);
    //     ESP.restart();
    // }
}

void initializeRTC()
{
    printSerial("Initializing RTC");
    Rtc.Begin();
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

    if (!Rtc.GetIsRunning())
    {
        printSerial("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }
    now = Rtc.GetDateTime();
    if (now < compiled)
    {
        printSerial("RTC is older than compile time! Updating");
        Rtc.SetDateTime(compiled);
    }
    else if (now > compiled)
    {
        printSerial("RTC is newer than compile time. Not updating");
    }
    else if (now == compiled)
    {
        printSerial("RTC is the same as compile time! Not updating");
    }
}
bool initializeSDCard()
{
    printSerial("Initializing SD Card");
    if (!SD.begin(SD_CS))
    {
        printSerial("SD Card initialization failed"); // SD Card initialization failed
        return false;
    }
    printSerial("SD Card initialization successful");
    return true;
}

String readDeviceJson(File file)
{ // Adjust size as needed
    file = SD.open("/device.txt", FILE_READ);
    String docRDJ = "";
    if (!file)
    {
        printSerial("Failed to open device.json for reading");
        return docRDJ;
        // Return empty document in case of failure
    }
    docRDJ += file.readString();
    printSerial(docRDJ);
    file.close();
    return docRDJ;
}

void initializeIndicationLEDs()
{
    printSerial("initializeIndicationLEDs start...");
    pinMode(DATA_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    printSerial("initializeIndicationLEDs Done.");
}

void initializeRGBLEDs()
{
    printSerial("initializeRGBLEDs start...");
    pinMode(RGB_R, OUTPUT);
    pinMode(RGB_G, OUTPUT);
    pinMode(RGB_B, OUTPUT);
    printSerial("initializeRGBLEDs Done.")
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
    printSerial("initializeSwitches Start...");
    pinMode(SW2_PIN, INPUT_PULLUP);
    pinMode(SW3_PIN, INPUT_PULLUP);
    pinMode(SW4_PIN_SOS, INPUT_PULLUP);
    pinMode(SW5_PIN, INPUT_PULLUP);
    printSerial("initializeSwitches Done.");
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
            printSerial(red);
        }
    }
    else
    {
        printSerial(fbdo.errorReason());
        return false;
    }
    if (Firebase.RTDB.getInt(&fbdo, "/" + USER_ID + "/led/green"))
    {
        if (fbdo.dataType() == "int")
        {
            green = fbdo.intData();
            printSerial(green);
        }
    }
    else
    {
        printSerial(fbdo.errorReason());
        return false;
    }
    if (Firebase.RTDB.getInt(&fbdo, "/" + USER_ID + "/led/blue"))
    {
        if (fbdo.dataType() == "int")
        {
            blue = fbdo.intData();
            printSerial(blue);
        }
    }
    else
    {
        printSerial(fbdo.errorReason());
        return false;
    }
    return true;
}

uint8_t findErrorCode(int code)
{
    switch (code)
    {
    case 0:
        return 0b00000000;
        break;
    case 1:
        return 0b00000001;
        break;
    case 2:
        return 0b00000010;
        break;
    case 3:
        return 0b00000011;
        break;
    case 4:
        return 0b00000100;
        break;
    case 5:
        return 0b00000101;
        break;
    case 6:
        return 0b00000110;
        break;
    case 7:
        return 0b00000111;
        break;
    case 8:
        return 0b00001000;
        break;
    case 9:
        return 0b00001001;
        break;
    case 10:
        return 0b00001010;
        break;
    case 11:
        return 0b00001011;
        break;
    case 12:
        return 0b00001100;
        break;
    case 13:
        return 0b00001101;
        break;
    case 14:
        return 0b00001110;
        break;
    case 15:
        return 0b00001111;
        break;
    default:
        return 0b00000000;
        break;
    }
}

uint8_t findSuccessCode(int code)
{
    switch (code)
    {
    case 0:
        return 0b00000000;
        break;
    case 1:
        return 0b01000000;
        break;
    case 2:
        return 0b10000000;
        break;
    case 3:
        return 0b11000000;
        break;
    default:
        return 0b00000000;
        break;
    }
}

uint8_t findWarningCode(int code)
{
    switch (code)
    {
    case 0:
        return 0b00000000;
        break;
    case 1:
        return 0b00010000;
        break;
    case 2:
        return 0b00100000;
        break;
    case 3:
        return 0b00110000;
        break;
    default:
        return 0b00000000;
        break;
    }
}

void updateFontPannel(uint8_t leds)
{
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, leds);
    digitalWrite(LATCH_PIN, HIGH);
}

void printSerial(String message)
{
    Serial.println(message);
    // create json object
    DynamicJsonDocument docPrint(1024);
    docPrint["message"] = message;
    docPrint["date"] = getDate();
    docPrint["time"] = getTime();
    // serialize json to string
    String output;
    serializeJson(doc, output);
    if (onlineMode)
    {
        addLogData(message);
    }
    logInSD(&output)
}
void logInSD(const String &message)
{
    File fileLSD = SD.open("/log.txt", FILE_APPEND);
    if (fileLSD)
    {

        fileLSD.println(message);
    }
    else
    {
        printSerial("Failed to open log.txt for writing");
    }
    fileLSD.close();
}

String getDate()
{
    now = Rtc.GetDateTime();
    String dateTime = String(now.Year(), DEC) + "/" + String(now.Month(), DEC) + "/" + String(now.Day(), DEC);
    return dateTime;
}
String getDateForPath()
{
    now = Rtc.GetDateTime();
    String dateTime = String(now.Year(), DEC) + "-" + String(now.Month(), DEC) + "-" + String(now.Day(), DEC);
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
    // printSerial(a.acceleration.z);

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
            // saveStepCount();
            printSerial("Step Count: " + String(stepCount));
            printSerial("Step Count: " + String(stepCount));
            if (onlineMode)
            {
                saveStepCountInCloud();
                saveStepCountInSD();
            }
            else
            {
                saveStepCountInSD();
            }
        }
        accMagnitudePrev = accMagnitude;
    }
    else
    {
        printSerial("Error reading MPU data");
    }
}

void saveStepCountInCloud()
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
void saveStepCountInSD()
{
    // Save stepCount to SD card
    String pathSSD = "/step-" + getDateForPath() + ".txt"; // ex:  date = 2021-10-10, path = /step-2021-10-10.txt
    File fileSSD = SD.open(pathSSD, FILE_APPEND);
    if (fileSSD)
    {
        // create json object
        DynamicJsonDocument docSSD(1024);
        docSSD["step"] = stepCount;
        docSSD["date"] = getDate();
        docSSD["time"] = getTime();
        // serialize json to string
        String outputSSD = "";
        serializeJson(docSSD, outputSSD);
        fileSSD.println(outputSSD);
        fileSSD.close();
    }
    else
    {
        printSerial("Failed to open stepCount.txt for writing");
    }
}
bool findLastDate()
{
    FirebaseJson json;
    FirebaseData fbdoLastIndex;
    // FirebaseJsonArray arr;

    String pathFLD = "/" + USER_ID + "/count/step";
    Serial.printf("Get Array... %s\n\n", Firebase.RTDB.getJSON(&fbdo, pathFLD) ? "ok" : fbdo.errorReason().c_str());
    if (fbdo.dataType() == "json")
    {
        json = fbdo.jsonObject();
        printSerial("Data type: " + fbdo.dataType());
        FirebaseJsonData result;

        json.get(result, "/list");
        if (result.success)
        {
            printSerial(result.type);
        }
        FirebaseJsonArray arr;
        result.get<FirebaseJsonArray>(arr);
        Serial.printf("Get Array... %s\n\n", Firebase.RTDB.getJSON(&fbdoLastIndex, pathFLD + "/list/" + (arr.size() - 1) + "/date") ? "ok" : fbdoLastIndex.errorReason().c_str());
        if (fbdoLastIndex.dataType() == "string")
        {
            lastIndexInt = arr.size() - 1;
            String date = fbdoLastIndex.stringData();
            printSerial("Last date: " + date);
            if (date == getDate())
            {
                printSerial("Same date");
                return true;
            }
            else
            {
                printSerial("Different date");
                return false;
            }
        }
        else
        {
            printSerial("Invalid data type");
            return false;
        }
    }
    else
    {
        printSerial("Invalid data type");
        return false;
    }
}
void updateStepCount()
{
    FirebaseJson json;
    String pathUSC = "/" + USER_ID + "/count/step/list/" + lastIndexInt;
    json.set("/step", stepCount);
    json.set("/date", getDate());
    json.set("/time", getTime());
    // update json
    Serial.printf("Update json... %s\n\n", Firebase.RTDB.updateNode(&fbdo, pathUSC, &json) ? "ok" : fbdo.errorReason().c_str());
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
    String pathASC = "/" + USER_ID + "/count/step";
    // arr.add(json);
    // update json
    Serial.printf("Update json... %s\n\n", Firebase.RTDB.setJSON(&fbdo, pathASC + "/list/" + (lastIndexInt + 1), &json) ? "ok" : fbdo.errorReason().c_str());
    // logData();
    // delay(100);
}
void displayStepCount()
{
    printSerial("Steps: " + String(stepCount));
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

void readStepCountFromEEPROM()
{
    // Read stepCount from EEPROM
    EEPROM.get(0, stepCount);
    printSerial("Step count: " + String(stepCount));
}
void readStepCounterLogFormSD()
{
    String pathCLSD = "/step-" + getDateForPath() + ".txt"; // ex:  date = 2021-10-10, path = /step-2021-10-10.txt
    File file = SD.open(pathCLSD, FILE_READ);
    if (!file)
    {
        String message = "Failed to open " + pathCLSD + " for reading";
        printSerial(message);
        return;
    }
    String docSD = "";
    docSD += file.readString();
    printSerial(docSD);
    file.close();
}

int readLastStepcountFromSD(String date)
{
    String pathLSD = "/step-" + date + ".txt"; // ex:  date = 2021-10-10, path = /step-2021-10-10.txt
    File file = SD.open(pathLSD, FILE_READ);
    if (!file)
    {
        String message = "Failed to open " + pathLSD + " for reading";
        printSerial(message);
        return 0;
    }
    String docSDR = "";
    docSDR += file.readString();
    printSerial(docSDR);
    file.close();
    // this doc is the json string, get the data from the json string
    StaticJsonDocument<256> docjson;
    DeserializationError error = deserializeJson(docjson, docSDR); // Convert string to JSON object
    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        printSerial(error.c_str());
        return 0;
    }
    int step = docjson["step"]; // 100
    return step;
}