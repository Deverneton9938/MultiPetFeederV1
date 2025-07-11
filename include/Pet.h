#ifndef PET_H
#define PET_H
#include <Arduino.h>

struct Pet
{
    String name;
    
    String rfidTag;
    
    int stepsToDispense;

    uint8_t startHour1;
    uint8_t startMinute1;
    uint8_t endHour1;
    uint8_t endMinute1;

    uint8_t startHour2;
    uint8_t startMinute2;
    uint8_t endHour2;
    uint8_t endMinute2;

    bool hasEatenWindow1;
    bool hasEatenWindow2;

    bool stepperDispensed1;
    bool stepperDispensed2;
};

#endif