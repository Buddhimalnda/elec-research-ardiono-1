// #include <header.h>
// // RTC
// #include <ThreeWire.h>
// #include <RtcDS1302.h>

// #define RTC_CLK 5
// #define RTC_RST 2
// #define RTC_IO 4
// // now time
// RtcDateTime now;
// ThreeWire myWire(RTC_IO, RTC_CLK, RTC_RST); // IO, SCLK, CE
// RtcDS1302<ThreeWire> Rtc(myWire);


// void initializeRTC()
// {
//     Serial.println("Initializing RTC");
//     Rtc.Begin();
//     RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    
//     if (!Rtc.GetIsRunning())
//     {
//         Serial.println("RTC was not actively running, starting now");
//         Rtc.SetIsRunning(true);
//     }
//     now = Rtc.GetDateTime();
//     if (now < compiled)
//     {
//         Serial.println("RTC is older than compile time! Updating");
//         Rtc.SetDateTime(compiled);
//     }
//     else if (now > compiled)
//     {
//         Serial.println("RTC is newer than compile time. Not updating");
//     }
//     else if (now == compiled)
//     {
//         Serial.println("RTC is the same as compile time! Not updating");
//     }
// }
// // print date time
// void printDateTime()
// {
//     now = Rtc.GetDateTime();
//     Serial.print(now.Year(), DEC);
//     Serial.print('/');
//     Serial.print(now.Month(), DEC);
//     Serial.print('/');
//     Serial.print(now.Day(), DEC);
//     Serial.print(" ");
//     Serial.print(now.Hour(), DEC);
//     Serial.print(':');
//     Serial.print(now.Minute(), DEC);
//     Serial.print(':');
//     Serial.print(now.Second(), DEC);
//     Serial.println();
// }