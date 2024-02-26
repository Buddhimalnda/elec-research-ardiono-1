#include <Arduino.h>

#define DATA_PIN 33
#define LATCH_PIN 26
#define CLOCK_PIN 27
#define NUM_LEDS 8

void initializeShiftRegisty()
{
    pinMode(DATA_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
}

void updateShiftRegister(int error, bool wifi, bool mode, bool activity)
{
    uint8_t data = 0;

    // Set the first 5 bits based on the error code
    data |= (error & 31); // Error is limited to 5 bits (0-31)

    // Set the next three bits based on the status flags
    if (wifi)
        data |= (1 << 5);
    if (mode)
        data |= (1 << 6);
    if (activity)
        data |= (1 << 7);

    // Update the shift register
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, data);
    digitalWrite(LATCH_PIN, HIGH);
}