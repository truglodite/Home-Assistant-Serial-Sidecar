#include <Arduino.h>
#include <ArduinoJson.h>

void setup() {
  Serial.begin(115200);
}

void loop() {
  StaticJsonBuffer<100> jsonBuffer;
  JsonObject& json = prepareResponse(jsonBuffer);
  json.printTo(Serial);
  Serial.println();
  delay(2000);
}

JsonObject& prepareResponse(JsonBuffer& jsonBuffer) {
  JsonObject& root = jsonBuffer.createObject();
  root["temperature"] = analogRead(A0);
  root["humidity"] = analogRead(A1);
  return root;
}
