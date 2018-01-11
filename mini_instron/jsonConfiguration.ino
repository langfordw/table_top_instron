#include <ArduinoJson.h>

#define ARRAY_SIZE 32

StaticJsonBuffer<512> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

void constructJSONObject() {
  JsonObject& device = root.createNestedObject("device");

  device["name"] = "Voice Coil Driver";
  device["author"] = "Will Langford";
  
  JsonArray& controls = jsonBuffer.createArray();
  
    JsonObject& control1 = controls.createNestedObject();
    control1["name"] = "enable";
    control1["type"] = "range";
    control1["min"] = 0;
    control1["max"] = 1600;
    control1["init"] = 0;
  
    JsonObject& control2 = controls.createNestedObject();
    control2["name"] = "phase";
    control2["type"] = "boolean";
    control2["init"] = 0;

    JsonObject& control3 = controls.createNestedObject();
    control3["name"] = "frequency";
    control3["type"] = "range";
    control3["min"] = 0;
    control3["max"] = 10;
    control3["init"] = 1;
  
    device["controls"] = controls;

  JsonArray& data = jsonBuffer.createArray();
  
    JsonObject& data1 = data.createNestedObject();
    data1["name"] = "currentSense";
    data1["type"] = "analog";
    data1["unit"] = "mA";
    data1["plot"] = "Y";

    JsonObject& data2 = data.createNestedObject();
    data2["name"] = "sourceVoltage";
    data2["type"] = "analog";
    data2["unit"] = "V";
    data2["plot"] = "Y";

    JsonObject& data3 = data.createNestedObject();
    data3["name"] = "frequency";
    data3["type"] = "analog";
    data3["unit"] = "Hz";
    data3["plot"] = "Y";

    device["data"] = data;
    
//  JsonArray& commands = jsonBuffer.createArray();
//  
////    JsonObject& hotkey = hotkeys.createNestedObject();
//    commands["?"] = "getConfiguration";
//    commands["g"] = "go";
//    commands["x"] = "stop";
//    
//    device["commands"] = commands;
}

void sendJSONObject() {
  root.prettyPrintTo(Serial);
  Serial.println("");
}
