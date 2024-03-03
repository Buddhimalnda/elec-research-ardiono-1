#include <Arduino.h>
#define RGB_R 14
#define RGB_G 12
#define RGB_B 13

void setup()
{
    initializeRGBLEDs();
}

void loop()
{
    rgb(255, 255, 255);
}
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
