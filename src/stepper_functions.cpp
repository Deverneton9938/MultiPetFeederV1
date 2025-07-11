#include "Feeder_Features.h"

volatile bool manualOverride = false;

void stepper1Move(int steps)
{
    stepper1.setMaxSpeed(1000);     // Set maximum speed for the stepper motor
    stepper1.setAcceleration(1000); // Set acceleration for the stepper motor

    stepper1.setSpeed(800);

    stepper1.move(steps);

    while (stepper1.distanceToGo() != 0) // Wait until the stepper has moved to the target position
    {
        stepper1.run();                     // Run the stepper to move it
        vTaskDelay(1 / portTICK_PERIOD_MS); // Small delay to allow other tasks to run
    }

    stepper1.setSpeed(200);
    stepper1.setAcceleration(250);

    stepper1.move(-steps);

    while (stepper1.distanceToGo() != 0) // Wait until the stepper has moved back to the initial position
    {
        stepper1.run();                     // Run the stepper to move it
        vTaskDelay(1 / portTICK_PERIOD_MS); // Small delay to allow other tasks to run
    }

    stepper1.stop();
}

void stepper1Manual(int steps)
{
    if (xSemaphoreTake(stepperMutex, portMAX_DELAY)) // Take the mutex before accessing the stepper
    {
        stepper1.setMaxSpeed(1000);     // Set speed for manual feeding
        stepper1.setAcceleration(1000); // Set acceleration for manual feeding
        stepper1.setSpeed(800);         // Set speed for manual feeding
        stepper1.move(steps);           // move instead of moveTo for manual feeding, relative movement

        while (stepper1.distanceToGo() != 0) // Wait until the stepper has moved back to the initial position
        {
            stepper1.run();                     // Run the stepper to move it
            vTaskDelay(1 / portTICK_PERIOD_MS); // Small delay to allow other tasks to run
        }

        stepper1.stop();

        xSemaphoreGive(stepperMutex); // Release the mutex after dispensing food
    }
}

void dispenseFood(int steps)
{

    if (xSemaphoreTake(stepperMutex, portMAX_DELAY)) // Take the mutex before accessing the stepper
    {
        stepper1Move(steps);          // Steps for how much food to dispense, different for each pet
        xSemaphoreGive(stepperMutex); // Release the mutex after dispensing food
    }
}

void manualFeedingTask(void *parameter)
{
    

    ManualFeedingParams *params = (ManualFeedingParams *)parameter;
    LiquidCrystal_I2C *lcd = params->lcd;
    AccelStepper *stepper1 = params->stepper1;

    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for notification to start manual feeding

        bool enabled = false;

        if (xSemaphoreTake(stateMutex, portMAX_DELAY))
        {

            enabled = manualFeedingEnabled; // Store the current state of manual feeding feature
            xSemaphoreGive(stateMutex);     // Release the mutex after accessing the state
        }

        if (!enabled)
            continue; // If manual feeding is not enabled, skip the task

        if (xSemaphoreTake(lcdMutex, portMAX_DELAY))
        {
            lcd->clear();
            lcd->setCursor(0, 0);
            lcd->print("Manual Feeding");
            lcd->setCursor(0, 1);
            lcd->print("Dispensing Food...");

            StepperCommand cmd = {STEPS_PER_REV};
            xQueueSend(stepperQueue, &cmd, portMAX_DELAY); // Send command to stepper queue
            manualOverride = true; //Send flag to servo task
            xTaskNotifyGive(servoTaskHandle);

            xSemaphoreGive(lcdMutex); // Release the mutex after updating the LCD
        }

        inManualFeedingMode = false; // Disable manual feeding after use
    }
}
