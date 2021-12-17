#include <WiFi.h>
#include "time.h"
#include <PubSubClient.h>

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -8*3600; //vancouver time -8, toronto time -5
const int   daylightOffset_sec = 3600;

//time when I started writing this firmware: 1:04AM
int hour = 1;
int minute = 4;

const char* mqttServer = "96.49.218.25"; //temporary server, replace later
int mqttPort = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient); 
String clientId = "BlueClock_1";
const int byteSizeForMessage = 20;
const int byteSizeForScreen = 1;
const int byteSizeForBrightness = 1;
const int byteSizeForHour = 1;
const int byteSizeForMinute = 1;

char messageToClock[byteSizeForMessage] = "msg";

const int numberOfHotspots = 3;

const char *ssid[numberOfHotspots] = { "TorontoDungeon", 
                                       "TakeItBoy",
                                       "yuyu" 
                                     };

const char *password[numberOfHotspots] = { "Thankyousir228!", 
                                           "Thankyousir228!",
                                           "yuyu" 
                                         };