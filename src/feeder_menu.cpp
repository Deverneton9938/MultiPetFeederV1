#include "Feeder_Menu.h"
#include "Feeder_Features.h"

const int *buttons;
static bool buttonUpPressed = false;
static bool buttonDownPressed = false;
static bool buttonSelPressed = false;
static bool buttonBackPressed = false;

// Menu variables
static Menu *primeMenu;   // Static prime menu
static Menu *currentMenu; // Pointer to current menu
static int currentMenuIndex = 0;
bool isMenuActive = false;
bool tempHumidEnabled = false;
bool rfidEnabled = true;
bool manualFeedingEnabled = true; // Manual feeding feature toggle
bool inManualFeedingMode = false;  // Manual feeding menu
bool showingMessage = false;       // Flag to indicate if a temporary message is being displayed

// Timeout variables

const unsigned long INACTIVITY_TIMEOUT = 10000; // 10 seconds timeout

// Submenus and main menu

Menu manualFeedingMenu = {
    "Manual Feeding",                     // Title
    {"Feed Now"},                         // Submenu options
    1,                                    // Number of submenu options
    {nullptr, nullptr, nullptr, nullptr}, // SubMenus
    nullptr                               // Parent menu
};

Menu settingsMenu = {
    "Settings",                                               // Title
    {"Toggle Temp/Hum", "Toggle RFID", "Toggle Manual Feed"}, // Submenu options
    3,                                                        // Number of submenu options
    {nullptr, nullptr, nullptr, nullptr},                     // SubMenus
    nullptr                                                   // Parent menu
};

Menu mainMenu = {
    "Main Menu",                                             // Title
    {"Option 1", "Manual Feeding Mode", settingsMenu.title}, // Main menu options
    3,                                                       // Number of main menu options
    {nullptr, &manualFeedingMenu, &settingsMenu},            // SubMenus
    nullptr                                                  // No other menu to go back to from Main Menu
};

// Initialize menu system
void initMenuSystem(LiquidCrystal_I2C &lcd, const int *buttonPins)
{
    buttons = buttonPins;

    // Initialize menu pointers
    primeMenu = &mainMenu;
    currentMenu = primeMenu;

    // Set parent pointers
    mainMenu.parent = nullptr;
    settingsMenu.parent = &mainMenu;
    manualFeedingMenu.parent = &mainMenu;

    for (int i = 0; i < 4; i++)
    {
        pinMode(buttons[i], INPUT_PULLUP);
    }

    // Initialize LCD and display menu
    if (xSemaphoreTake(lcdMutex, portMAX_DELAY)) // Take the mutex before accessing the LCD
    {
        lcd.init();
        lcd.backlight(); // Turn on the backlight
        lcd.noBlink();
        lcd.noCursor();
        xSemaphoreGive(lcdMutex); // Release the mutex after initializing the LCD
    }
    displayMenu(lcd); // Display the initial menu
}

// Start in splash screen mode

void showSplashScreen(LiquidCrystal_I2C &lcd)
{
    rtc.refresh();

    if (xSemaphoreTake(lcdMutex, portMAX_DELAY))
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Pet Feeder V1 by Dev");
        lcd.setCursor(0, 1);

        lcd.print("Date: ");
        lcd.print(rtc.month());
        lcd.print("/");
        lcd.print(rtc.day());
        lcd.print("/");
        lcd.print(rtc.year());

        uint8_t hour = rtc.hour();
        bool isPM = false;
        if (hour >= 12)
        {
            isPM = true;
            if (hour > 12)
                hour -= 12; // Convert to 12-hour format
        }
        else if (hour == 0)
        {
            hour = 12; // Midnight in 12-hour format
        }

        lcd.setCursor(0, 2);
        lcd.print("Time: ");
        lcd.print(hour);
        lcd.print(":");
        if (rtc.minute() < 10)
            lcd.print("0"); // Add leading zero for minutes
        lcd.print(rtc.minute());
        lcd.print(isPM ? " PM" : " AM");

        vTaskDelay(pdMS_TO_TICKS(2000)); // Display for 3 seconds

        xSemaphoreGive(lcdMutex); // Release the LCD mutex
    }
}

void setRTCTimeFromCompile()
{
    const char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    int year, day, hour, min, sec, month = 0;

    char monthStr[4];

    sscanf(__DATE__, "%s %d %d", monthStr, &day, &year);
    for (int i = 0; i < 12; i++)
    {
        if (strcmp(monthStr, months[i]) == 0)
        {
            month = i + 1; // Month is 1-indexed
            break;
        }
    }

    sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec);
    rtc.set(sec, min, hour, 1, day, month, year % 100); // Set RTC time from compile time
    rtc.refresh();
}

// Display menu
void displayMenu(LiquidCrystal_I2C &lcd)
{
    if (xSemaphoreTake(lcdMutex, portMAX_DELAY))
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(currentMenu->title); // Display menu title

        // Display selected item and next item
        lcd.setCursor(0, 1);
        lcd.print(currentMenu->items[currentMenuIndex]);
        xSemaphoreGive(lcdMutex); // Release the LCD mutex
    }
}

// Handle menu
void handleMenu(LiquidCrystal_I2C &lcd)
{

    static TickType_t lastUpdateTime = 0;   // Tracks the last time a temporary message was displayed
    static TickType_t lastActivityTime = 0; // Tracks the last time a button was pressed
    // static bool showingMessage = false;     // Tracks if a temporary message is being displayed
    bool activityDetected = false; // Tracks if any button activity occurred

    // Check if a temporary message is being displayed
    if (showingMessage)
    {
        if (xTaskGetTickCount() - lastUpdateTime >= pdMS_TO_TICKS(2000))
        {                           // Wait 2 seconds
            showingMessage = false; // Stop showing the message
            displayMenu(lcd);       // Return to the menu display
        }
        return;
    }

    // Check button Up
    if (!digitalRead(buttons[0]) && !buttonUpPressed)
    {
        buttonUpPressed = true;
        isMenuActive = true; // Menu is active when navigating
        currentMenuIndex = (currentMenuIndex - 1 + currentMenu->itemCount) % currentMenu->itemCount;
        displayMenu(lcd);
        activityDetected = true;
    }
    if (digitalRead(buttons[0]))
        buttonUpPressed = false;

    // Check button Down
    if (!digitalRead(buttons[1]) && !buttonDownPressed)
    {
        buttonDownPressed = true;
        isMenuActive = true; // Menu is active when navigating
        currentMenuIndex = (currentMenuIndex + 1) % currentMenu->itemCount;
        displayMenu(lcd);
        activityDetected = true;
    }
    if (digitalRead(buttons[1]))
        buttonDownPressed = false;

    // Check button Select
    if (!digitalRead(buttons[2]) && !buttonSelPressed)
    {
        buttonSelPressed = true;
        isMenuActive = true; // Menu is active when a selection is made
        Menu *selectedMenu = currentMenu->subMenus[currentMenuIndex];
        if (selectedMenu != nullptr)
        {
            currentMenu = selectedMenu; // Navigate to the submenu
            currentMenuIndex = 0;       // Reset the index for the submenu
            displayMenu(lcd);
        }
        else if (currentMenu == &manualFeedingMenu && currentMenuIndex == 0)
        {                                             // "Feed Now"
            inManualFeedingMode = true;               // Toggle manual feeding mode          // Indicate that we're showing a message
            lastUpdateTime = xTaskGetTickCount();     // Record the current time
            xTaskNotifyGive(manualFeedingTaskHandle); // Notify the manual feeding task to start feeding
            showingMessage = true;                    // Indicate that we're showing a message
        }
        else if (currentMenu == &settingsMenu && currentMenuIndex == 0)
        {                                                  // "Toggle Temp/Hum"
            if (xSemaphoreTake(stateMutex, portMAX_DELAY)) // Take the mutex before accessing shared state
            {
                tempHumidEnabled = !tempHumidEnabled; // Toggle the temperature and humidity feature
                xSemaphoreGive(stateMutex);           // Release the mutex after updating the state
            }
            if (xSemaphoreTake(lcdMutex, portMAX_DELAY)) // Take the mutex before accessing the LCD
            {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Temp/Humid: ");
                lcd.print(tempHumidEnabled ? "Enabled" : "Disabled");
                xSemaphoreGive(lcdMutex);
            }
            showingMessage = true;                // Indicate that we're showing a message
            lastUpdateTime = xTaskGetTickCount(); // Record the current time
        }
        else if (currentMenu == &settingsMenu && currentMenuIndex == 1)
        {                                                  // "Toggle RFID"
            if (xSemaphoreTake(stateMutex, portMAX_DELAY)) // Take the mutex before accessing shared state
            {
                rfidEnabled = !rfidEnabled; // Toggle the RFID feature
                xSemaphoreGive(stateMutex); // Release the mutex after updating the state
            }

            if (xSemaphoreTake(lcdMutex, portMAX_DELAY)) // Take the mutex before accessing the LCD
            {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("RFID: ");
                lcd.print(rfidEnabled ? "Enabled" : "Disabled");
                xSemaphoreGive(lcdMutex);
            }

            showingMessage = true;                // Indicate that we're showing a message
            lastUpdateTime = xTaskGetTickCount(); // Record the current time
        }
        else if (currentMenu == &settingsMenu && currentMenuIndex == 2)
        {                                                  // "Toggle Manual Feeding"
            if (xSemaphoreTake(stateMutex, portMAX_DELAY)) // Take the mutex before accessing shared state
            {
                manualFeedingEnabled = !manualFeedingEnabled; // Toggle the manual feeding feature
                xSemaphoreGive(stateMutex);                   // Release the mutex after updating the state
            }

            if (xSemaphoreTake(lcdMutex, portMAX_DELAY)) // Take the mutex before accessing the LCD
            {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Manual Feeding: ");
                lcd.print(manualFeedingEnabled ? "Enabled" : "Disabled");
                xSemaphoreGive(lcdMutex);
            }

            showingMessage = true;
            lastUpdateTime = xTaskGetTickCount(); // Record the current time
        }
        else
        {
            if (xSemaphoreTake(lcdMutex, portMAX_DELAY))
            {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("No options for:");
                lcd.setCursor(0, 1);
                lcd.print(currentMenu->items[currentMenuIndex]);
                xSemaphoreGive(lcdMutex);
            }
            showingMessage = true;                // Indicate that we're showing a message
            lastUpdateTime = xTaskGetTickCount(); // Record the current time
        }
        activityDetected = true;
    }
    if (digitalRead(buttons[2]))
        buttonSelPressed = false;

    // Check back button (button[3])
    if (currentMenu->parent != nullptr && !digitalRead(buttons[3]) && !buttonBackPressed)
    {
        buttonBackPressed = true;
        currentMenu = currentMenu->parent; // Go back to the parent menu
        currentMenuIndex = 0;              // Reset the index to the start of the parent menu
        displayMenu(lcd);
        activityDetected = true;
    }
    if (digitalRead(buttons[3]))
        buttonBackPressed = false;

    // Update last activity time if any activity is detected
    if (activityDetected)
    {
        lastActivityTime = xTaskGetTickCount();
        isMenuActive = true; // Mark the menu as active
    }

    // Check for inactivity timeout (10 seconds)
    if (xTaskGetTickCount() - lastActivityTime >= pdMS_TO_TICKS(INACTIVITY_TIMEOUT))
    {
        isMenuActive = false; // Deactivate the menu
        bool tempEnabled = false;
        if (xSemaphoreTake(stateMutex, portMAX_DELAY)) // Take the mutex before accessing shared state
        {
            tempEnabled = tempHumidEnabled; // Store the current state of temperature and humidity feature
            xSemaphoreGive(stateMutex);     // Release the mutex after accessing the state
        }

        if (tempEnabled)
        {
            xTaskNotifyGive(tempHumidTaskHandle); // Notify the temperature and humidity task to update the display
        }
    }
}
