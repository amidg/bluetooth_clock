/*

  BLUETOOTH CLOCK WITH WI-FI / BLUETOOTH SPEAKER AND NEOPIXEL RGB MATRIX

  Developed by Dmitrii Gusev.
  github.com/amidg

  This firmware contains products developed by Adafruit, please, buy their work! 

  This firmware is designed to run on custom PCB by Dmitrii Gusev. Performance on other platforms is not guaranteed.

  FOR SUCCESSFULL BUILD:
  execute in PlatformIO CLI (VIEW -> COMMAND PALLETE -> PLATFORM IO CLI)
  pio lib install "adafruit/Adafruit BusIO"

*/

// LIBRARY
#include <Arduino.h>

#include "Adafruit-GFX-Library\Adafruit_GFX.h"
#include "Adafruit_NeoMatrix\Adafruit_NeoMatrix.h"
#include "Adafruit_NeoPixel\Adafruit_NeoPixel.h"

#include "wifi_spots.h"

// ================== MULTICORE OPERATION ================== //
TaskHandle_t Core0task; //task to run on core #0
TaskHandle_t Core1task; //task to run on core #1

// char* CPUusage = "";
// char* RAMusage = "";

// ================== LED MATRIX ================== //
#define PIN 21 //led matrix pin
#define BRIGHTNESS 50
#define LAST_SCREEN 8

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32, 8, PIN,
  NEO_MATRIX_BOTTOM  + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);

const uint16_t colors[] = {
  matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255) };

int screenMode = 1; //1 default

#define rainbowTimeout 10 //timeout in ms
void drawRainbow(int delayBetweenFrames);
void printText(String text, uint16_t desiredColor);

// ================= BUTTONS AND SCREEN ================ //
#define CHANGE_MODE 27
#define PREV_TRACK  32
#define PLAY_PAUSE  33
#define NEXT_TRACK  34

void IRAM_ATTR modeISR();
void IRAM_ATTR prevISR();
void IRAM_ATTR playISR();
void IRAM_ATTR nextISR();

void timeScreen();
void messageScreen();
void musicScreen();
void weatherScreen();
void loveYouScreen();

// ================== STEREO AUDIO AMPLIFIER ================== //


// ================== SD CARD SETUP ================== //



// ================== MULTI CORE ====================== //
void Core0loopTask( void * parameter );
void Core1loopTask( void * parameter );


// ================= DAY AND TIME ====================== //
String currentTime = "01:04 AM"; //time I started developing this firwmare
unsigned long timeTakenAt = 0;
unsigned long timeout = 60000; //1 minute
void connectToWiFi();
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

  xTaskCreatePinnedToCore(
      Core1loopTask, // Function to implement the task
      "Core1",       // Name of the task
      10000,         // Stack size in words
      NULL,          // Task input parameter
      1,             // Priority of the task
      &Core1task,    // Task handle
      1);            // Core where the task should run

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

  // bluetooth setup

}

void Core0loopTask( void * parameter ) {
  //core 0 task is responsible for the screen and buttons

  // matrix setup --> do in Core #0
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(BRIGHTNESS);


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
        // matrix
        break;
      case LAST_SCREEN:
        // cpu usage screen

        //print ram
        // sprintf(RAMusage, "RAM: %lu %", (100 - 100*(ESP.getFreeHeap()/520000) ));
        // printText( String(RAMusage), colors[0] );
        // Serial.println(RAMusage);
        break;
    }

    // =========== infinite loop for core #0 =========== //
  }
}

void Core1loopTask( void * parameter ) {
  //core 1 task is responsible for the communication
  // wi-fi setup
  //while (WiFi.status() != WL_CONNECTED) { connectToWiFi(); }; //don't use, causes page fault due to multiple wi-fi interrupts
  
  while(true) {
    // =========== infinite loop for core #1 =========== //
    switch (screenMode) {
      case 1:
        // time screen
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
        // matrix screen
        break;
      case 6:
        // love you screen
        break;
      case 7:
        // matrix
        break;
      case LAST_SCREEN:
        // cpu usage screen

        //print ram
        // sprintf(RAMusage, "RAM: %lu %", (100 - 100*(ESP.getFreeHeap()/520000) ));
        // printText( String(RAMusage), colors[0] );
        // Serial.println(RAMusage);
        break;
    }
    
    // =========== infinite loop for core #1 =========== //
  }
}

////// SUPPORTING FUNCTIONALITY /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//screen functions
void timeScreen() {
  // time screen
  if (millis() - timeTakenAt >= timeout) {

  while (WiFi.status() != WL_CONNECTED) { connectToWiFi(); };

  setupLocalTime();
  timeTakenAt = millis();
  Serial.println("time taken");
  } else {
    WiFi.disconnect();
    Serial.println(millis() - timeTakenAt);
  }
        
  printText(currentTime, colors[0]);
}

void messageScreen() {

}

void musicScreen() {

}

void weatherScreen() {

} 

void loveYouScreen() {

}

// MATRIX LED
void drawRainbow(int delayBetweenFrames) {
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
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
  screenMode++;

  if (screenMode > LAST_SCREEN)
    screenMode = 1;
}

void IRAM_ATTR prevISR() {
  
}

void IRAM_ATTR playISR() {
  
}

void IRAM_ATTR nextISR() {
  
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

void setupLocalTime(){
  struct tm timeinfo;

  while (!getLocalTime(&timeinfo)) {
    //waint until time is obtained
  } 

  if (timeinfo.tm_hour < 10)
    currentTime = currentTime + "0" + String(timeinfo.tm_hour) + ":";
  else
    currentTime = currentTime + String(timeinfo.tm_hour) + ":";

  if (timeinfo.tm_min < 10)
    currentTime = currentTime + "0" + String(timeinfo.tm_min);
  else
    currentTime = currentTime + String(timeinfo.tm_min);
}

//////////////////////////////////////////////
void loop() { 
  //do nothing here, exists just to pass the compiler
}
  