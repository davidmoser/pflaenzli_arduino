#include "command_handler.h"
#include <ArduinoJson.h>
#include "pump_controller.h"
#include "sensor_manager.h"
#include "config.h"

CommandResult handleCommands(ArduinoHttpServer::StreamHttpRequest<1000>& httpRequest) {
  const String body = String(httpRequest.getBody());
  const String endpoint = httpRequest.getResource()[0];
  ArduinoHttpServer::Method method = httpRequest.getMethod();
  
  if(endpoint == "configuration") {
    if(method == ArduinoHttpServer::Method::Get) {
      return CommandResult(true, getConfiguration());
    } else if(method == ArduinoHttpServer::Method::Put) {
      setConfiguration(body);
      return CommandResult();
    } else {
      Serial.println("Unexpected method");
      return CommandResult(false, "Unexpected method");
    }
  } else if(endpoint == "triggerMeasurement") {
    if(method == ArduinoHttpServer::Method::Put) {
      triggerMeasurement();
      return CommandResult();
    } else {
      Serial.println("Unexpected method");
      return CommandResult(false, "Unexpected method");
    }
  } else if(endpoint == "pump") {
    if(method == ArduinoHttpServer::Method::Put) {
      const String action = httpRequest.getResource()[1]; 
      if(action == "start") {
        activatePump();
      } else if(action == "stop") {
        deactivatePump();
      } else if(action == "reset") {
        consecutivePumps = 0;
      }
      return CommandResult();
    } else {
      Serial.println("Unexpected method");
      return CommandResult(false, "Unexpected method");
    } 
  }
}

String getConfiguration() {
  JsonDocument doc;
  doc["moisture_threshold"] = moistureThreshold;
  doc["measurement_interval"] = measurementInterval;
  doc["pump_duration"] = pumpDuration;
  doc["consecutive_pumps"] = consecutivePumps;
  doc["maxConsecutive_pumps"] = maxConsecutivePumps;
  doc["measurement_enabled"] = measurementEnabled;
  doc["pump_enabled"] = pumpEnabled;
  String response;
  serializeJson(doc, response);
  return response;
}

void setConfiguration(String payload) {
  JsonDocument doc;
  deserializeJson(doc, payload);
  if (doc.containsKey("moisture_threshold")) {
    moistureThreshold = doc["moisture_threshold"];
  }
  if (doc.containsKey("measurement_interval")) {
    measurementInterval = doc["measurement_interval"];
  }
  if (doc.containsKey("pump_duration")) {
    pumpDuration = doc["pump_duration"];
  }
  if (doc.containsKey("consecutive_pumps")) {
    consecutivePumps = doc["consecutive_pumps"];
  }
  if (doc.containsKey("max_consecutive_pumps")) {
    maxConsecutivePumps = doc["max_consecutive_pumps"];
  }
  if (doc.containsKey("measurement_enabled")) {
    measurementEnabled = doc["measurement_enabled"];
  }
  if (doc.containsKey("pump_enabled")) {
    pumpEnabled = doc["pump_enabled"];
    if(!pumpEnabled) {
      deactivatePump();
    }
  }
}
