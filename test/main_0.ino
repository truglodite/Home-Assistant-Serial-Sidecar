// Hassio Analog Sidecar
// main.ino
// by: Truglodite
// Code for a nano to handle analog signals for hassio (add analogs to RPi).
#include <ArduinoJson.h>

// User Configuration //////////////////////////////////////////////////////////
// All times are in millis, and thresholds are in ints
//#define calibrationMode 2000   // Uncomment!!! for calibration mode

#define battPin           A0     // backup battery voltage divider
#define battName          "battery"
#define battUploadPeriod  600000 // time between battery voltage uploads
#define battDividerRatio  0.0146 // calibrate: DMMvolts / ADCout

#define mainsPin          A1     // mains voltage sense (5V wallwart or other)
#define mainsName         "mains"
#define mainsTimeout      2000   // min. time mains can send on/off messages
#define mainsThreshold    512    // calibrate: (ADCon + ADCoff) / 2
#define mainsOnMsg        "on"
#define mainsOffMsg       "off"

#define bellPin           A2     // doorbell 5A ac hall sensor
#define bellName          "bell"
#define bellTimeout       2000   // min. time doorbell can send on/off messages
#define bellThreshold     210    // calibrate: (ADCon + ADCoff) / 2
#define bellOnMsg         "on"
#define bellOffMsg        "off"

// <51> is technically enough for the defaults, and <200> seems safe. :P
// Use arduinojson.org/v6/assistant to compute the capacity if needed.
StaticJsonDocument<200> jsonBuffer;
#define baudrate         115200

// Globals /////////////////////////////////////////////////////////////////////
unsigned long lastBattUploadTime = 0;
float vBatt = 0.0;
char vBattStr[7] = {0};
unsigned long mainsChangeTime = 0;
bool mainsState = 0;
int mainsADC = 0;
unsigned long bellChangeTime = 0;
bool bellState = 0;
int bellADC = 0;
bool changedState = 0;

// Setup ///////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(baudrate);
  while (!Serial) continue;

  // initialize and send data after boot up
  #ifndef calibrationMode
  vBatt = battDividerRatio * analogRead(battPin);
  dtostrf(vBatt, 3, 1, vBattStr); // convert: >=4 wide, precision 1 (-99.9)
  // load updated values in our json message
  jsonBuffer[battName] = vBattStr;
  lastBattUploadTime = millis();
  if(analogRead(mainsPin) >= mainsThreshold) {
    jsonBuffer[mainsName] = mainsOnMsg;
    mainsState = 1;
  }
  else {
    jsonBuffer[mainsName] = mainsOffMsg;
    mainsState = 0;
  }
  mainsChangeTime = millis();
  if(analogRead(bellPin) >= bellThreshold) {
    jsonBuffer[bellName] = bellOnMsg;
    bellState = 1;
  }
  else {
    jsonBuffer[bellName] = bellOffMsg;
    bellState = 0;
  }
  bellChangeTime = millis();
  serializeJson(jsonBuffer, Serial);
  Serial.println();
  #endif
}

// Loop ////////////////////////////////////////////////////////////////////////
void loop() {
  // normal operating loop (non-calibration) ///////////////////////////////////
  #ifndef calibrationMode

  // time to update battery voltage
  if(millis() - lastBattUploadTime > battUploadPeriod)  {
    vBatt = battDividerRatio * analogRead(battPin);
    dtostrf(vBatt, 3, 1, vBattStr); // convert: >=4 wide, precision 1 (-99.9)
    // load updated values in our json message
    jsonBuffer[battName] = vBattStr;
    lastBattUploadTime = millis();
    changedState = 1;
  }

  mainsADC = analogRead(mainsPin);
  // mains are on, server state is off, and enough time has passed...
  if(mainsADC >= mainsThreshold && !mainsState && millis() - mainsChangeTime > mainsTimeout) {
    jsonBuffer[mainsName] = mainsOnMsg;
    mainsChangeTime = millis();
    mainsState = 1;
    changedState = 1;
  }
  // mains are off, server state is on, and enough time has passed...
  else if(mainsADC < mainsThreshold && mainsState && millis() - mainsChangeTime > mainsTimeout) {
    jsonBuffer[mainsName] = mainsOffMsg;
    mainsChangeTime = millis();
    mainsState = 0;
    changedState = 1;
  }

  bellADC = analogRead(bellPin);
  // doorbell on, server state is off, and timeout elapsed...
  if(bellADC >= bellThreshold && !bellState && millis() - bellChangeTime > bellTimeout) {
    jsonBuffer[bellName] = bellOnMsg;
    bellChangeTime = millis();
    bellState = 1;
    changedState = 1;
  }
  // doorbell off, server state is on, and enough time has passed...
  else if(bellADC < bellThreshold && bellState && millis() - bellChangeTime > bellTimeout) {
    jsonBuffer[bellName] = bellOffMsg;
    bellChangeTime = millis();
    bellState = 0;
    changedState = 1;
  }

  // we have new data to send...
  if(changedState) {
    serializeJson(jsonBuffer, Serial);
    Serial.println();
    changedState = 0;
  }
  #endif // end of normal loop

  // calibration loop() ////////////////////////////////////////////////////////
  #ifdef calibrationMode
  if(millis() - lastBattUploadTime > calibrationMode) {
    jsonBuffer[battName] = analogRead(battPin);
    jsonBuffer[mainsName] = analogRead(mainsPin);
    jsonBuffer[bellName] = analogRead(bellPin);
    serializeJson(jsonBuffer, Serial);
    Serial.println();
    lastBattUploadTime = millis();
  }
  #endif
}
