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

// ================== MULTICORE OPERATION ================== //
TaskHandle_t Core0task; //task to run on core #0
TaskHandle_t Core1task; //task to run on core #1

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

int screenMode = 1;
String currentTime = "01:04 AM";

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

// ================== STEREO AUDIO AMPLIFIER ================== //


// ================== SD CARD SETUP ================== //



// ================== MULTI CORE ====================== //
void Core0loopTask( void * parameter );
void Core1loopTask( void * parameter );

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
      &Core0task,    // Task handle
      1);            // Core where the task should run

  // matrix setup
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(BRIGHTNESS);

  // button setup
  pinMode(CHANGE_MODE, INPUT);  attachInterrupt(CHANGE_MODE, modeISR, RISING);
  pinMode(PREV_TRACK,  INPUT);  attachInterrupt(PREV_TRACK,  prevISR, RISING);
  pinMode(PLAY_PAUSE,  INPUT);  attachInterrupt(PLAY_PAUSE,  playISR, RISING);
  pinMode(NEXT_TRACK,  INPUT);  attachInterrupt(NEXT_TRACK,  nextISR, RISING);

  // audio setup


  // sd card setup


  // wi-fi setup


  // bluetooth setup



}

void Core0loopTask( void * parameter ) {
  //core 0 task is responsible for the screen and buttons
  while(true) {
    // =========== infinite loop for core #0 =========== //

    switch (screenMode) {
      case 1:
        // rainbow screen
        drawRainbow(rainbowTimeout);
        break;
      case 2:
        // time screen
        printText(currentTime, colors[0]);
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
        break;
    }

    // =========== infinite loop for core #0 =========== //
  }
}

void Core1loopTask( void * parameter ) {
  //core 1 task is responsible for the communication
  while(true) {
    // =========== infinite loop for core #1 =========== //

    Serial.println(digitalRead(PREV_TRACK));

    
    // =========== infinite loop for core #1 =========== //
  }
}

////// SUPPORTING FUNCTIONALITY /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
  matrix.setCursor(1, 0);
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


//////////////////////////////////////////////
void loop() { 
  //do nothing here, exists just to pass the compiler
}
  