// #include <header.h>
// #include <fileManagement.cpp>

// String findUserIDFromFirbase(const FirebaseData &userData)
// {
//     if (Firebase.RTDB.getString(&userData, "/device/"+DEVICE_ID+"/user_id"))
//     {
//         if (userData.dataType() == "string")
//             return userData.stringData();
//         else
//             Serial.println("Invalid data type");
//     }
//     return "No user ID found";
// }

// String findUserIDFromSD()
// {
//     // string to json
//     String doc = readFile("/device.txt");
//     if (doc != "")
//     {
//         DynamicJsonDocument doc(1024);
//         deserializeJson(doc, doc);
//         return doc["user_id"].as<String>();
//     }
//     return "ERROR: No user ID found";
// }

// bool getUserDataFormFirebase(FirebaseData &userData, String USER_ID)
// {
//     if (Firebase.RTDB.getJSON(&userData, "/" + USER_ID + "/user"))
//     {
//         if (userData.dataType() == "json")
//             return true;
//         else
//             Serial.println("Invalid data type");
//     }
//     else
//         Serial.println(userData.errorReason());
//     return false;
// }

