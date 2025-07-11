/* This program will have to be its own task when moved to freeRTOS */

#include "Feeder_Features.h"
#include "Feeder_Menu.h"

bool readRFID(MFRC522 &rfid, String &uidStr)
{

    if (!rfidEnabled)
        return false;

    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
        return false;

    uidStr = "";
    for (byte i = 0; i < rfid.uid.size; i++)
    {
        if (rfid.uid.uidByte[i] < 0x10)
            uidStr += "0";
        uidStr += String(rfid.uid.uidByte[i], HEX);
        if (i < rfid.uid.size - 1)
            uidStr += ":";
    }

    uidStr.toUpperCase();

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    Serial.print("Scanned UID: ");
    Serial.println(uidStr);

    return true;
}
// TODO!
/*bool writeRFID(MFRC522 &rfid, String &uidStr)
{
    
}*/
