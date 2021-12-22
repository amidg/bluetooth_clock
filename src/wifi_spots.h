#include <WiFi.h>
#include "time.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -8*3600; //vancouver time -8, toronto time -5
const int   daylightOffset_sec = 3600;

//time when I started writing this firmware: 1:04AM
int hour = 1;
int minute = 4;

#define AIO_SERVER     "io.adafruit.com"
#define AIO_SERVERPORT  1883  
#define AIO_USERNAME  "amidg4x4"
#define AIO_KEY       "aio_FFXM51hRR2Urc5Np917rUTsX600T"

WiFiClient wifiClient;
Adafruit_MQTT_Client mqttClient(&wifiClient, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Subscribe message = Adafruit_MQTT_Subscribe(&mqttClient, AIO_USERNAME "/feeds/messagetoyuyu");

int mqttTimeout = 0; //5 seconds

const int byteSizeForMessage = 20;
const int byteSizeForScreen = 1;
const int byteSizeForBrightness = 1;
const int byteSizeForHour = 1;
const int byteSizeForMinute = 1;
const int byteSizeForColors = 3;

char *messageToClock = "msg";

const int numberOfHotspots = 3;

const char *ssid[numberOfHotspots] = { "TorontoDungeon", 
                                       "TakeItBoy",
                                       "yuyu" 
                                     };

const char *password[numberOfHotspots] = { "Thankyousir228!", 
                                           "Thankyousir228!",
                                           "yuyu" 
                                         };