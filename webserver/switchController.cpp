#include <Arduino.h>

#define SW2_PIN 35     // sync Switch
#define SW3_PIN 32     // Mode Switch
#define SW4_PIN_SOS 34 // SOS Switch
#define SW5_PIN 36     // start Activity

void initializeSwitches()
{
    pinMode(SW2_PIN, INPUT);
    pinMode(SW3_PIN, INPUT);
    pinMode(SW4_PIN_SOS, INPUT);
    pinMode(SW5_PIN, INPUT);
}

bool readSwitch(int pin)
{
    return digitalRead(pin);
}

int switchController()
{
    while (1)
    {
        if (readSwitch(SW2_PIN))
        {
            Serial.println("SW2_PIN is pressed");
            return 1;
        }
        if (readSwitch(SW3_PIN))
        {
            Serial.println("SW3_PIN is pressed");
            return 2;
        }
        if (readSwitch(SW4_PIN_SOS))
        {
            Serial.println("SW4_PIN_SOS is pressed");
            return 3;
        }
        if (readSwitch(SW5_PIN))
        {
            Serial.println("SW5_PIN is pressed");
            return 4;
        }
        delay(100);
    }
}

