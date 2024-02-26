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
    rgb(100, 150, 100);
    delay(500);
    rgb(50, 255, 20);
    delay(500);
    rgb(0, 0, 255);
    delay(500);
    rgb(0, 0, 0);
    delay(500);
    rgb(255, 255, 255);
    delay(500);
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
