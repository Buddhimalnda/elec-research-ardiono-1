#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// Firebase
// #include <FirebaseESP32.h>
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

// Network and Firebase credentials
const char *ssid = "SLT-4G_163BEA";
const char *password = "751FCEED";

#define FIREBASE_HOST "https://elec-research-0-default-rtdb.asia-southeast1.firebasedatabase.app" // "<YOUR_FIREBASE_HOST>"
#define FIREBASE_AUTH "AIzaSyCvhDMEJ9gnl8lqyV282-8pdHOVmZqyVPs"                                   //"<YOUR_FIREBASE_AUTH>"

// Firebase
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData fbdo;

#define USER_EMAIL "device@gmail.com"
#define USER_PASSWORD "123456"
const String DEVICE_ID = "10001";
String USER_ID = "HYWBRRXdwtN7TseWcR5AKpybrqW2";

#define SW2_PIN 35     // wifi switch
#define SW3_PIN 32     // Mode Switch
#define SW4_PIN_SOS 34 // SOS Switch

#define IL1_PIN 33
#define IL2_PIN 26
#define IL3_PIN 27
#define IL4_PIN 25

#define RGB_R 14
#define RGB_G 12
#define RGB_B 13

int stateOfMode = 0;

bool stateOfSOS = false;

int red = 0;
int green = 0;
int blue = 0;

//-------------------------
// errors
bool error = false;
int errorCode = 0;
//-------------------------

void setup()
{
    Serial.begin(115200);
    initializeSwitches();
    
        connectToWiFi();
        initializeFirebase();
    
    initializeIndicationLEDs();
    initializeRGBLEDs();
    stateOfMode = 0;
}

void loop()
{
    // put your main code here, to run repeatedly:
    
    readRGBFromFirebse();
    if (digitalRead(SW2_PIN) == HIGH)
    {
        digitalWrite(IL1_PIN, HIGH);
    }
    else
    {
        digitalWrite(IL1_PIN, LOW);
    }

    if (digitalRead(SW3_PIN))
    {
        digitalWrite(IL2_PIN, HIGH);
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
    if (red == 0 && green == 0 && blue == 0)
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
       rgb(red, green, blue);
    }
    
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