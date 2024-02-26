#include <Arduino.h>
#include <ArduinoJson.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

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

// Network and Firebase credentials
String ssid = "SLT-4G_163BEA";
String password = "751FCEED";

#define FIREBASE_HOST "https://elec-research-0-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "AIzaSyCvhDMEJ9gnl8lqyV282-8pdHOVmZqyVPs"
String USER_EMAIL = "device@gmail.com";
String USER_PASSWORD = "123456";
const String DEVICE_ID = "10001";
String USER_ID = "ltMvLzncQ6et1HDFmrDffP4r8NE3";

//-------------------------
// errors
bool error = false;
int errorCode = 0;
//-------------------------
void setup()
{
    Serial.begin(115200);

    connectToWiFi();
    initializeFirebase();
    initializeMPU6050();
    initializeRTC();
    initializeSDCard();
}
void loop()
{
    // put your main code here, to run repeatedly:
    // Serial.println("loop");
    detectStep();
    displayStepCount();
    delay(100);
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
    // if (!Rtc.IsDateTimeValid())
    // {
    //     if (Rtc.LastError() != 0)
    //     {
    //         Serial.print("RTC communications error = ");
    //         Serial.println(Rtc.LastError());
    //     }
    //     else
    //     {
    //         Serial.println("RTC lost confidence in the DateTime!");
    //         Rtc.SetDateTime(compiled);
    //     }
    // }
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
            delay(400);
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
    FirebaseJsonArray arr;

    String path = "/" + USER_ID + "/count/step/list";
    Serial.printf("Get Array... %s\n\n", Firebase.RTDB.getArray(&fbdo, path) ? "ok" : fbdo.errorReason().c_str());
    if (fbdo.dataType() == "array")
    {
        arr = fbdo.jsonObject();
        Serial.println(arr);
    }
    else
    {
        Serial.println("Invalid data type");
        return false;
    }
}
// void saveStepCount()
// {
//     // Save stepCount to EEPROM
//     EEPROM.put(0, stepCount);
//     EEPROM.commit();
//     FirebaseJsonArray arr;
//     FirebaseJson json1;
//     FirebaseJson json2;
//     json1.set("/step", stepCount);
//     json1.set("/date", getDate());
//     json1.set("/time", getTime());
//     String path = "/" + USER_ID + "/count/step";
//     arr.add(json1);
//     json2.set("/step", stepCount);
//     json2.set("/list", arr);
//     // update json
//     Serial.printf("Update json... %s\n\n", Firebase.RTDB.updateNode(&fbdo, path, &json2) ? "ok" : fbdo.errorReason().c_str());
//     logData();
//     // delay(100);
// }

// void displayStepCount()
// {
//     Serial.print("Steps: ");
//     Serial.println(stepCount);
// }

// // log data in SD
// void logData()
// {
//     // log data in SD
//     // open file for writing
//     File file = SD.open("/step.txt", FILE_APPEND);
//     if (file)
//     {
//         String data = getDate() + " " + getTime() + " " + String(stepCount);
//         file.println(data);
//         file.close();
//         Serial.println("Data logged in SD " + data);
//     }
//     else
//     {
//         Serial.println("Failed to open file for writing");
//     }
// }
