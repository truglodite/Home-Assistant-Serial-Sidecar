// Hass Analog Sidecar
// main.ino
// by: Truglodite
// Code for a nano to handle analog signals for hass (add analogs to RPi).
// Sends raw ADC values. So calibrations and calcs are handled in hass.
#include <ArduinoJson.h>

// User Configuration //////////////////////////////////////////////////////////
// All times are in millis, and thresholds are ints
// For variables with a 'Threshold', updates are only sent to hass if the value
// changes more or less than the set threshold.
// Tune these thresholds high enough to stop undesired spamming from noise,
// and low enough to reliably capture all desired changes in state.
// Mains and brick are more or less on/off, so large values are fine.
// Smaller values will be needed for the doorbell. The exact value should be
// tuned to work with your chime, wiring, & transformer of choice.

#define battPin           A0     // backup battery voltage divider (10:5)
#define battName          "battery"
#define battUpdatePeriod  600000 // time between updates

#define mainsPin          A1     // mains voltage sense (5V wallwart or other)
#define mainsName         "mains"
#define mainsUpdatePeriod 2000   // min. time between updates
#define mainsThreshold    400    // min ADC change required before updating

#define bellPin           A2     // doorbell 5A ac hall sensor
#define bellName          "bell"
#define bellUpdatePeriod  2000   // min. time between updates
#define bellThreshold     10     // min ADC change required before updating
#define bellSampleTime    20     // msec wait for peaks (60hz = 17ms period)

#define brickPin          A3     // UPS voltage sense (5v wallwart or other)
#define brickName         "brick"
#define brickUpdatePeriod 2000   // min. time between updates
#define brickThreshold    400    // min ADC change required before updating

#define baudrate          115200

// jsonBuffer... <57> is technically enough, so <200> seems safe. :P
// Use arduinojson.org/v6/assistant to compute the capacity if needed.
StaticJsonDocument<200> jsonBuffer;

// Globals /////////////////////////////////////////////////////////////////////
unsigned long battChangeTime = 0;
int battADC = 0;
unsigned long mainsChangeTime = 0;
int mainsState = 0;
int mainsADC = 0;
unsigned long bellChangeTime = 0;
int bellState = 0;
int bellADC = 0;
int bellMax = 0;
int bellMin = 1023;
unsigned long bellSampleStart = 0;
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
  mainsADC = analogRead(mainsPin);
  jsonBuffer[mainsName] = mainsADC;
  mainsState = mainsADC;
  bellSampleStart = millis();
  // sample bell ADC and record peaks/valleys
  while(millis() - bellSampleStart < bellSampleTime) {
    bellADC = analogRead(bellPin);
    if(bellADC > bellMax) {
      bellMax = bellADC;
    }
    else if (bellADC < bellMin) {
      bellMin = bellADC;
    }
  }
  // convert bell peaks/valleys to 'Vpp' (actually adc peak to peak)
  bellADC = bellMax - bellMin;
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
  if(millis() - battChangeTime > battUpdatePeriod)  {
    jsonBuffer[battName] = analogRead(battPin);
    battChangeTime = millis();
    changedState = 1;
  }

  mainsADC = analogRead(mainsPin);
  // mains has changed enough to report it...
  if(mainsADC >= mainsState + mainsThreshold || mainsADC <= mainsState - mainsThreshold)  {
    // only change if enough time has passed
    if(millis() - mainsChangeTime > mainsUpdatePeriod) {
      jsonBuffer[mainsName] = mainsADC;
      mainsState = mainsADC;
      mainsChangeTime = millis();
      changedState = 1;
    }
  }

  // read bell voltage
  int bellMax = 0;  // reset peak to peak value every loop
  int bellMin = 1023;
  bellSampleStart = millis();
  // sample bell ADC and record peaks/valleys
  while(millis() - bellSampleStart < bellSampleTime) {
    bellADC = analogRead(bellPin);
    if(bellADC > bellMax) {
      bellMax = bellADC;
    }
    else if (bellADC < bellMin) {
      bellMin = bellADC;
    }
  }
  // convert bell peaks/valleys to 'Vpp' (actually adc peak to peak)
  bellADC = bellMax - bellMin;

  // bell has changed enough to report it...
  if(bellADC >= bellState + bellThreshold || bellADC <= bellState - bellThreshold)  {
    // only change if enough time has passed
    if(millis() - bellChangeTime > bellUpdatePeriod) {
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
    if(millis() - brickChangeTime > brickUpdatePeriod) {
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
