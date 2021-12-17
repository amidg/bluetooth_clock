/*

  BLUETOOTH CLOCK WITH WI-FI / BLUETOOTH SPEAKER AND NEOPIXEL RGB MATRIX

  Developed by Dmitrii Gusev.
  github.com/amidg

  This firmware contains products developed by Adafruit, please, buy their work! 

  This firmware is designed to run on custom PCB by Dmitrii Gusev. Performance on other platforms is not guaranteed.

  FOR SUCCESSFULL BUILD:
  execute in PlatformIO CLI (VIEW -> COMMAND PALLETE -> PLATFORM IO CLI)
  pio lib install "adafruit/Adafruit BusIO"
  pio lib install "knolleary/PubSubClient"

*/

// LIBRARY
#include <Arduino.h>

#include "Adafruit-GFX-Library\Adafruit_GFX.h"
#include "Adafruit_NeoMatrix\Adafruit_NeoMatrix.h"
#include "Adafruit_NeoPixel\Adafruit_NeoPixel.h"

#include "wifi_spots.h"
#include "images.h"
#include <EEPROM.h>

// ================== MULTICORE OPERATION ================== //
TaskHandle_t Core0task; //task to run on core #0
//TaskHandle_t Core1task; //task to run on core #1 --> replaced by loop()

#define EEPROM_SIZE (byteSizeForMessage + byteSizeForScreen + byteSizeForBrightness + byteSizeForHour + byteSizeForMinute) // define the number of bytes you want to access
/*
  byte 1 -> last screen used
  byte 2 -> last screen brightness
  byte 3..22 -> 20 chars for message
  byte 23 -> byte for hour
  byte 24 -> byte for minutes
*/

void readLastDataFromEEPROM();
int firstScan = true; // property similar to PLC because this code is also some sort of a fake RTOS
void setIntegerToEEPROM(byte byte, int data);
void setCharToEEPROM(byte byte, char data);

// ================== LED MATRIX ================== //
#define PIN 21 //led matrix pin
int BRIGHTNESS_DAY = 50;
#define BRIGHTNESS_NIGHT 20
#define LAST_SCREEN 9
bool nightModeEnabled = false;

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32, 8, PIN,
  NEO_MATRIX_BOTTOM  + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);

const uint16_t colors[] = {
  matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255) }; //green, red, blue

int screenMode = 1; //1 default
int prevScreen = screenMode;
bool playMusic = false;
bool next = false;
bool prev = false;

#define rainbowTimeout 10 //timeout in ms
void drawRainbow(int delayBetweenFrames);
void printText(String text, uint16_t desiredColor);

// ================= BUTTONS AND SCREEN ================ //
#define CHANGE_MODE 27
#define PREV_TRACK  32
#define PLAY_PAUSE  33
#define NEXT_TRACK  34

int musicTrackNum = 0;

void IRAM_ATTR modeISR();
void IRAM_ATTR prevISR();
void IRAM_ATTR playISR();
void IRAM_ATTR nextISR();

void timeScreen();
void messageScreen();
void musicScreen();
void weatherScreen();
void loveYouScreen();
void synthwaveScreen();
void fireScreen();
void brightnessScreen();
void drawVerticalBar(int x);
void clearMatrix();
void setupMQTT();
void reconnect();

// ================== STEREO AUDIO SETUP ================== //


// ================== SD CARD SETUP ================== //



// ================== MULTI CORE ====================== //
void Core0loopTask( void * parameter );
void Core1loopTask( void * parameter );


// ================= DAY AND TIME ====================== //
String currentTime = " ";
unsigned long timeTakenAt = 0;
unsigned long timeout = 60000; //1 minute
void connectToWiFi();
void setCurrentTime(int hour, int minute);
void setupLocalTime();

void setup() {
  Serial.begin(9600);

  //multicore setup
  xTaskCreatePinnedToCore(
      Core0loopTask, // Function to implement the task
      "Core0",       // Name of the task
      10000,         // Stack size in words
      NULL,          // Task input parameter
      0,             // Priority of the task
      &Core0task,    // Task handle
      0);            // Core where the task should run

  // xTaskCreatePinnedToCore( //replaced by loop()
  //     Core1loopTask, // Function to implement the task
  //     "Core1",       // Name of the task
  //     10000,         // Stack size in words
  //     NULL,          // Task input parameter
  //     1,             // Priority of the task
  //     &Core1task,    // Task handle
  //     1);            // Core where the task should run

  // setup EEPROM
  EEPROM.begin(EEPROM_SIZE);
  readLastDataFromEEPROM();
  if (screenMode < 1 || screenMode > LAST_SCREEN) { screenMode = 1; };
  if (BRIGHTNESS_DAY < 10 || BRIGHTNESS_DAY > 100) { BRIGHTNESS_DAY = 50; };

  // button setup
  pinMode(CHANGE_MODE, INPUT);  attachInterrupt(CHANGE_MODE, modeISR, RISING);
  pinMode(PREV_TRACK,  INPUT);  attachInterrupt(PREV_TRACK,  prevISR, RISING);
  pinMode(PLAY_PAUSE,  INPUT);  attachInterrupt(PLAY_PAUSE,  playISR, RISING);
  pinMode(NEXT_TRACK,  INPUT);  attachInterrupt(NEXT_TRACK,  nextISR, RISING);

  // audio setup


  // sd card setup


  // time setup
  timeTakenAt = millis(); //initial setup
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  setCurrentTime(hour, minute); //initial time setup

  // bluetooth setup

  // mqtt setup
  

  // matrix setup --> do in Core #0
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(BRIGHTNESS_DAY);
}

void Core0loopTask( void * parameter ) {
  //core 0 task is responsible for the screen and buttons
  while(true) {
    setIntegerToEEPROM(0, screenMode);
    // =========== infinite loop for core #0 =========== //
    if (nightModeEnabled) 
      matrix.setBrightness(BRIGHTNESS_NIGHT);

    switch (screenMode) {
      case 1:
        //time screen
        timeScreen();
        break;
      case 2:
        // rainbow screen
        drawRainbow(rainbowTimeout);
        break;
      case 3:
        // message screen
        messageScreen();
        break;
      case 4:
        // weather screen
        weatherScreen();
        break;
      case 5:
        // music screen
        musicScreen();
        break;
      case 6:
        // love you screen
        loveYouScreen();
        break;
      case 7:
        // synthwave screen
        synthwaveScreen();
        //zerotwoScreen();
        break;
      case 8:
        // fire screen
        fireScreen();
        break;
      case LAST_SCREEN:
        // brightness screen
        brightnessScreen();
        break;
    }

    if (!firstScan) //this first scan property allows to utilize EEPROM without causing page fault
      readLastDataFromEEPROM();

    // =========== infinite loop for core #0 =========== //
  }
}

void loop() { //COMMUNICATION INTERFACE CONTROL ONLY
  while(true) {
    // =========== infinite loop for core #1 =========== //
    //Serial.println("loop() runs on: " + String(xPortGetCoreID()));
    switch (screenMode) {
      case 1:
        //time screen
        
        break;
      case 2:
        // rainbow screen
        WiFi.disconnect();
        break;
      case 3:
        // message screen
        while (WiFi.status() != WL_CONNECTED && screenMode == 3) { connectToWiFi(); };
        break;
      case 4:
        // weather screen
        
        break;
      case 5:
        // music screen
        
        break;
      case 6:
        // love you screen
        
        break;
      case 7:
        // synthwave screen
        
        break;
      case 8:
        // fire screen
        
        break;
      case LAST_SCREEN:
        
        break;
    }

    //debug only
    //Serial.println("Screen Mode: " + String(screenMode));
    //Serial.println("Track #: " + String(musicTrackNum));
    //Serial.println("Play? " + String(playMusic));

    // =========== infinite loop for core #1 =========== //
  }
}

////// SUPPORTING FUNCTIONALITY /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void readLastDataFromEEPROM() {
  /*
    byte 0 -> last screen used
    byte 1 -> last screen brightness
    byte 2..21 -> 20 chars for message
    byte 22 -> byte for hour
    byte 23 -> byte for minutes
  */

  screenMode = EEPROM.read(0);
  BRIGHTNESS_DAY = EEPROM.read(1);

  for (int byteNum = 0; byteNum < byteSizeForMessage; byteNum++) {
    messageToClock[byteNum] = EEPROM.read(byteNum + 2);
  }

  hour = EEPROM.read(22);
  minute = EEPROM.read(23);

}

void setIntegerToEEPROM(byte byte, int data) {
  EEPROM.write(byte, data);
  EEPROM.commit();
}

void setCharToEEPROM(byte byte, char data) {
  EEPROM.write(byte, data);
  EEPROM.commit();
}

//screen functions
void timeScreen() {
  // time screen
  printText(currentTime, colors[0]);

  if (millis() - timeTakenAt >= timeout) {

  while (WiFi.status() != WL_CONNECTED) { connectToWiFi(); };

  setupLocalTime();
  firstScan = false; // this is where we start caring about the EEPROM to avoid pagefault
  timeTakenAt = millis();
  Serial.println("time taken");
  } else {
    WiFi.disconnect();
    //Serial.println("core 0 task runs on: " + String(xPortGetCoreID()));
    Serial.println(millis() - timeTakenAt);
  }
}

void messageScreen() {
  //once connected start manipulation
  while (screenMode == 3 && WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected())
      reconnect(); //reconnect if not connected
    
    printText(String(messageToClock), colors[0]);
  }
}

void musicScreen() {
  printText("music", colors[0]);
}

void weatherScreen() {
  printText("rain", colors[0]);
} 

void loveYouScreen() {
  printText("love", colors[0]);
}

void synthwaveScreen() {
  int pixelNum = 0;

  for (int x = 0; x < 32 && pixelNum < 256; x++) {
    for (int y = 0; y < 8; y++) {
      matrix.writePixel(synthwave[pixelNum].x, synthwave[pixelNum].y, matrix.Color(synthwave[pixelNum].green, synthwave[pixelNum].red, synthwave[pixelNum].blue));
      //delay(100);
      matrix.show();
      pixelNum++;
    }
  }
}

void fireScreen() {
  // work in progress
  printText("fire", colors[0]);
}

void brightnessScreen() {
  int brightBar = 0;

  playMusic = false;
  while(!playMusic) {
    clearMatrix();
    brightBar = 3*BRIGHTNESS_DAY/10;
    drawVerticalBar(brightBar);
    matrix.setBrightness(BRIGHTNESS_DAY);
  }

  setIntegerToEEPROM(1, BRIGHTNESS_DAY);

  screenMode = 1;
  clearMatrix();
}

// MATRIX LED
void drawRainbow(int delayBetweenFrames) {
  prevScreen = screenMode;
  for(long firstPixelHue = 0; firstPixelHue < 5*65536 && prevScreen == screenMode; firstPixelHue += 256) {
    matrix.rainbow(firstPixelHue);
    matrix.show(); // Update strip with new contents
    delay(delayBetweenFrames);  // Pause for a moment
  }
}

void printText(String text, uint16_t desiredColor) {
  matrix.setTextColor(desiredColor);
  matrix.fillScreen(0);
  matrix.setCursor(2, 0);
  matrix.print(text);
  matrix.show();
  delay(100);
}

// INTERRUPT SERVICE ROUTINES
void IRAM_ATTR modeISR() {
  prevScreen = screenMode;

  screenMode++;

  if (screenMode > LAST_SCREEN)
    screenMode = 1;
}

void IRAM_ATTR prevISR() {
  if (screenMode == LAST_SCREEN && BRIGHTNESS_DAY > 10) {
    BRIGHTNESS_DAY -= 10;
  } else {
    musicTrackNum--;

    if (musicTrackNum <= 0) 
      musicTrackNum = 0;
  }  
}

void IRAM_ATTR playISR() {
  playMusic = !playMusic;

  //if (screenMode == LAST_SCREEN) { setIntegerToEEPROM(1, BRIGHTNESS_DAY); };
}

void IRAM_ATTR nextISR() {
  if (screenMode == LAST_SCREEN && BRIGHTNESS_DAY < 100) {
    BRIGHTNESS_DAY += 10;
  } else {
    musicTrackNum++;

    if (musicTrackNum > 10) 
      musicTrackNum = 10;
  }
}

void connectToWiFi() {
  //connect to one of the Wi-Fi
  unsigned int timeoutWiFi = 5000;
  unsigned long connectionTime = millis();

  for (int i = 0; i < numberOfHotspots && WiFi.status() != WL_CONNECTED; i++) {
    //try one of the wi-fi hotspots
      Serial.print("Connecting to ");
      Serial.println(ssid[i]);
      WiFi.begin(ssid[i], password[i]);

      while (WiFi.status() != WL_CONNECTED && (millis() - connectionTime < timeoutWiFi) ) {
        Serial.print(".");
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected.");
        break;
      }

      if (i == numberOfHotspots && WiFi.status() != WL_CONNECTED) { i = 0; };
  }
}

void setCurrentTime(int hour, int minute) {
  if (hour < 10)
    currentTime = "0" + String(hour) + ":";
  else
    currentTime = String(hour) + ":";

  if (minute < 10)
    currentTime = currentTime + "0" + String(minute);
  else
    currentTime = currentTime + String(minute);
}

void setupLocalTime(){
  struct tm timeinfo;

  while (!getLocalTime(&timeinfo)) {
    //waint until time is obtained
  } 

  hour = timeinfo.tm_hour;
  minute = timeinfo.tm_min;

  setCurrentTime(hour, minute);

  setIntegerToEEPROM(22, hour);
  setIntegerToEEPROM(23, minute);
}

void drawVerticalBar(int x) {
  for (int currentBar = 0; currentBar <= x; currentBar++) {
    for (int y = 0; y < 8; y++) {
      matrix.writePixel(currentBar, y, colors[0]);
    }
  }
  
  matrix.show();
}

void clearMatrix() {
  for(int i = 0; i < 256; i++) {
    matrix.setPixelColor(i, matrix.Color(0, 0, 0));
  }
  matrix.show();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message:");
  for(int i = 0; i < byteSizeForMessage; i++) {
    messageToClock[i] = ' ';
  }  

  for (int i = 0; i < length && length < byteSizeForMessage; i++) {
    Serial.print((char)payload[i]);
    messageToClock[i] += (char)payload[i];
    setCharToEEPROM(2 + i, (char)payload[i]);
  }
}

void reconnect() {
  Serial.println("Connecting to MQTT Broker...");
  while (!mqttClient.connected()) {
    Serial.println("Reconnecting to MQTT Broker..");

    mqttClient.connect(clientId.c_str());
  }

  if (mqttClient.connect(clientId.c_str())) {
    Serial.println("Connected!");
    mqttClient.subscribe("/msg/toYuyu"); // subscribe to topic
  }
}

void setupMQTT() {
  mqttClient.setServer(mqttServer, mqttPort);
  // set the callback function
  mqttClient.setCallback(callback);
}