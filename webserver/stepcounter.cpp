// #include <header.h>
// #include <Adafruit_MPU6050.h>
// #include <Adafruit_Sensor.h>

// #define EEPROM_SIZE 1

// // MPU
// #define MPU_SDA 21
// #define MPU_SCL 22

// Adafruit_MPU6050 mpu;
// float accelX, accelY, accelZ;

// float accMagnitudePrev = 0;
// // unsigned long lastStepTime = 0;
// int stepCount = 0;
// float stepThreshold = 1.2; // Experimentally determined threshold for step detection
// // float debounceTime = 250;  // Minimum time between steps (milliseconds)

// //-------------------------
// // void setup()
// // {
// //     Serial.begin(115200);
// //     initializeMPU6050();
// //     EEPROM.begin(EEPROM_SIZE);
// // }
// // void loop()
// // {
// //     detectStep();
// //     displayStepCount();
// //     delay(100);
// // }

// void initializeMPU6050()
// {
//     Wire.begin(MPU_SDA, MPU_SCL);
//     if (!mpu.begin())
//     {
//         Serial.println("Failed to find MPU6050 chip");
//         while (1)
//         {
//             delay(10);
//         }
//     }
//     Serial.println("MPU6050 Found!");
//     mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
//     mpu.setGyroRange(MPU6050_RANGE_250_DEG);
//     mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
// }


// bool readMpuData()
// {
//     sensors_event_t a, g, temp;
//     mpu.getEvent(&a, &g, &temp);
//     accelX = a.acceleration.x;
//     accelY = a.acceleration.y;
//     accelZ = a.acceleration.z;
//     return true;
// }

// void detectStep()
// {
//     if (readMpuData())
//     {
//         float accX = accelX / 10;
//         float accY = accelY / 10;
//         float accZ = accelZ / 10;

//         float accMagnitude = sqrt(accX * accX + accY * accY + accZ * accZ);

//         if ((accMagnitudePrev > accMagnitude + 0.1) && (accMagnitudePrev > stepThreshold))
//         {
//             stepCount++;
//             EEPROM.writeInt(0, stepCount);
//             EEPROM.commit();
//             delay(400);
//         }
//         accMagnitudePrev = accMagnitude;
//     }
//     else
//     {
//         Serial.println("Error reading MPU data");
//     }
// }

// // void saveStepCount()
// // {
// //     // Save stepCount to EEPROM
    
// //     // delay(100);
// // }

// void displayStepCount()
// {
//     Serial.print("Steps: ");
//     Serial.println(EEPROM.readInt(0));
// }
