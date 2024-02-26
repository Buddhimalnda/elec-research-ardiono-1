#include <Arduino.h>
// #include <ArduinoJson.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include <EEPROM.h>
#include <WiFi.h>
#include <WebServer.h>

// // Firebase
// // #include <FirebaseESP32.h>
// #include <Firebase_ESP_Client.h>
// // Provide the token generation process info.
// #include <addons/TokenHelper.h>
// // Provide the RTDB payload printing info and other helper functions.
// #include <addons/RTDBHelper.h>

// MPU
#define MPU_SDA 21
#define MPU_SCL 22
#define OUT 25
#define RGB_R 14
#define RGB_G 12
#define RGB_B 13

// firebase
// FirebaseData firebaseData;
// FirebaseAuth auth;
// FirebaseConfig config;
// FirebaseData fbdo;
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

// #define FIREBASE_HOST "https://elec-research-0-default-rtdb.asia-southeast1.firebasedatabase.app"
// #define FIREBASE_AUTH "AIzaSyCvhDMEJ9gnl8lqyV282-8pdHOVmZqyVPs"
// String USER_EMAIL = "device@gmail.com";
// String USER_PASSWORD = "123456";
// const String DEVICE_ID = "10001";
// String USER_ID = "HYWBRRXdwtN7TseWcR5AKpybrqW2";

// bool stateOfWIFI = true;
//-------------------------
// errors
bool error = false;
int errorCode = 0;
//-------------------------

WebServer server(80); // Object to hold the HTTP server

String logData = ""; // String to store log data

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
    page += logData;
    page += R"rawliteral(</pre>
</body>
</html>
)rawliteral";
    return page;
}

void addLogData(const String &message)
{
    // Add new log message to logData
    logData += message + "\n"; // Append new message with a newline
    if (logData.length() > 1000)
    {                                     // Prevent string from becoming too large
        logData = logData.substring(500); // Keep the last part if it's too long
    }
}
void setup()
{
    Serial.begin(115200);
    // if (stateOfWIFI)
    // {
    connectToWiFi();
    //     initializeFirebase();
    // }
    initializeMPU6050();
    pinMode(OUT, OUTPUT);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
        addLogData("Connecting to WiFi...");
       
    }
    if(WiFi.status() == WL_CONNECTED){
      digitalWrite(OUT, HIGH);
    }else{
      digitalWrite(OUT, LOW);
    }
    Serial.println("Connected to WiFi");
    addLogData("Connected to WiFi");
    Serial.println(WiFi.localIP());
    server.on("/", HTTP_GET, []()
              {
                  server.send(200, "text/html", getHtmlPage()); // Send web page with log data
              });
  initializeRGBLEDs();
  server.begin();
    rgb(100, 200, 255);
}
void loop()
{
    // put your main code here, to run repeatedly:
    // Serial.println("loop");
    server.handleClient(); // Handle client requests
    detectStep();
    displayStepCount();
if(WiFi.status() == WL_CONNECTED){
      digitalWrite(OUT, HIGH);
    }else{
      digitalWrite(OUT, LOW);
    }
    // Example of adding data to the log
    // Normally, you would add real log messages here
    static unsigned long lastTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastTime > 5000)
    { // Add new log every 5 seconds
        lastTime = currentTime;
        String message = "Timestamp: " + String(currentTime);
        Serial.println(message);
        addLogData(message);
    }
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
    // stateOfWIFI = true;
}

// void initializeFirebase()
// {
//     Serial.println("Initializing Firebase");
//     // Firebase.begin("FIREBASE_HOST", "FIREBASE_AUTH");
//     config.api_key = FIREBASE_AUTH;
//     config.database_url = FIREBASE_HOST;
//     fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

//     // Or use legacy authenticate method
//     // config.database_url = DATABASE_URL;
//     // config.signer.tokens.legacy_token = "<database secret>";
//     /* Assign the api key (required) */
//     config.api_key = FIREBASE_AUTH;

//     /* Assign the user sign in credentials */
//     auth.user.email = USER_EMAIL;
//     auth.user.password = USER_PASSWORD;

//     /* Assign the RTDB URL (required) */
//     config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
//     Firebase.begin(&config, &auth);
//     Firebase.reconnectNetwork(true);
//     Firebase.setDoubleDigits(5);
// }
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
    
    addLogData(String(stepCount));
    // FirebaseJson json;
    // json.set("/step", stepCount);
    // String path = "/" + USER_ID + "/count/step/";
    // Serial.printf("Update json... %s\n\n", Firebase.RTDB.updateNode(&firebaseData, path, &json) ? "ok" : firebaseData.errorReason().c_str());
}

void displayStepCount()
{
    Serial.print("Steps: ");
    Serial.println(String(stepCount));
}

// Serial.print("Gyro X: ");
// Serial.print(g.gyro.x);
// Serial.print(", Y: ");
// Serial.print(g.gyro.y);
// Serial.print(", Z: ");
// Serial.println(g.gyro.z);

// Serial.print("Temperature: ");
// Serial.println(temp.temperature);

// float accMagnitude = sqrt(accX * accX + accY * accY + accZ * accZ);: This line calculates the magnitude of the acceleration vector using the calculated acceleration values for the X, Y, and Z axes. The sqrt() function is used to calculate the square root of the sum of the squared acceleration values.
// Serial.println(accMagnitude);
// Serial.println(i);
// Serial.println(accX);
// Peak detection

// float accX = accelX / 16384.0;
// float accY = accelY / 16384.0;
// float accZ = accelZ / 16384.0;
// Create a string containing the step count data
// String stepData = String(stepCount);
// Update the characteristic value
// pStepDataCharacteristic->setValue(stepData.c_str());
// pStepDataCharacteristic->notify();

// Calculate the magnitude of acceleration
// accX * accX is equivalent to pow(accX, 2)
void initializeRGBLEDs()
{
    pinMode(RGB_R, OUTPUT);
    pinMode(RGB_G, OUTPUT);
    pinMode(RGB_B, OUTPUT);
}
void rgb(int r, int g, int b)
{
    analogWrite(RGB_R, r);
    analogWrite(RGB_G, g);
    analogWrite(RGB_B, b);
}
