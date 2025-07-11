#ifndef FEEDER_FEATURES_H
#define FEEDER_FEATURES_H

#include <Pet.h>
#include <Feeder_Files.h>
#include <Arduino.h>
#include "Feeder_Menu.h"
#include "DHT_Async.h"
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
// #include <MFRC522DriverI2C.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include <SPI.h> // for RFID (MFRC522) communication {add more here if other SPI devices are used}
#include <Stepper.h>
#include <AccelStepper.h>


// Pin Constants

// SPI pins
#define MOSI 23
#define MISO 19
#define SCK 18
#define SS 5

// I2C pins
#define SDA 24
#define SCL 22

#define UP 32
#define DOWN 33
#define SEL 25
#define BACK 26

#define DHT_PIN 27

// Stepper 1 motor pins and interface
#define MOTOR_INTERFACE_TYPE AccelStepper::HALF4WIRE // Use HALF4WIRE for 28BYJ-48 stepper motor

#define STEPPER1_IN1 14
#define STEPPER1_IN2 12
#define STEPPER1_IN3 4
#define STEPPER1_IN4 15
#define STEPS_PER_REV 4096 // 2048 steps for 28BYJ-48 stepper motor with 64:1 gear ratio

#define SERVO1_PIN 17    // Pin for servo motor
#define SERVO1_CHANNEL 0 // Servo channel
#define SERVO1_TIMER LEDC_TIMER_1
#define SERVO_FREQ 50
#define SERVO1_OPEN_ANGLE 180
#define SERVO1_CLOSED_ANGLE 0
#define SERVO1_STEP_DELAY 30
#define SERVO_STEP_SIZE 5
#define SERVO_OPEN_PAUSE 20000

extern SemaphoreHandle_t lcdMutex;
extern SemaphoreHandle_t stepperMutex;
extern SemaphoreHandle_t rfidMutex;
extern SemaphoreHandle_t stateMutex; // Mutex for shared flags like rfidEnabled, tempHumidEnabled, manualFeedingEnabled, inManualFeedingMode
extern SemaphoreHandle_t servoMutex;

extern TaskHandle_t manualFeedingTaskHandle;
extern TaskHandle_t tempHumidTaskHandle;
extern TaskHandle_t servoTaskHandle;

extern QueueHandle_t stepperQueue;

void tempAndHumidTask(void *parameter);

bool readRFID(MFRC522 &rfid, String &uidStr);

void stepper1Move(int steps);

void dispenseFood(int steps);

void openCloseServo();

void manualFeedingTask(void *parameter);

void stepperTask(void *parameter);

void servoTask(void *parameter);

/*struct Pet
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
}; */

struct FeedingTaskParameters
{
    MFRC522 *rfid;
    LiquidCrystal_I2C *lcd;
    Pet *petList;
    size_t *petCount;
};

typedef struct
{
    DHT_Async *dht;
    LiquidCrystal_I2C *lcd;
} TempHumidParams;

typedef struct
{
    LiquidCrystal_I2C *lcd;
    AccelStepper *stepper1;
} ManualFeedingParams;

typedef struct
{
    int steps;
} StepperCommand;

typedef struct
{
    uRTCLib *rtc;
    Pet *pet;
} ServoTaskParams;

enum FeedingWindow
{
    NONE,
    WINDOW_1,
    WINDOW_2
};

FeedingWindow getActiveFeedingWindow(const Pet &pet);

/* TO DO
void manualFeeding();
*/
extern bool tempHumidEnabled;
extern bool manualFeedingEnabled;
extern bool inManualFeedingMode;
extern bool rfidEnabled;
extern bool lidOpen;
extern DHT_Async dht;
extern uRTCLib rtc;
extern MFRC522DriverPinSimple ss_pin;
extern MFRC522DriverSPI driver;
extern MFRC522 rfid;
// extern Stepper stepper1;
extern AccelStepper stepper1;

extern enum FeedingWindow window;
extern volatile size_t authorizedIndex;
extern volatile bool manualOverride;

#endif