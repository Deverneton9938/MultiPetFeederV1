/* temp_and_humid_fn->cpp */
#include "Feeder_Features.h"



void tempAndHumidTask(void *parameter) 
{
  TempHumidParams *params = (TempHumidParams *)parameter;

  LiquidCrystal_I2C *lcd = params->lcd;
  DHT_Async *dht = params->dht;

 
 
  float temp, humid;
  TickType_t lastReadTime = xTaskGetTickCount();

  while(true)
  {

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for notification to start measuring temperature and humidity

    bool tempEnabled = false;
    bool menuActive = false;

    if(xSemaphoreTake(stateMutex, portMAX_DELAY)) // Take the mutex before accessing shared state
    {
      tempEnabled = tempHumidEnabled; // Store the current state of temperature and humidity feature
      menuActive = isMenuActive; // Store the current state of menu activity
      xSemaphoreGive(stateMutex); // Release the mutex after accessing the state
    }



    if(tempHumidEnabled && !isMenuActive) // Only measure if feature is enabled and menu is not active
    {
      if(xTaskGetTickCount() - lastReadTime >= pdMS_TO_TICKS(2000))
      {
        if(dht->measure(&temp, &humid, true)) 
        {
        if(xSemaphoreTake(lcdMutex, portMAX_DELAY)) // Take the mutex before accessing the LCD
          {
            lcd->clear();
            lcd->setCursor(0, 0);
            lcd->print("Temp: ");
            lcd->print(dht->convertCtoF(temp-3));
            lcd->print("F");
            lcd->setCursor(0, 1);
            lcd->print("Humid: ");
            lcd->print(humid);
            lcd->print("%");
            xSemaphoreGive(lcdMutex); // Release the mutex after updating the LCD
          }

        }        

      }
      
    }
    vTaskDelay(50 / portTICK_PERIOD_MS); // Delay for 5 seconds before next measurement
  }
  
}



