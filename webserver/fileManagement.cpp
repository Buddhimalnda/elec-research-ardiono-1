// #include <header.h>

// // SD Card
// #include <SD.h>
// #include <SPI.h>

// #define SD_CS 15
// #define SD_MOSI 23
// #define SD_MISO 19
// #define SD_SCK 18

// File file;

// bool initializeSDCard()
// {
//     Serial.println("Initializing SD Card");
//     if (!SD.begin(SD_CS))
//     {
//         Serial.println("SD Card initialization failed"); // SD Card initialization failed
//         return false;
//     }
//     Serial.println("SD Card initialization successful");
//     return true;
// }

// bool checkAndCreateDeviceJson(FirebaseData &fbdo, FirebaseData &userData)
// {
//     if (!SD.exists("/device.txt"))
//     {
//         // Device.json doesn't exist, fetch data from Firebase
//         Serial.println("device.json  doesn't exists");
//     }
//     else
//     {
//         Serial.println("device.json already exists");
//     }
//     return true; // File exists or was successfully created
// }

// // bool fetchDataFromFirebase(FirebaseData &fbdo, FirebaseData &userData, String USER_ID)
// // {
// //     String doc = "";
// //     if (Firebase.RTDB.getString(&fbdo, "/device/" + DEVICE_ID + "/user_id"))
// //     {
// //         if (fbdo.dataType() == "string")
// //         {
// //             FirebaseJson firebaseJson;
// //             Serial.println(fbdo.stringData());
// //             USER_ID = fbdo.stringData();
// //             FirebaseJsonData result;
// //             if (Firebase.RTDB.getJSON(&userData, "/" + USER_ID + "/user", &firebaseJson))
// //             {
// //                 if (userData.dataType() == "json")
// //                 {
// //                     firebaseJson.get(result, "/");
// //                     if (result.success)
// //                     {
// //                         doc = result.stringValue;
// //                         if (writeFile(doc, "/device.txt"))
// //                         {
// //                             Serial.println("Writting success");
// //                             return true;
// //                         }
// //                         else
// //                         {
// //                             Serial.println("Writting error");
// //                             return false;
// //                         }
// //                     }
// //                 }
// //                 else
// //                 {
// //                     Serial.println("Invalid data type");
// //                     return false;
// //                 }
// //             }
// //             else
// //             {
// //                 Serial.println(userData.errorReason());
// //                 return false;
// //             }
// //             return true;
// //         }
// //         else
// //         {
// //             Serial.println("Invalid data type");
// //             return false;
// //         }
// //     }
// //     else
// //     {
// //         Serial.println(fbdo.errorReason());
// //         return false;
// //     }
// //     return true;
// // }

// String readDevice()
// { // Adjust size as needed
//     String doc = "";
//     file = SD.open("/device.txt", FILE_READ);
//     if (!file)
//     {
//         Serial.println("Failed to open device.json for reading");
//         return "";
//         // Return empty document in case of failure
//     }
//     doc += file.read();
//     Serial.println(doc);
//     file.close();
//     return doc;
// }
// String readFile(String path)
// { // Adjust size as needed
//     String doc = "";
//     File file = SD.open(path, FILE_READ);
//     if (!file)
//     {
//         Serial.println("Failed to open device.json for reading");
//         return "";
//         // Return empty document in case of failure
//     }
//     doc += file.read();
//     Serial.println(doc);
//     file.close();
//     return doc;
// }

// bool writeFile(const String &doc, String path)
// {
//     file = SD.open(path, FILE_APPEND);
//     if (!file)
//     {
//         Serial.println("Failed to open device.txt for writing");
//         return false;
//     }
//     file.print(doc);
//     file.close();
//     return true;
// }

// // delete file
// bool deleteFile(const char *path)
// {
//     Serial.printf("Deleting file: %s\n", path);
//     if (SD.exists(path))
//     {
//         SD.remove(path);
//         Serial.println("File deleted");
//         return true;
//     }
//     else
//     {
//         Serial.println("File doesn't exist");
//         return false;
//     }
// }

// // DynamicJsonDocument toJson(String jsonString)
// // {
// //   DynamicJsonDocument docs(1024);
// //   DeserializationError error = deserializeJson(docs, jsonString);

// //   // Test if parsing succeeds
// //   if (error)
// //   {
// //     Serial.print(F("deserializeJson() failed: "));
// //     Serial.println(error.c_str());
// //   }
// //   return docs;
// // }