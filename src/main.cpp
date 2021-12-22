/*

  BLUETOOTH CLOCK WITH WI-FI / BLUETOOTH SPEAKER AND NEOPIXEL RGB MATRIX

  Developed by Dmitrii Gusev.
  github.com/amidg

  This firmware contains products developed by Adafruit, please, buy their work! 

  This firmware is designed to run on custom PCB by Dmitrii Gusev. Performance on other platforms is not guaranteed.

  FOR SUCCESSFULL BUILD:
  execute in PlatformIO CLI (VIEW -> COMMAND PALLETE -> PLATFORM IO CLI)
  pio lib install "adafruit/Adafruit BusIO"
  pio lib install "adafruit/Adafruit MQTT Library"
  pio lib install "earlephilhower/ESP8266Audio"
  platformio lib search "header:FS.h"

*/

// LIBRARY
#include <Arduino.h>

#include "wifi_spots.h"
#include "images.h"
#include <EEPROM.h>

#include "music.h"

// ================== MULTICORE OPERATION ================== //
TaskHandle_t Core0task; //task to run on core #0
//TaskHandle_t Core1task; //task to run on core #1 --> replaced by loop()

#define EEPROM_SIZE (byteSizeForMessage + byteSizeForScreen + byteSizeForBrightness + byteSizeForHour + byteSizeForMinute + byteSizeForColors) // define the number of bytes you want to access
/*
  byte 1 -> fav color
  byte 2 -> last screen brightness
  byte 3..22 -> 20 chars for message
  byte 23 -> byte for hour
  byte 24 -> byte for minutes
*/

void readLastDataFromEEPROM();
int firstScan = true; // property similar to PLC because this code is also some sort of a fake RTOS
void setIntegerToEEPROM(byte byte, int data);
void setCharToEEPROM(byte byte, char data);
void setDefaultEEPROM(bool isResetRequired);
#define HARD_RESET false //DO NOT SET TO TRUE UNLESS YOUR BLUETOOTH CLOCK STOPPED WORKING, THIS CLEARS YOUR SETTINGS AND STOPPS EEPROM FUNCTIONALITY

// ================== LED MATRIX ================== //
int favoriteColorNum = 0;
uint16_t favoriteColor = colors[favoriteColorNum];

int BRIGHTNESS_DAY = 50;
#define LAST_SCREEN 9
bool nightModeEnabled = false;

int screenMode = 1; //1 default
int prevScreen = screenMode;
bool setupBrightness = false;
bool setupColor = false;
bool playMusic = true;
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

unsigned long timeBetweenModeClick = 0;
unsigned long timeBetweenPrevClick = 0;
unsigned long timeBetweenPlayClick = 0;
unsigned long timeBetweenNextClick = 0;
#define buttonTimeout 500 //500 ms

void timeScreen();
void messageScreen();
void musicScreen();
void weatherScreen();
void loveYouScreen();
void synthwaveScreen();
void changeFavoriteColorScreen();
void brightnessScreen();
void drawVerticalBar(int x);
void clearMatrix();
void connectToMQTT();

// ================== STEREO AUDIO SETUP ================== //


// ================== SD CARD SETUP ================== //



// ================== MULTI CORE ====================== //
void Core0loopTask( void * parameter );
void Core1loopTask( void * parameter );


// ================= DAY AND TIME ====================== //
String currentTime = " ";
uint timeTakenAt = 0;
//unsigned long timeTakenAt = 0;
uint currentCPUtime = 0;
const int timeout = 60;
//unsigned long timeout = 60000; //1 minute
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

  timeBetweenModeClick = millis();
  timeBetweenPrevClick = millis();
  timeBetweenPlayClick = millis();
  timeBetweenNextClick = millis();

  // setup EEPROM
  EEPROM.begin(EEPROM_SIZE);
  setDefaultEEPROM(HARD_RESET);
  readLastDataFromEEPROM();
  if (screenMode < 1 || screenMode > LAST_SCREEN) { screenMode = 1; };
  if (BRIGHTNESS_DAY < 10 || BRIGHTNESS_DAY > 100) { BRIGHTNESS_DAY = 50; };

  // button setup
  pinMode(CHANGE_MODE, INPUT);  attachInterrupt(CHANGE_MODE, modeISR, RISING);
  pinMode(PREV_TRACK,  INPUT);  attachInterrupt(PREV_TRACK,  prevISR, RISING);
  pinMode(PLAY_PAUSE,  INPUT);  attachInterrupt(PLAY_PAUSE,  playISR, RISING);
  pinMode(NEXT_TRACK,  INPUT);  attachInterrupt(NEXT_TRACK,  nextISR, RISING);

  // sd card setup


  //bluetooth setup
  a2dp_sink.set_bits_per_sample(32); 
  a2dp_sink.start("Love you Yuyu!");

  // time setup
  timeTakenAt = millis()/1000; //initial setup
  currentCPUtime = millis()/1000;
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  setCurrentTime(hour, minute); //initial time setup
  printText(currentTime, favoriteColor);

  // mqtt setup
  mqttClient.subscribe(&message);

  // matrix setup --> do in Core #0
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(BRIGHTNESS_DAY);
}

void Core0loopTask( void * parameter ) {
  //core 0 task is responsible for the screen and buttons
  while(true) {
    // =========== infinite loop for core #0 =========== //
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
        break;
      case 8:
        // color screen
        changeFavoriteColorScreen();
        break;
      case LAST_SCREEN:
        // brightness screen
        brightnessScreen();
        break;
    }

    delay(1); //needed to allow task watchdog timer to reset
    // =========== infinite loop for core #0 =========== //
  }
}

void loop() { //COMMUNICATION INTERFACE CONTROL ONLY
  while(true) {
    // =========== infinite loop for core #1 =========== //
    //Serial.println("loop() runs on: " + String(xPortGetCoreID()));

    //switch case
    switch (screenMode) {
      case 1:
        //time screen
        //Serial.println(screenMode);
        break;
      case 2:
        // rainbow screen
        WiFi.disconnect();
        break;
      case 3:
        // message screen
        
        break;
      case 4:
        // weather screen
        
        break;
      case 5:
        // music screen = bluetooth speaker
        WiFi.disconnect();
        break;
      case 6:
        // love you screen
        
        break;
      case 7:
        // synthwave screen
        
        break;
      case 8:
        // color screen
        WiFi.disconnect();        
        break;
      case LAST_SCREEN:
        WiFi.disconnect();
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
void setDefaultEEPROM(bool isResetRequired) {
  if (isResetRequired) {
    setIntegerToEEPROM(0, 0);
    setIntegerToEEPROM(1, 50);

    for (int byteNum = 0; byteNum < byteSizeForMessage; byteNum++) {
      setCharToEEPROM(byteNum + 2, ' ');
    }

    setIntegerToEEPROM(22, 1);
    setIntegerToEEPROM(23, 4);
  }
}

void readLastDataFromEEPROM() {
  /*
    byte 0 -> fav color
    byte 1 -> last screen brightness
    byte 2..21 -> 20 chars for message
    byte 22 -> byte for hour
    byte 23 -> byte for minutes
  */

  favoriteColorNum = EEPROM.read(0); // read last favorite color
  BRIGHTNESS_DAY = EEPROM.read(1);

  // for (int byteNum = 0; byteNum < (unsigned)strlen(messageToClock) && byteNum < byteSizeForMessage; byteNum++) {
  //   messageToClock[byteNum] = EEPROM.read(byteNum + 2);
  // }

  hour = EEPROM.read(22);
  minute = EEPROM.read(23);

  randomGreen = EEPROM.read(24); // to do: rework this
  randomRed = EEPROM.read(25);
  randomBlue = EEPROM.read(26);

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
  currentCPUtime = millis()/1000;
  if (currentCPUtime - timeTakenAt >= timeout) {

  while (WiFi.status() != WL_CONNECTED && screenMode == 1) { connectToWiFi(); };

  setupLocalTime();
  firstScan = false; // this is where we start caring about the EEPROM to avoid pagefault
  timeTakenAt = millis()/1000;
  Serial.println("time taken");
  printText(currentTime, favoriteColor);
  } else {
    WiFi.disconnect();
    //Serial.println("core 0 task runs on: " + String(xPortGetCoreID()));
    Serial.println(currentCPUtime - timeTakenAt);
    //Serial.println(screenMode);
  }
}

void messageScreen() {
  //once connected start manipulation
  printText(messageToClock, favoriteColor);
  while (WiFi.status() != WL_CONNECTED && screenMode == 3) {
    // loop here until you connect to wifi
    connectToWiFi();
  }

  connectToMQTT();

  while (screenMode == 3 && mqttClient.connected()) {
    mqttClient.readSubscription(1000);
    messageToClock = (char*)message.lastread;

    Serial.println(messageToClock);
    printText(String(messageToClock), favoriteColor);
  }
}

void musicScreen() {
  printText("music", favoriteColor);

  // aac->begin(in, out);

  // if (aac->isRunning()) {
  //   aac->loop();
  // } else {
  //   aac -> stop();
  //   Serial.printf("AAC done\n");
  //   delay(1000);
  // }
}

void weatherScreen() {
  printText("rain", favoriteColor);
} 

void loveYouScreen() {
  printText("love", favoriteColor);
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

void changeFavoriteColorScreen() {
  bool randomColorSet = false;

  clearMatrix();
  while(!setupColor) {
    if (favoriteColorNum == maxNumOfColors - 1 && !randomColorSet) {
      randomGreen = random(10, 255); setIntegerToEEPROM(24, randomGreen);
      randomRed = random(10, 255);   setIntegerToEEPROM(25, randomRed);
      randomBlue = random(10, 255);  setIntegerToEEPROM(26, randomBlue);
      randomColorSet = !randomColorSet;
    } else if (favoriteColorNum != maxNumOfColors - 1) {
      randomColorSet = false;
    }

    favoriteColor = colors[favoriteColorNum];

    Serial.println(favoriteColorNum);
    matrix.fill(favoriteColor);
    matrix.show();
  }

  setIntegerToEEPROM(0, favoriteColorNum);

  screenMode = LAST_SCREEN;
  clearMatrix();
}

void brightnessScreen() {
  int brightBar = 0;

  while(!setupBrightness) {
    clearMatrix();
    brightBar = 3*BRIGHTNESS_DAY/10;
    drawVerticalBar(brightBar);
    matrix.setBrightness(BRIGHTNESS_DAY);
  }

  setIntegerToEEPROM(1, BRIGHTNESS_DAY);

  screenMode = 1;
  clearMatrix();
  printText(currentTime, favoriteColor);
}

// MATRIX LED
void drawRainbow(int delayBetweenFrames) {
  for(long firstPixelHue = 0; firstPixelHue < 5*65536 && screenMode == 2; firstPixelHue += 256) {
    matrix.rainbow(firstPixelHue);
    matrix.show(); // Update strip with new contents
    delay(delayBetweenFrames);  // Pause for a moment
  }
}

void printText(String text, uint16_t desiredColor) {
  matrix.setTextColor(desiredColor);
  matrix.fillScreen(0);

  if (text.length() < 6) {
    matrix.setCursor(2, 0);
    matrix.print(text);
  } else {
    for (int x = 2; x >= ( (-1)*matrix.width() - 20 ); x-- ) {
      matrix.fillScreen(0);
      matrix.setCursor(x, 0);
      matrix.print(text);
      matrix.show();
      delay(100);
    }
  }
  
  matrix.show();
  delay(100);
}

// INTERRUPT SERVICE ROUTINES
void IRAM_ATTR modeISR() {
  prevScreen = screenMode;

  if (millis() - timeBetweenModeClick > buttonTimeout) {
    screenMode++;
    if (screenMode - prevScreen > 1) { screenMode = prevScreen + 1; };
    timeBetweenModeClick = millis();
  }

  if (screenMode > LAST_SCREEN)
    screenMode = 1;
}

void IRAM_ATTR prevISR() {
  if (millis() - timeBetweenPrevClick > buttonTimeout) {
    timeBetweenPrevClick = millis();
  
    if (screenMode == LAST_SCREEN && BRIGHTNESS_DAY > 10) {
      BRIGHTNESS_DAY -= 10;
    } else if (screenMode == 8 && favoriteColorNum >= 0) {
      favoriteColorNum--;
    } else {
      musicTrackNum--;
      a2dp_sink.previous();

      if (musicTrackNum <= 0) 
        musicTrackNum = 0;
    }  
  }
}

void IRAM_ATTR playISR() {
  if (millis() - timeBetweenPlayClick > buttonTimeout) {

    if (screenMode == LAST_SCREEN && BRIGHTNESS_DAY > 10) {
      setupBrightness = !setupBrightness; //should execute as true
    } else if (screenMode == 8) {
      setupColor = !setupColor;
    } else {
      playMusic = !playMusic;

      if (playMusic)
        a2dp_sink.play();
      else 
        a2dp_sink.pause();
    }  
    
    timeBetweenPlayClick = millis();
  }
}

void IRAM_ATTR nextISR() {
  if (millis() - timeBetweenNextClick > buttonTimeout) {
    timeBetweenNextClick = millis();

    if (screenMode == LAST_SCREEN && BRIGHTNESS_DAY < 100) {
      BRIGHTNESS_DAY += 10;
    } else if (screenMode == 8 && favoriteColorNum < maxNumOfColors) {
      favoriteColorNum++;
    } else {
      musicTrackNum++;
      a2dp_sink.next();

      if (musicTrackNum > 10) 
        musicTrackNum = 10;
    }
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
    Serial.println('.');
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
      matrix.writePixel(currentBar, y, favoriteColor);
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

void connectToMQTT() {
  int8_t ret;
  uint8_t retries = 3;

  // Stop if already connected.
  if (mqttClient.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ( (ret = mqttClient.connect()) != 0 && screenMode == 3 && retries > 0 ) { // connect will return 0 for connected
    if (millis()/1000 - mqttTimeout > 5) {
      Serial.println(mqttClient.connectErrorString(ret));
      Serial.println("Retrying MQTT connection in 5 seconds...");
      mqttClient.disconnect();
      retries--;
      mqttTimeout = millis()/1000;
    }
  }

  Serial.println("MQTT Connected!");
}