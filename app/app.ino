#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// MPU
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// SD Card
#include <SD.h>
#include <SPI.h>

// RTC
#include <ThreeWire.h>
#include <RtcDS1302.h>

// Firebase
// #include <FirebaseESP32.h>
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

//----------- GPIO ------------
// MicroSD
#define SD_CS 15
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK 18
// MPU
#define MPU_SDA 21
#define MPU_SCL 22
// Switches
#define SW2_PIN 35     // wifi switch
#define SW3_PIN 32     // Mode Switch
#define SW4_PIN_SOS 34 // SOS Switch
// LED
#define IL1_PIN 33
#define IL2_PIN 26
#define IL3_PIN 27
#define IL4_PIN 25
// RGB
#define RGB_R 14
#define RGB_G 12
#define RGB_B 13
// RTC
#define RTC_CLK 5
#define RTC_RST 2
#define RTC_IO 4

//----------- MPU ------------
Adafruit_MPU6050 mpu;
float accelX, accelY, accelZ;

float accMagnitudePrev = 0;
unsigned long lastStepTime = 0;
int stepCount = 0;
float stepThreshold = 1.2; // Experimentally determined threshold for step detection

//----------- SD ------------
File userFile;
String doc;

//----------- Firebase ------------
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData fbdo;
FirebaseData firebaseData;
FirebaseData userData;
// Network and Firebase credentials
String ssid = "SLT-4G_163BEA";
String password = "751FCEED";
#define FIREBASE_HOST "https://elec-research-0-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "AIzaSyCvhDMEJ9gnl8lqyV282-8pdHOVmZqyVPs"
String USER_EMAIL = "device@gmail.com";
String USER_PASSWORD = "123456";
const String DEVICE_ID = "10001";
String USER_ID = "HYWBRRXdwtN7TseWcR5AKpybrqW2";
//----------- errors ------------
bool error = false;
int errorCode = 0;
//----------- RTC ------------
RtcDateTime now;
ThreeWire myWire(RTC_IO, RTC_CLK, RTC_RST); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);
//-------------------------
int stateOfMode = 0;
bool stateOfSOS = false;
int red = 0;
int green = 0;
int blue = 0;
//-------------------------

bool sw2State = false;
bool sw3State = false;
bool stateOfFirebase = false;
bool stateOfSD = false;

void setup()
{
    Serial.begin(115200);
    if (sw2State)
    {
        connectToWiFi();
        initializeFirebase();
    }

    if (initializeSDCard())
    {
        Serial.println("initializeSDCard");
        checkAndCreateDeviceJson();
        stateOfSD = true;
    }

    initializeSwitches();
    initializeIndicationLEDs();
    initializeRGBLEDs();
    initializeRTC();
    initializeMPU6050();
}

void loop()
{
    if (sw2State)
    {
        connectToWiFi();
        if (initializeFirebase())
        {
            stateOfFirebase = true;
        }
    }

    if (digitalRead(SW2_PIN) == HIGH)
    {
        digitalWrite(IL1_PIN, HIGH);
        sw2State ? sw2State = false : sw2State = true;
    }
    else
    {
        digitalWrite(IL1_PIN, LOW);
    }

    if (digitalRead(SW3_PIN))
    {
        digitalWrite(IL2_PIN, HIGH);
        sw3State ? sw3State = false : sw3State = true;
        stateOfMode++;
        if (stateOfMode > 6)
        {
            stateOfMode = 0;
        }
    }
    else
    {
        digitalWrite(IL2_PIN, LOW);
    }

    if (digitalRead(SW4_PIN_SOS))
    {
        digitalWrite(IL3_PIN, HIGH);
        stateOfSOS ? stateOfSOS = false : stateOfSOS = true;
        Serial.println("Active SOS");
    }
    else
    {
        digitalWrite(IL3_PIN, LOW);
    }
    Serial.println(stateOfMode);
    if (sw3State)
    {
        if (stateOfSOS)
        {
            Serial.println("SOS");
            sos(stateOfSOS);
        }
        else
        {
            rgbLighting(stateOfMode);
        }
    }
    else
    {
        if (stateOfFirebase)
        {
            readRGBFromFirebse();
            rgb(red, green, blue);
            delay(100);
        }
        else
        {
            rgbLighting(stateOfMode);
        }
    }
    detectStep();
    displayStepCount();
    delay(10);
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

bool initializeFirebase()
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
    return true;
}

void initializeSwitches()
{
    pinMode(SW2_PIN, INPUT_PULLUP);
    pinMode(SW3_PIN, INPUT);
    pinMode(SW4_PIN_SOS, INPUT);
}

void initializeIndicationLEDs()
{
    pinMode(IL1_PIN, OUTPUT);
    pinMode(IL2_PIN, OUTPUT);
    pinMode(IL3_PIN, OUTPUT);
    pinMode(IL4_PIN, OUTPUT);
}

void initializeRGBLEDs()
{
    pinMode(RGB_R, OUTPUT);
    pinMode(RGB_G, OUTPUT);
    pinMode(RGB_B, OUTPUT);
}

// RGB LED
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

void readRGBFromFirebse()
{
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
    }
    delay(100);
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

bool readMpuData()
{
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    Serial.print("Acceleration X: ");
    Serial.print(a.acceleration.x);
    Serial.print(", Y: ");
    Serial.print(a.acceleration.y);
    Serial.print(", Z: ");
    Serial.println(a.acceleration.z);

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
        }
        accMagnitudePrev = accMagnitude;
    }
    else
    {
        Serial.println("Error reading MPU data");
    }
}
void saveStepCount()
{
    // Save stepCount to EEPROM
    EEPROM.put(0, stepCount);
    EEPROM.commit();
    if (stateOfFirebase)
    {
        /* code */
        FirebaseJson json;
        json.set("/step", stepCount);
        String path = "/" + USER_ID + "/count/step/";
        Serial.printf("Update json... %s\n\n", Firebase.RTDB.updateNode(&firebaseData, path, &json) ? "ok" : firebaseData.errorReason().c_str());
    }
    if (stateOfSD)
    {
        File file = SD.open("/step.txt", FILE_WRITE);
        if (!file)
        {
            Serial.println("Failed to open step.txt for writing");
        }
        String x = "";
        while (file.available())
        {
            x += (char)userFile.read();
        }
        // json time and step
        String row = "{\"time\": \"" + printDateTime() + "\", \"step\": " + stepCount + "}";
        Serial.println(x + row);
        file.print(row);
        file.close();
    }
    delay(100);
}

void displayStepCount()
{
    Serial.print("Steps: ");
    Serial.println(stepCount);
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
        if (stateOfFirebase)
        {
            // Device.json doesn't exist, fetch data from Firebase
            if (!fetchDataFromFirebase())
            {
                return false; // Failed to fetch data
            }
            // Write the fetched data to device.json
            return writeDeviceJson(doc);
        }
        else
        {
            Serial.println("Firebase not connected");
            return false;
        }
    }
    else
    {
        Serial.println("device.json already exists");
        deleteFile("/device.txt");
        checkAndCreateDeviceJson();
    }
    return true; // File exists or was successfully created
}

bool fetchDataFromFirebase()
{

    if (Firebase.RTDB.getString(&fbdo, "/device/" + DEVICE_ID + "/user_id"))
    {
        if (fbdo.dataType() == "string")
        {
            FirebaseJson firebaseJson;
            Serial.println(fbdo.stringData());
            USER_ID = fbdo.stringData();
            if (Firebase.RTDB.getJSON(&userData, "/" + USER_ID + "/", &firebaseJson))
            {
                if (userData.dataType() == "json")
                {
                    firebaseJson.toString(doc, true); // The 'true' argument formats the string with indentation
                    Serial.println(doc);
                    // Step 2: Deserialize String to JsonDocument
                    // Adjust size as necessary
                    // DeserializationError error = deserializeJson(doc, jsonString);

                    // if (error)
                    // {
                    //   Serial.print(F("deserializeJson() failed: "));
                    //   Serial.println(error.f_str());
                    // }
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
        return doc; // Return empty document in case of failure
    }
    doc = "";
    while (userFile.available())
    {
        doc += (char)userFile.read();
    }
    Serial.println(doc);
    userFile.close();
    return doc;
}

bool writeDeviceJson(const String &doc)
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

void emergencyMode()
{
    // emergency mode
    if (stateOfSOS)
    {
        Serial.println("SOS");
        sos(stateOfSOS);
    }
    else
    {
    }
}

String printDateTime()
{
    now = Rtc.GetDateTime();
    Serial.print(now.Year(), DEC);
    Serial.print('/');
    Serial.print(now.Month(), DEC);
    Serial.print('/');
    Serial.print(now.Day(), DEC);
    Serial.print(" ");
    Serial.print(now.Hour(), DEC);
    Serial.print(':');
    Serial.print(now.Minute(), DEC);
    Serial.print(':');
    Serial.print(now.Second(), DEC);
    Serial.println();
    String date = String(now.Year()) + "/" + String(now.Month()) + "/" + String(now.Day()) + " " + String(now.Hour()) + ":" + String(now.Minute()) + ":" + String(now.Second());
    return date;
}