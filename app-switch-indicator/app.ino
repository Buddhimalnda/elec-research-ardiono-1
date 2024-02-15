#include <Arduino.h>
#define SW2_PIN 35     // wifi switch
#define SW3_PIN 32     // Mode Switch
#define SW4_PIN_SOS 34 // SOS Switch

#define IL1_PIN 33
#define IL2_PIN 26
#define IL3_PIN 27
#define IL4_PIN 25

void setup()
{
    Serial.begin(115200);
    initializeSwitches();
    initializeIndicationLEDs();
}

void loop()
{
    // put your main code here, to run repeatedly:
    if (digitalRead(SW2_PIN))
    {
        digitalWrite(IL1_PIN, HIGH);
        Serial.println("IL1_PIN");
    }
    else
    {
        digitalWrite(IL1_PIN, LOW);
    }

    if (digitalRead(SW3_PIN))
    {
        digitalWrite(IL2_PIN, HIGH);
    }
    else
    {
        digitalWrite(IL2_PIN, LOW);
    }

    if (digitalRead(SW4_PIN_SOS))
    {
        digitalWrite(IL3_PIN, HIGH);
    }
    else
    {
        digitalWrite(IL3_PIN, LOW);
    }
}

void initializeSwitches()
{
    pinMode(SW2_PIN, INPUT);
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
