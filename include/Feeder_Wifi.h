#ifndef FEEDER_WIFI_H
#define FEEDER_WIFI_H

#include <Arduino.h>
#include <Feeder_Files.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Feeder_Features.h>



extern WiFiManager wm;

void wifiTask(void *paramater);

#endif