#ifndef FEEDER_MENU_H
#define FEEDER_MENU_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <uRTCLib.h> // For RTC_DS3231
#include <Wire.h>
#include <Feeder_Features.h>

// Menu struct

struct Menu
{
    const char *title;
    const char *items[4]; // Menu items, max 4
    int itemCount;
    Menu *subMenus[4]; // Set to null if no submenu
    Menu *parent;
};

extern bool isMenuActive;
extern bool showingMessage;
typedef enum
{
    SPLASH_SCREEN,
    MENU_ACTIVE
} UIMode;

extern UIMode uiMode; // Current UI mode

// function prototypes

void initMenuSystem(LiquidCrystal_I2C &lcd, const int *buttonPins);

void handleMenu(LiquidCrystal_I2C &lcd);

void displayMenu(LiquidCrystal_I2C &lcd);

void showSplashScreen(LiquidCrystal_I2C &lcd);

void setRTCTimeFromCompile();

#endif