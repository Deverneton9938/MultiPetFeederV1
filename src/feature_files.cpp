/* This file initializes file system for JSON files, which will hold pet feeder data
*  like pet names, feeding times, and telemetry data like temperature and humidity.
*/

#include <Pet.h>
#include "Feeder_Menu.h"
#include "Feeder_Features.h"
#include "Feeder_Files.h"
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

Pet pets[MAX_PETS]; // Array to hold pet data
size_t petCount = 0;

void loadPets()
{
    if(!LittleFS.begin())
    {
        Serial.println("Failed to mount LittleFS");
        return;
    }


    if(!LittleFS.exists("/pets.json"))
    {
        Serial.println("pets.json does not exist!");
        return;
    }

    File file = LittleFS.open("/pets.json", "r");

    if(!file)
    {
        Serial.println("Failed to open pets.json");
        return;
    }

    JsonDocument doc;
    doc.to<JsonArray>();
    DeserializationError error = deserializeJson(doc, file);

    if(error)
    {
        Serial.print("Failed to parse pets.json: ");
        Serial.println(error.c_str());
        file.close();
        return;
    }

    petCount = doc.size();
    Serial.printf("%d\n", petCount);
    for(size_t i = 0; i < petCount; i++)
    {
        JsonObject petObj = doc[i];
        pets[i].name = petObj["name"].as<String>();
        pets[i].rfidTag = petObj["rfidTag"].as<String>();
        pets[i].stepsToDispense = petObj["stepsToDispense"].as<int>();

        JsonArray windows = petObj["feedingWindows"];
        pets[i].startHour1 = windows[0]["startHour1"];
        pets[i].startMinute1 = windows[0]["startMinute1"];
        pets[i].endHour1 = windows[0]["endHour1"];
        pets[i].endMinute1 = windows[0]["endMinute1"];
        pets[i].startHour2 = windows[1]["startHour2"];
        pets[i].startMinute2 = windows[1]["startMinute2"];
        pets[i].endHour2 = windows[1]["endHour2"];
        pets[i].endMinute2 = windows[1]["endMinute2"];

        pets[i].hasEatenWindow1 = petObj["hasEatenWindow1"].as<bool>();
        pets[i].hasEatenWindow2 = petObj["hasEatenWindow2"].as<bool>();
        pets[i].stepperDispensed1 = petObj["stepperDispensed1"].as<bool>();
        pets[i].stepperDispensed2 = petObj["stepperDispensed2"].as<bool>();

    }

    file.close();
    Serial.printf("Loaded %d pets from pets.json\n", petCount);



}

void savePets()
{

    

    JsonDocument doc;
    doc.to<JsonArray>();


    JsonArray root = doc.as<JsonArray>();

    for (size_t i = 0; i < petCount; i++)
    {
        Pet &pet = pets[i];

        JsonObject petObj = root.add<JsonObject>();
        petObj["name"] = pet.name;
        petObj["rfidTag"] = pet.rfidTag;
        petObj["stepsToDispense"] = pet.stepsToDispense;

        JsonArray windows = petObj["feedingWindows"].to<JsonArray>();

        JsonObject window1 = windows.add<JsonObject>();
        window1["startHour1"] = pet.startHour1;
        window1["startMinute1"] = pet.startMinute1;
        window1["endHour1"] = pet.endHour1;
        window1["endMinute1"] = pet.endMinute1;
        
        JsonObject window2 = windows.add<JsonObject>();
        window2["startHour2"] = pet.startHour2;
        window2["startMinute2"] = pet.startMinute2;
        window2["endHour2"] = pet.endHour2;
        window2["endMinute2"] = pet.endMinute2;

        petObj["hasEatenWindow1"] = pet.hasEatenWindow1;
        petObj["hasEatenWindow2"] = pet.hasEatenWindow2;
        petObj["stepperDispensed1"] = pet.stepperDispensed1;
        petObj["stepperDispensed2"] = pet.stepperDispensed2;   
    }

    File file = LittleFS.open("/pets.json", "w");
    if (!file)
    {
        Serial.println("Failed to open pets.json for writing");
        return;
    }

    if(serializeJsonPretty(doc, file) == 0)
    {
        Serial.println("Failed to write pets.json");
    }
    else
    {
        Serial.println("pets.json saved successfully");
    }

    file.close();


}





