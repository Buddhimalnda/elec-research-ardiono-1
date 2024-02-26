/*
 * This program Print Numbers on 3, 7segment display
 * Hardware Connections (Breakoutboard to Arduino Nano):
 * Vin  - 5V (3.3V is allowed)
 * GND - GNDs
 * 74HC595 ST_CP - 4 (ESP32)
 * 74HC595 SH_CP - 12 (ESP32)
 * 74HC595 DS - 14 (ESP32)
 * 
 */
// set the 7segment type (common Cathode or Anode)
const bool commonCathode = true; // I'm using common Cathode 7segment if you use common Anode then change the value into false.
// alpha-digit pattern for a 7-segment display
const byte digit_pattern[17] =
{
  // 74HC595 Outpin Connection with 7segment display.
  // Q0 Q1 Q2 Q3 Q4 Q5 Q6 Q7
  // a  b  c  d  e  f  g  DP
  0b11111100,  // 0
  0b01100000,  // 1
  0b11011010,  // 2
  0b11110010,  // 3
  0b01100110,  // 4
  0b10110110,  // 5
  0b10111110,  // 6
  0b11100000,  // 7
  0b11111110,  // 8
  0b11110110,  // 9
  0b11101110,  // A
  0b00111110,  // b
  0b00011010,  // C
  0b01111010,  // d
  0b10011110,  // E
  0b10001110,  // F
  0b00000001   // .
};
//Pin connected to ST_CP of 74HC595
int latchPin = 26;
//Pin connected to SH_CP of 74HC595
int clkPin = 27;
//Pin connected to DS of 74HC595
int dtPin = 33;
// display value
int dispVal = 0;
bool increment = true;
void setup() {
  // put your setup code here, to run once:
  // set the serial port at 115200
  Serial.begin(115200);
  delay(1000);   
  // set the 74HC595 Control pin as output
  pinMode(latchPin, OUTPUT);    //ST_CP of 74HC595
  pinMode(clkPin, OUTPUT);      //SH_CP of 74HC595
  pinMode(dtPin, OUTPUT);       //DS of 74HC595
}
void loop() {
  // put your main code here, to run repeatedly:  
  int dispDigit1=dispVal/10;
  int dispDigit2=dispVal%10;  
  if(increment==true){
    printf("%d%d.\n", dispDigit1,dispDigit2);    
    digitalWrite(latchPin, LOW);
    if(commonCathode == true){
      shiftOut(dtPin, clkPin, LSBFIRST, digit_pattern[dispDigit2]|digit_pattern[16]);   // 1. (Digit+DP)
      shiftOut(dtPin, clkPin, LSBFIRST, digit_pattern[dispDigit1]);
    }else{
      shiftOut(dtPin, clkPin, LSBFIRST, ~(digit_pattern[dispDigit2]|digit_pattern[16]));   // 1. (Digit+DP)
      shiftOut(dtPin, clkPin, LSBFIRST, ~(digit_pattern[dispDigit1]));
    }
    digitalWrite(latchPin, HIGH);    
    dispVal += 1;
    if (dispVal == 99){
      increment=false;
    }
  }else{
    printf("%d%d.\n", dispDigit1,dispDigit2);
    digitalWrite(latchPin, LOW);
    if(commonCathode == true){
      shiftOut(dtPin, clkPin, LSBFIRST, digit_pattern[dispDigit2]|digit_pattern[16]);   // 1. (Digit+DP)
      shiftOut(dtPin, clkPin, LSBFIRST, digit_pattern[dispDigit1]);
    }else{
      shiftOut(dtPin, clkPin, LSBFIRST, ~(digit_pattern[dispDigit2]|digit_pattern[16]));   // 1. (Digit+DP)
      shiftOut(dtPin, clkPin, LSBFIRST, ~(digit_pattern[dispDigit1]));
    }
    digitalWrite(latchPin, HIGH);   
    dispVal -= 1;
    if (dispVal == 0){
      increment=true;
    }
  }
  delay(250);
}