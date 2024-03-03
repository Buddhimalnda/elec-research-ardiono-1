#define DATA_PIN 33
#define LATCH_PIN 26
#define CLOCK_PIN 27
#define NUM_LEDS 8

// Define your patterns here
uint8_t patterns[] = {
    0b00011000, // Middle LEDs
    0b00111100, // Middle to outer LEDs
    0b01111110, // All except first and last
    0b11111111, // All LEDs on
    0b01111110, // All except first and last
    0b00111100, // Middle to outer LEDs
    0b00011000  // Middle LEDs
};
int numPatterns = sizeof(patterns) / sizeof(patterns[0]);

void setup()
{
    pinMode(DATA_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
}

void loop()
{
    // Display each pattern in the array for a short time
    for (int i = 0; i < numPatterns; i++)
    {
        updateLEDs(patterns[2]);
        delay(250); // Change the speed of the pattern display
    }
}

void updateLEDs(uint8_t leds)
{
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, leds);
    digitalWrite(LATCH_PIN, HIGH);
}
