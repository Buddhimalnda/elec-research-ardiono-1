#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
// #include <FreeRTOS.h>

// MPU6050 Sensor Setup
#define MPU_SDA 21
#define MPU_SCL 22
Adafruit_MPU6050 mpu;

// WiFi Credentials
const char *ssid = "SLT-4G_163BEA";
const char *password = "751FCEED";

// Function prototypes
void TaskSensorRead(void *pvParameters);
void TaskWifiManage(void *pvParameters);

void setup()
{
    Serial.begin(115200);

    // Initialize MPU6050
    if (!mpu.begin(MPU6050_I2CADDR_DEFAULT, &Wire))
    {
        Serial.println("Sensor init failed");
        while (1)
            yield();
    }
    Serial.println("MPU6050 Found!");

    // Create FreeRTOS tasks
    xTaskCreate(
        TaskSensorRead, /* Task function */
        "SensorRead",   /* Name of the task */
        2048,           /* Stack size of task */
        NULL,           /* parameter of the task */
        1,              /* priority of the task */
        NULL);          /* Task handle to keep track of created task */

    xTaskCreate(
        TaskWifiManage, /* Task function */
        "WifiManage",   /* Name of the task */
        2048,           /* Stack size of task */
        NULL,           /* parameter of the task */
        1,              /* priority of the task */
        NULL);          /* Task handle to keep track of created task */
}

void loop()
{
    // In a multitasking environment, the loop can remain empty.
}

// Task for reading sensor data
void TaskSensorRead(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        /* Sensor reading and processing logic */
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        Serial.print("Acceleration X: ");
        Serial.print(a.acceleration.x);
        Serial.print(", Y: ");
        Serial.print(a.acceleration.y);
        Serial.print(", Z: ");
        Serial.println(a.acceleration.z);

        // Delay for a period of time (e.g., 100 ms)
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// Task for managing WiFi connectivity
void TaskWifiManage(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("Connecting to WiFi...");
            WiFi.begin(ssid, password);

            while (WiFi.status() != WL_CONNECTED)
            {
                vTaskDelay(500 / portTICK_PERIOD_MS);
                Serial.print(".");
            }
            Serial.println("WiFi connected");
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS); // Check every 10 seconds
    }
}
