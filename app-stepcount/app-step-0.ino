#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <WiFi.h>

// Firebase
#include <FirebaseESP32.h>
// #include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

// MPU
#define MPU_SDA 21
#define MPU_SCL 22

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
String USER_ID = "HYWBRRXdwtN7TseWcR5AKpybrqW2";

bool stateOfWIFI = true;
//-------------------------
// errors
bool error = false;
int errorCode = 0;
//-------------------------
void setup()
{
    Serial.begin(115200);
    if (stateOfWIFI)
    {
        connectToWiFi();
    }
    initializeFirebase();
    initializeMPU6050();
}
void loop()
{
    // put your main code here, to run repeatedly:
    // Serial.println("loop");
    detectStep();
    displayStepCount();
    delay(100);
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
    stateOfWIFI = true;
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

    // Serial.print("Gyro X: ");
    // Serial.print(g.gyro.x);
    // Serial.print(", Y: ");
    // Serial.print(g.gyro.y);
    // Serial.print(", Z: ");
    // Serial.println(g.gyro.z);

    // Serial.print("Temperature: ");
    // Serial.println(temp.temperature);
    accelX = a.acceleration.x;
    accelY = a.acceleration.y;
    accelZ = a.acceleration.z;
    return true;
}

void detectStep()
{
    readMpuData();
    // float accX = accelX / 16384.0;
    // float accY = accelY / 16384.0;
    // float accZ = accelZ / 16384.0;
    float accX = accelX / 16;
    float accY = accelY / 16;
    float accZ = accelZ / 16;

    // Calculate the magnitude of acceleration
    // accX * accX is equivalent to pow(accX, 2)
    float accMagnitude = sqrt(accX * accX + accY * accY + accZ * accZ);
    // float accMagnitude = sqrt(accX * accX + accY * accY + accZ * accZ);: This line calculates the magnitude of the acceleration vector using the calculated acceleration values for the X, Y, and Z axes. The sqrt() function is used to calculate the square root of the sum of the squared acceleration values.
    // Serial.println(accMagnitude);
    // Serial.println(i);
    // Serial.println(accX);
    // Peak detection
    if ((accMagnitudePrev > accMagnitude + 0.1) && (accMagnitudePrev > stepThreshold))
    {
        stepCount++;
        saveStepCount();
        // Create a string containing the step count data
        // String stepData = String(stepCount);
        // Update the characteristic value
        // pStepDataCharacteristic->setValue(stepData.c_str());
        // pStepDataCharacteristic->notify();
    }
    accMagnitudePrev = accMagnitude;
}
void saveStepCount()
{
    // Save stepCount to EEPROM
    EEPROM.put(0, stepCount);
    EEPROM.commit();
    FirebaseJson json;
    json.add("step", stepCount);
    String path = "/" + USER_ID + "/count/";
    updateFirebaseData(json, path);
}

void displayStepCount()
{
    Serial.print("Steps: ");
    Serial.println(stepCount);
}

// update firebase
void updateFirebaseData(FirebaseJson json, String path)
{
    Serial.printf("Update json... %s\n\n", Firebase.RTDB.updateNode(&fbdo, path, &json) ? "ok" : fbdo.errorReason().c_str());
}