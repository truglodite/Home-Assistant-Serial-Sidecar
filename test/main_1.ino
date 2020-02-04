// Hassio Analog Sidecar
// main.ino
// by: Truglodite
// Code for a nano to handle analog signals for hassio (add analogs to RPi).
// New version sends raw ADC values so calibration and calcs are handled on pi.
#include <ArduinoJson.h>

// User Configuration //////////////////////////////////////////////////////////
// All times are in millis, and thresholds are ints 1-511
// Set thresholds high enough to stop undesired spamming from signal noise,
// and low enough to reliably capture all desired changes in state.
// Mains is more or less on/off, so a large value is fine. Smaller values
// will be needed for the doorbell, depending on chime, wiring, & transformer.

#define battPin           A0     // backup battery voltage divider (10:5)
#define battName          "battery"
#define battUploadPeriod  600000 // time between updates

#define mainsPin          A1     // mains voltage sense (5V wallwart or other)
#define mainsName         "mains"
#define mainsTimeout      2000   // min. time between updates
#define mainsThreshold    400    // min ADC change required before updating

#define bellPin           A2     // doorbell 5A ac hall sensor
#define bellName          "bell"
#define bellTimeout       2000   // min. time between updates
#define bellThreshold     25     // min ADC change required before updating

#define brickPin          A3     // UPS voltage sense (5v wallwart or other)
#define brickName         "brick"
#define brickTimeout      2000   // min. time between updates
#define brickThreshold    400    // min ADC change required before updating

#define baudrate          115200
// <57> is technically enough so <200> seems safe. :P
// Use arduinojson.org/v6/assistant to compute the capacity if needed.
StaticJsonDocument<200> jsonBuffer;

// Globals /////////////////////////////////////////////////////////////////////
unsigned long battChangeTime = 0;
int battState = 0;
int battADC = 0;
unsigned long mainsChangeTime = 0;
int mainsState = 0;
int mainsADC = 0;
unsigned long bellChangeTime = 0;
int bellState = 0;
int bellADC = 0;
unsigned long brickChangeTime = 0;
int brickState = 0;
int brickADC = 0;
bool changedState = 0;

// Setup ///////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(baudrate);
  while (!Serial) continue;

  // initialize and send data after boot up
  #ifndef calibrationMode
  battADC = analogRead(battPin);
  jsonBuffer[battName] = battADC;
  battState = battADC;
  mainsADC = analogRead(mainsPin);
  jsonBuffer[mainsName] = mainsADC;
  mainsState = mainsADC;
  bellADC = analogRead(bellPin);
  jsonBuffer[bellName] = bellADC;
  bellState = bellADC;
  brickADC = analogRead(brickPin);
  jsonBuffer[brickName] = brickADC;
  brickState = brickADC;
  brickChangeTime = bellChangeTime = mainsChangeTime = battChangeTime = millis();
  serializeJson(jsonBuffer, Serial);
  Serial.println();
  #endif
}

// Loop ////////////////////////////////////////////////////////////////////////
void loop() {
  // time to update battery voltage
  if(millis() - battChangeTime > battUploadPeriod)  {
    jsonBuffer[battName] = analogRead(battPin);
    battChangeTime = millis();
    changedState = 1;
  }

  mainsADC = analogRead(mainsPin);
  // mains has changed enough to report it...
  if(mainsADC >= mainsState + mainsThreshold || mainsADC <= mainsState - mainsThreshold)  {
    // only change if enough time has passed
    if(millis() - mainsChangeTime > mainsTimeout) {
      jsonBuffer[mainsName] = mainsADC;
      mainsState = mainsADC;
      mainsChangeTime = millis();
      changedState = 1;
    }
  }

  bellADC = analogRead(bellPin);
  // bell has changed enough to report it...
  if(bellADC >= bellState + bellThreshold || bellADC <= bellState - bellThreshold)  {
    // only change if enough time has passed
    if(millis() - bellChangeTime > bellTimeout) {
      jsonBuffer[bellName] = bellADC;
      bellState = bellADC;
      bellChangeTime = millis();
      changedState = 1;
    }
  }

  brickADC = analogRead(brickPin);
  // UPS has changed enough to report it...
  if(brickADC >= brickState + brickThreshold || brickADC <= brickState - brickThreshold)  {
    // only change if enough time has passed
    if(millis() - brickChangeTime > brickTimeout) {
      jsonBuffer[brickName] = brickADC;
      brickState = brickADC;
      brickChangeTime = millis();
      changedState = 1;
    }
  }

  // we have new data to send...
  if(changedState) {
    serializeJson(jsonBuffer, Serial);
    Serial.println();
    changedState = 0;
  }
}
