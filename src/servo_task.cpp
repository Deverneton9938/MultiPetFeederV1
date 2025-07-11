#include <Feeder_Features.h>
#include <Feeder_Files.h>

// Function to convert angle to duty cycle for servo motor (16 bit resolution)
uint32_t angleToDutyCycle(uint8_t angle)
{
  uint32_t minDuty = 3276;
  uint32_t maxDuty = 6553;

  return map(angle, 0, 180, minDuty, maxDuty);
}

void servoTask(void *parameter)
{

  ledcSetup(SERVO1_CHANNEL, SERVO_FREQ, 16); // Setup the servo channel with frequency and resolution

  uint8_t angle = SERVO1_CLOSED_ANGLE;

  ServoTaskParams *params = (ServoTaskParams *)parameter;
  uRTCLib *rtc = params->rtc;
  Pet *petList = params->pet;

  

  while (true)
  {

    if(manualOverride == true)
    {
      Serial.println("[ServoTask] Manual feeding: opening lid");
      ledcAttachPin(SERVO1_PIN, SERVO1_CHANNEL);                        // Attach the servo pin to the channel
      ledcWrite(SERVO1_CHANNEL, angleToDutyCycle(SERVO1_CLOSED_ANGLE)); // Write the initial duty cycle to the servo channel
      angle = SERVO1_CLOSED_ANGLE;

      while (angle < SERVO1_OPEN_ANGLE)
      {

        angle += SERVO_STEP_SIZE;

        if (angle > SERVO1_OPEN_ANGLE)
          angle = SERVO1_OPEN_ANGLE;

        ledcWrite(SERVO1_CHANNEL, angleToDutyCycle(angle)); // Write the duty cycle to the servo channel

        vTaskDelay(pdMS_TO_TICKS(SERVO1_STEP_DELAY)); // Delay for servo step size
      }

      const uint32_t manualOpenDuration = 5 * 60 * 1000; //5 minutes

      uint32_t startTime = millis();

      while(millis() - startTime < manualOpenDuration)
      {
        if(ulTaskNotifyTake(pdTRUE, 0))
        {
          Serial.println("[ServoTask] Force close during manual override");
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
      }

      Serial.println("[ServoTask] Manual overrid done: closing lid");

      while (angle > SERVO1_CLOSED_ANGLE)
      {

        angle -= SERVO_STEP_SIZE;

        if (angle < SERVO1_CLOSED_ANGLE)
          angle = SERVO1_CLOSED_ANGLE;

        ledcWrite(SERVO1_CHANNEL, angleToDutyCycle(angle)); // Write the duty cycle to the servo channel

        vTaskDelay(pdMS_TO_TICKS(SERVO1_STEP_DELAY)); // Delay for servo step size
      }

      ledcDetachPin(SERVO1_PIN); // Detach the servo pin when done

      manualOverride = false;

    }

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for notification to start servo operation

    rtc->refresh();
    uint16_t now = rtc->hour() * 60 + rtc->minute();

    FeedingWindow window = getActiveFeedingWindow(petList[authorizedIndex]);
    uint16_t startMin = 0, endMin = 0;

    if (window == WINDOW_1)
    {
      startMin = petList[authorizedIndex].startHour1 * 60 + petList[authorizedIndex].startMinute1;
      endMin = petList[authorizedIndex].endHour1 * 60 + petList[authorizedIndex].endMinute1;
    }
    else if (window == WINDOW_2)
    {
      startMin = petList[authorizedIndex].startHour2 * 60 + petList[authorizedIndex].startMinute2;
      endMin = petList[authorizedIndex].endHour2 * 60 + petList[authorizedIndex].endMinute2;
    }
    else
    {
      Serial.println("[ServoTask] Invalid window! Will not open lid.");
      continue; // Skip lid open
    }

    uint16_t windowHalfTime = (startMin + endMin) / 2;

    if (now >= windowHalfTime || now >= endMin)
    {
      Serial.printf("[ServoTask] Time=%d >= HalfTime=%d or End=%d. Will not open lid.\n", now, windowHalfTime, endMin);
      continue; // Also skip lid open
    }

    ledcAttachPin(SERVO1_PIN, SERVO1_CHANNEL);                        // Attach the servo pin to the channel
    ledcWrite(SERVO1_CHANNEL, angleToDutyCycle(SERVO1_CLOSED_ANGLE)); // Write the initial duty cycle to the servo channel
    angle = SERVO1_CLOSED_ANGLE;

    while (angle < SERVO1_OPEN_ANGLE)
    {

      angle += SERVO_STEP_SIZE;

      if (angle > SERVO1_OPEN_ANGLE)
        angle = SERVO1_OPEN_ANGLE;

      ledcWrite(SERVO1_CHANNEL, angleToDutyCycle(angle)); // Write the duty cycle to the servo channel

      vTaskDelay(pdMS_TO_TICKS(SERVO1_STEP_DELAY)); // Delay for servo step size
    }

    TickType_t openStart = xTaskGetTickCount();

    bool forceClose = false;

    while (true)
    {
      rtc->refresh();
      now = rtc->hour() * 60 + rtc->minute(); // Get current time in minutes

      FeedingWindow window = getActiveFeedingWindow(petList[authorizedIndex]);

      if (window == WINDOW_1)
      {
        startMin = petList[authorizedIndex].startHour1 * 60 + petList[authorizedIndex].startMinute1;
        endMin = petList[authorizedIndex].endHour1 * 60 + petList[authorizedIndex].endMinute1;
      }
      else if (window == WINDOW_2)
      {
        startMin = petList[authorizedIndex].startHour2 * 60 + petList[authorizedIndex].startMinute2;
        endMin = petList[authorizedIndex].endHour2 * 60 + petList[authorizedIndex].endMinute2;
      }
      else
      {
        // Invalid window, close immediately
        Serial.println("No valid window, closing lid immediately.");
        break;
      }

      windowHalfTime = (startMin + endMin) / 2; // Calculate half of the feeding window time in minutes

      if (now >= windowHalfTime)
      {
        break;
      }

      if (now >= endMin)
      {
        break;
      }

      if (ulTaskNotifyTake(pdTRUE, 0))
      {
        forceClose = true;
        break;
      }

      vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 1 second before checking the time again
    }

    while (angle > SERVO1_CLOSED_ANGLE)
    {

      angle -= SERVO_STEP_SIZE;

      if (angle < SERVO1_CLOSED_ANGLE)
        angle = SERVO1_CLOSED_ANGLE;

      ledcWrite(SERVO1_CHANNEL, angleToDutyCycle(angle)); // Write the duty cycle to the servo channel

      vTaskDelay(pdMS_TO_TICKS(SERVO1_STEP_DELAY)); // Delay for servo step size
    }

    ledcDetachPin(SERVO1_PIN); // Detach the servo pin when done
  }
}
