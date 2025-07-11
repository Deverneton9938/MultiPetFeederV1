#ifndef FEEDER_FILES_H
#define FEEDER_FILES_H 


#include <Feeder_Menu.h>
#include <LittleFS.h>
#include <FS.h>

#define MAX_PETS 8

void loadPets();

void savePets();



extern Pet pets[MAX_PETS]; // Array to hold pet data
extern size_t petCount;








#endif
