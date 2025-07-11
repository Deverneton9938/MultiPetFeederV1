#include <Arduino.h>
#include "Feeder_Menu.h"
#include "Feeder_Features.h"
#include <Feeder_Files.h>
#include <Feeder_Wifi.h>

SemaphoreHandle_t lcdMutex;
SemaphoreHandle_t stepperMutex;
SemaphoreHandle_t rfidMutex;
SemaphoreHandle_t stateMutex; // Mutex for shared flags like rfidEnabled, tempHumidEnabled, manualFeedingEnabled, inManualFeedingMode
SemaphoreHandle_t servoMutex;
TaskHandle_t manualFeedingTaskHandle = nullptr;
TaskHandle_t tempHumidTaskHandle = nullptr;
TaskHandle_t servoTaskHandle = nullptr;
QueueHandle_t stepperQueue = nullptr;

const int buttonPins[] = {UP, DOWN, SEL, BACK};

bool lidOpen = false;

volatile size_t authorizedIndex;

// Connect lcd pins to class
LiquidCrystal_I2C lcd(0x27, 20, 4); // This also initiates the I2C bus

// Connect DHT sensor

DHT_Async dht(DHT_PIN, DHT_TYPE_11);

// Connect RFID pins to class
MFRC522DriverPinSimple ss_pin(SS);
MFRC522DriverSPI driver{ss_pin};
MFRC522 rfid{driver};

// Stepper1 motor
AccelStepper stepper1(MOTOR_INTERFACE_TYPE, STEPPER1_IN1, STEPPER1_IN3, STEPPER1_IN2, STEPPER1_IN4);

// RTC

uRTCLib rtc(0x68);



// Define Pet structure (subject to change to include more fields, add/delete more pets dynamically)
/*Pet pets[] =
    {
        {"Meow Meows", "16:00:C3:01", 700, 6, 0, 7, 0, 18, 0, 19, 0, false, false, false, false},
        {"Fluffy", "25:80:18:02", 1024, 7, 0, 8, 0, 19, 0, 20, 0, false, false, false, false},
        {"Jasper", "44:47:A1:F8", 500, 8, 0, 9, 0, 22, 30, 23, 0, false, false, false, false}};*/

  
// Define parameters for RFID task
static FeedingTaskParameters rfidParams = {
    .rfid = &rfid,
    .lcd = &lcd,
    .petList = pets,
    .petCount = &petCount};

static ServoTaskParams servoParams = {
    .rtc = &rtc,
    .pet = pets};

UIMode uiMode = SPLASH_SCREEN;

void menuTask(void *parameter)
{
  LiquidCrystal_I2C *lcd = (LiquidCrystal_I2C *)parameter;

  TickType_t lastActivity = xTaskGetTickCount();
  const TickType_t inactivityTimeout = pdMS_TO_TICKS(10000);

  while (true)
  {

    if (uiMode == SPLASH_SCREEN)
    {
      showSplashScreen(*lcd);

      for (int i = 0; i < 4; i++)
      {
        if (!digitalRead(buttonPins[i])) // If any button is pressed
        {
          uiMode = MENU_ACTIVE; // Switch to menu mode
          isMenuActive = true;
          lastActivity = xTaskGetTickCount(); // Reset last activity time
          displayMenu(*lcd);                  // Display the menu
          break;
        }
      }
    }

    else if (uiMode == MENU_ACTIVE)
    {
      TickType_t before = xTaskGetTickCount();
      handleMenu(*lcd);
      TickType_t after = xTaskGetTickCount();

      if (isMenuActive)
      {
        lastActivity = after;
      }

      if (after - lastActivity > inactivityTimeout)
      {
        uiMode = SPLASH_SCREEN;
        isMenuActive = false;
      }
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void feedingTask(void *pvParameters)
{
  FeedingTaskParameters *params = (FeedingTaskParameters *)pvParameters;

  

  MFRC522 *rfid = params->rfid;
  LiquidCrystal_I2C *lcd = params->lcd;
  Pet *petList = params->petList;
  size_t *petCount = params->petCount;

  String uid;
  String currentFeedingUID;
  static bool lidOpen = false;

  while (true)
  {
    bool isRecognizedPet = false;
    size_t recognizedIndex = 0; // Index of the recognized pet in the pets array

    if (readRFID(*rfid, uid))
    {
      for (size_t i = 0; i < *petCount; i++)
      {
        Serial.print("Comparing with pet ");
        Serial.print(petList[i].name);
        Serial.print(" UID: ");
        Serial.println(petList[i].rfidTag);

        if (petList[i].rfidTag == uid)
        {
          Serial.println("Match found!");
          isRecognizedPet = true;

          recognizedIndex = i; // Store the index of the recognized pet

          break;
        }
        if(i == *petCount - 1)
        {
          Serial.println("Tag not recognized! Is the pet added to the feeder?");
          if(xSemaphoreTake(lcdMutex, portMAX_DELAY))
          {
            lcd->clear();
            lcd->setCursor(0,0);
            lcd->print("Pet not recognized!");
            lcd->setCursor(0,1);
            lcd->print("Is the pet added?");
            vTaskDelay(pdMS_TO_TICKS(3000));
            xSemaphoreGive(lcdMutex);
          }

        }
        
      }


      // Unauthorized pet attempt during feeding
      if (lidOpen && uid != currentFeedingUID && isRecognizedPet)
      {
        Serial.println("Unauthorized pet detected, closing lid!");
        xTaskNotifyGive(servoTaskHandle);
        lidOpen = false;
        currentFeedingUID = "";
        isRecognizedPet = false; // Reset recognized pet status
        continue;
      }

      if (isRecognizedPet)
      {
        if (xSemaphoreTake(stateMutex, portMAX_DELAY))
        {
          authorizedIndex = recognizedIndex;
          xSemaphoreGive(stateMutex);
        }

        Pet &pet = petList[authorizedIndex];
        FeedingWindow window = getActiveFeedingWindow(pet);

        if ((window == WINDOW_1 && !pet.hasEatenWindow1) ||
            (window == WINDOW_2 && !pet.hasEatenWindow2))
        {

          if (!lidOpen)
          {
            currentFeedingUID = uid;
            lidOpen = true;
            xTaskNotifyGive(servoTaskHandle);
          }
          else
          {
            if (uid == currentFeedingUID)
            {
            }
          }

          isMenuActive = true;

          if (xSemaphoreTake(lcdMutex, portMAX_DELAY))
          {
            lcd->clear();
            lcd->setCursor(0, 0);
            lcd->print("Pet: ");
            lcd->print(pet.name);
            lcd->setCursor(0, 1);
            lcd->print("Food: ");
            lcd->print(pet.stepsToDispense);
          }

          if ((window == WINDOW_1 && !pet.stepperDispensed1) ||
              (window == WINDOW_2 && !pet.stepperDispensed2))
          {
            StepperCommand cmd = {pet.stepsToDispense};
            xQueueSend(stepperQueue, &cmd, portMAX_DELAY);

            if (window == WINDOW_1)
              pet.stepperDispensed1 = true;
            else if (window == WINDOW_2)
              pet.stepperDispensed2 = true;
          }

          vTaskDelay(pdMS_TO_TICKS(3000));
          xSemaphoreGive(lcdMutex);
          isMenuActive = false;

          if (window == WINDOW_1 && (((uint16_t)(rtc.hour() * 60 + rtc.minute())) >= ((pet.startHour1 * 60 + pet.startMinute1) + (pet.endHour1 * 60 + pet.endMinute1) / 2)))
            pet.hasEatenWindow1 = true;
          else if (window == WINDOW_2 && (((uint16_t)(rtc.hour() * 60 + rtc.minute())) >= ((pet.startHour2 * 60 + pet.startMinute2) + (pet.endHour2 * 60 + pet.endMinute2) / 2)))
            pet.hasEatenWindow2 = true;
        }
        else if (window == NONE ||
                 (window == WINDOW_1 && pet.hasEatenWindow1) ||
                 (window == WINDOW_2 && pet.hasEatenWindow2))
        {
          if (xSemaphoreTake(lcdMutex, portMAX_DELAY))
          {
            lcd->clear();
            lcd->setCursor(0, 0);
            lcd->print("Pet: ");
            lcd->print(pet.name);
            lcd->setCursor(0, 1);
            lcd->print(window == NONE ? "Not feeding now!" : "Already fed!");
            vTaskDelay(pdMS_TO_TICKS(3000)); // Show message for 2 seconds
            xSemaphoreGive(lcdMutex);
          }

          vTaskDelay(pdMS_TO_TICKS(3000));
        }
      }

      if (xSemaphoreTake(stateMutex, portMAX_DELAY))
      {
        showingMessage = true;
        xSemaphoreGive(stateMutex);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void stepperTask(void *parameter)
{
  StepperCommand cmd;

  while (true)
  {
    if (xQueueReceive(stepperQueue, &cmd, portMAX_DELAY) == pdTRUE)
    {
      if (xSemaphoreTake(stepperMutex, portMAX_DELAY))
      {
        stepper1.setMaxSpeed(1000);

        stepper1.setSpeed(400);
        stepper1.setAcceleration(200);
        stepper1.move(cmd.steps);
        while (stepper1.distanceToGo() != 0) // Wait until the stepper has moved back to the initial position
        {
          stepper1.run();                     // Run the stepper to move it
          vTaskDelay(1 / portTICK_PERIOD_MS); // Small delay to allow other tasks to run
        }

        stepper1.setSpeed(400);
        stepper1.setAcceleration(200);
        stepper1.move(-cmd.steps / 2);
        while (stepper1.distanceToGo() != 0) // Wait until the stepper has moved back to the initial position
        {
          stepper1.run();                     // Run the stepper to move it
          vTaskDelay(1 / portTICK_PERIOD_MS); // Small delay to allow other tasks to run
        }
        xSemaphoreGive(stepperMutex); // Release the mutex after moving the stepper
      }
    }
  }
}

// may need to move servo task here

void resetFeedingWindows(void *parameter)
{
  while (true)
  {
    rtc.refresh();
    uint16_t now = rtc.hour() * 60 + rtc.minute();

    for (size_t i = 0; i < sizeof(pets) / sizeof(Pet); i++)
    {
      Pet &pet = pets[i];

      uint16_t end1 = pet.endHour1 * 60 + pet.endMinute1;
      uint16_t end2 = pet.endHour2 * 60 + pet.endMinute2;

      if (now >= end1 && pet.hasEatenWindow1)
      {
        pet.hasEatenWindow1 = false;   // Reset window 1
        pet.stepperDispensed1 = false; // Reset stepper dispensed flag
      }

      if (now >= end2 && pet.hasEatenWindow2)
      {
        pet.hasEatenWindow2 = false;   // Reset window 2
        pet.stepperDispensed2 = false; // Reset stepper dispensed flag
      }
    }

    vTaskDelay(pdMS_TO_TICKS(60000)); // Check every minute
  }
}

void setup()
{

  Serial.begin(115200);

  lcdMutex = xSemaphoreCreateMutex();
  stepperMutex = xSemaphoreCreateMutex();
  rfidMutex = xSemaphoreCreateMutex();
  stateMutex = xSemaphoreCreateMutex(); // Mutex for shared flags

  initMenuSystem(lcd, buttonPins);
  rfid.PCD_Init();                                          // Create MFRC522 instance
  SPI.begin();                                              // Initialize SPI bus for RFID
  rfid.PCD_SetAntennaGain(MFRC522::PCD_RxGain::RxGain_max); // Optional, boosts antenna power
  SPI.setFrequency(13560000);                               // Set SPI frequency for RFID (13.56 MHz)

  loadPets();

  //setRTCTimeFromCompile();

  TempHumidParams *tempHumidParams = new TempHumidParams{&dht, &lcd};
  ManualFeedingParams *manualFeedingParams = new ManualFeedingParams{&lcd, &stepper1};

  stepperQueue = xQueueCreate(5, sizeof(StepperCommand)); // Stepper command queue

  xTaskCreatePinnedToCore(
      menuTask,    // Task function
      "Menu Task", // Name of the task
      4096,        // Stack size in bytes
      &lcd,        // Parameter to pass to the task
      7,           // Task priority
      NULL,        // Task handle
      1            // Core ID (0 for core 0)
  );

  xTaskCreatePinnedToCore(
      tempAndHumidTask,        // Task function
      "Temp/Humid Task",       // Name of the task
      2048,                    // Stack size in bytes
      (void *)tempHumidParams, // Parameter to pass to the task
      2,                       // Task priority
      &tempHumidTaskHandle,    // Task handle
      1                        // Core ID (0 for core 0)
  );

  xTaskCreatePinnedToCore(
      manualFeedingTask,           // Task function
      "Manual Feeding Task",       // Name of the task
      2048,                        // Stack size in bytes
      (void *)manualFeedingParams, // Parameter to pass to the task
      3,                           // Task priority
      &manualFeedingTaskHandle,    // Task handle
      1                            // Core ID (0 for core 0)
  );

  xTaskCreatePinnedToCore(
      feedingTask,    // Task function
      "Feeding Task", // Name of the task
      2048,           // Stack size in bytes
      &rfidParams,    // Parameter to pass to the task
      4,              // Task priority
      NULL,           // Task handle
      1               // Core ID (0 for core 0)
  );

  xTaskCreatePinnedToCore(
      stepperTask,    // Task function
      "Stepper Task", // Name of the task
      1024,           // Stack size in bytes
      NULL,           // Parameter to pass to the task
      5,              // Task priority
      NULL,           // Task handle
      1               // Core ID (0 for core 0)
  );

  xTaskCreatePinnedToCore(
      servoTask,    // Task function
      "Servo Task", // Name of the task
      2048,         // Stack size in bytes
      &servoParams, // Parameter to pass to the task
      6,
      &servoTaskHandle, // Task handle
      1                 // Core ID (0 for core 0)

  );

  xTaskCreatePinnedToCore(
      resetFeedingWindows,     // Task function
      "Reset Feeding Windows", // Name of the task
      2048,                    // Stack size in bytes
      NULL,                    // Parameter to pass to the task
      1,                       // Task priority
      NULL,                    // Task handle
      1                        // Core ID (0 for core 0)
  );

  xTaskCreatePinnedToCore(
    wifiTask,
    "WiFi Task",
    4096,
    &rtc, //pass rtc to sync clock
    1,
    NULL,
    0
  );

}

void loop()
{
}
