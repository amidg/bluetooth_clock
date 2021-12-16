#include <WiFi.h>
#include "time.h"
#include <PubSubClient.h>

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -8*3600; //vancouver time -8, toronto time -5
const int   daylightOffset_sec = 3600;


const int numberOfHotspots = 3;

const char *ssid[numberOfHotspots] = { "TorontoDungeon", 
                                       "TakeItBoy",
                                       "yuyu" 
                                     };

const char *password[numberOfHotspots] = { "Thankyousir228!", 
                                           "Thankyousir228!",
                                           "yuyu" 
                                         };

const char *timeZone[numberOfHotspots] = { "Canada/Eastern",
                                            "Canada/Pacific"
                                            "Canada/Pacific"
                                          };