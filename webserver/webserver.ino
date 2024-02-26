#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

const char *ssid = "SLT-4G_163BEA"; // Replace with your WiFi SSID
const char *password = "751FCEED";  // Replace with your WiFi Password

WebServer server(80); // Object to hold the HTTP server

String logData = ""; // String to store log data

// HTML code for the web page
String getHtmlPage()
{
    String page = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <h1>ESP32 Web Server</h1>
  <h2>Console Output:</h2>
  <pre>)rawliteral";
    page += logData;
    page += R"rawliteral(</pre>
</body>
</html>
)rawliteral";
    return page;
}

void addLogData(const String &message)
{
    // Add new log message to logData
    logData += message + "\n"; // Append new message with a newline
    if (logData.length() > 1000)
    {                                     // Prevent string from becoming too large
        logData = logData.substring(500); // Keep the last part if it's too long
    }
}

void setup()
{
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
        addLogData("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    addLogData("Connected to WiFi");
    Serial.println(WiFi.localIP());
    server.on("/", HTTP_GET, []()
              {
                  server.send(200, "text/html", getHtmlPage()); // Send web page with log data
              });

    server.begin(); // Start the server
}

void loop()
{
    server.handleClient(); // Handle client requests

    // Example of adding data to the log
    // Normally, you would add real log messages here
    static unsigned long lastTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastTime > 5000)
    { // Add new log every 5 seconds
        lastTime = currentTime;
        String message = "Timestamp: " + String(currentTime);
        Serial.println(message);
        addLogData(message);
    }
}
