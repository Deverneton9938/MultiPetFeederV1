/* This file initializes WiFi and MQTT protocols for sending telemetry data from features to web dashboard */


#include <Feeder_Files.h>  // This has your loadPets() / savePets()
#include <Feeder_Features.h>
#include <Feeder_Wifi.h>
#include <ArduinoJson.h>
#include <time.h>

// The same global pets array



// Create the server on port 80

//DNSServer dns;

WiFiManager wm;



WebServer server(80);

// ---------- TIME ----------

//const char *ntpServer = "pool.ntp.org";
const long gmtOffset = -6 * 3600;
const int dst = 3600;




void handleRoot() {
  if (LittleFS.exists("/index.html")) {
    File file = LittleFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  } else {
    server.send(404, "text/plain", "index.html not found");
  }
}

void handleAddPet() {
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "{\"status\":\"No body provided\"}");
    return;
  }

  String body = server.arg("plain");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json", "{\"status\":\"Invalid JSON\"}");
    return;
  }

  if (petCount >= MAX_PETS) {
    server.send(400, "application/json", "{\"status\":\"Max pets reached\"}");
    return;
  }

  Pet &newPet = pets[petCount];
  newPet.name = doc["name"].as<String>();
  newPet.rfidTag = doc["rfidTag"].as<String>();
  newPet.stepsToDispense = doc["stepsToDispense"].as<int>();

  JsonArray windows = doc["feedingWindows"];
  newPet.startHour1 = windows[0]["startHour1"];
  newPet.startMinute1 = windows[0]["startMinute1"];
  newPet.endHour1 = windows[0]["endHour1"];
  newPet.endMinute1 = windows[0]["endMinute1"];
  newPet.startHour2 = windows[1]["startHour2"];
  newPet.startMinute2 = windows[1]["startMinute2"];
  newPet.endHour2 = windows[1]["endHour2"];
  newPet.endMinute2 = windows[1]["endMinute2"];

  newPet.hasEatenWindow1 = false;
  newPet.hasEatenWindow2 = false;
  newPet.stepperDispensed1 = false;
  newPet.stepperDispensed2 = false;

  petCount++;
  savePets();

  server.send(200, "application/json", "{\"status\":\"Pet added\"}");
}

void handleDeletePet() {
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "{\"status\":\"No body provided\"}");
    return;
  }

  String body = server.arg("plain");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json", "{\"status\":\"Invalid JSON\"}");
    return;
  }

  String tagToDelete = doc["rfidTag"].as<String>();
  bool found = false;

  for (size_t i = 0; i < petCount; i++) {
    if (pets[i].rfidTag == tagToDelete) {
      found = true;
      for (size_t j = i; j < petCount - 1; j++) {
        pets[j] = pets[j + 1];
      }
      petCount--;
      break;
    }
  }

  if (found) {
    savePets();
    server.send(200, "application/json", "{\"status\":\"Pet deleted\"}");
  } else {
    server.send(404, "application/json", "{\"status\":\"Pet not found\"}");
  }
}

void handleGetPets() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();

  for (size_t i = 0; i < petCount; i++) {
    JsonObject obj = arr.add<JsonObject>();
    obj["name"] = pets[i].name;
    obj["rfidTag"] = pets[i].rfidTag;
    obj["stepsToDispense"] = pets[i].stepsToDispense;

    JsonArray windows = obj["feedingWindows"].to<JsonArray>();

    JsonObject window1 = windows.add<JsonObject>();
    window1["startHour1"] = pets[i].startHour1;
    window1["startMinute1"] = pets[i].startMinute1;
    window1["endHour1"] = pets[i].endHour1;
    window1["endMinute1"] = pets[i].endMinute1;

    JsonObject window2 = windows.add<JsonObject>();
    window2["startHour2"] = pets[i].startHour2;
    window2["startMinute2"] = pets[i].startMinute2;
    window2["endHour2"] = pets[i].endHour2;
    window2["endMinute2"] = pets[i].endMinute2;
  }

  String jsonStr;
  serializeJsonPretty(doc, jsonStr);
  server.send(200, "application/json", jsonStr);
}

void startWebServer() {
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return;
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/addPet", HTTP_POST, handleAddPet);
  server.on("/deletePet", HTTP_POST, handleDeletePet);
  server.on("/getPets", HTTP_GET, handleGetPets);

  server.begin();
  Serial.println("WebServer started on port 80");
}



void wifiTask(void *parameter)
{

  uRTCLib *rtc = (uRTCLib *)parameter;
    

    //WiFi.mode(WIFI_AP_STA);

    wm.resetSettings();

    

    wm.setConfigPortalBlocking(false);

    wm.setConfigPortalTimeout(300);
    

   wm.autoConnect("PetFeederV1", "87654321");

   while(WiFi.status() != WL_CONNECTED)
   {
      wm.process();
      vTaskDelay(pdMS_TO_TICKS(100));
   }

   
  Serial.println("Connected!");
  configTime(gmtOffset, dst, "pool.ntp.org", "time.nist.gov", "time.google.com");

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) 
  {
    Serial.println("Failed to obtain time from NTP");
  }

  rtc->set(timeinfo.tm_sec, timeinfo.tm_min, timeinfo.tm_hour, timeinfo.tm_wday, timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year % 100);

  Serial.printf("NTP time: %04d-%02d-%02d %02d:%02d:%02d\n",
  timeinfo.tm_year % 100,
  timeinfo.tm_mon + 1,
  timeinfo.tm_mday,
  timeinfo.tm_hour,
  timeinfo.tm_min,
  timeinfo.tm_sec);
  startWebServer();


   



   
    while(true)
    {
      server.handleClient(); 
      vTaskDelay(pdMS_TO_TICKS(50));
    }

}




