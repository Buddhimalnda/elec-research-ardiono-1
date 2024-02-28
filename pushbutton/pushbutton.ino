#include <Arduino.h>

bool state = false;

void setup()
{
  pinMode(33, INPUT);
  pinMode(25, OUTPUT);
  Serial.begin(115200);
  digitalWrite(25, LOW);
}

void loop()
{
    if (digitalRead(33) == HIGH)
    {
        Serial.println("Button pressed");
        state = !state;
        digitalWrite(25, state ? HIGH: LOW);
    }
    else
    {
        Serial.println("Button not pressed");
    }
    delay(1000);
}