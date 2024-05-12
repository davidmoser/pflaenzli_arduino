#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>  // Include this to recognize the Arduino-specific types and classes such as String
#include <ArduinoHttpServer.h>

class CommandResult {
  public:
    boolean success;
    String body;

    CommandResult(boolean s, String r) : success(s) {
      if(s) {
        body = r;
      } else {
        body = "{\"message\": \"" + r + "\"}";
      }
    }

    CommandResult() : success(true), body("{\"message\": \"OK\"}") {}
};

CommandResult handleCommands(ArduinoHttpServer::StreamHttpRequest<1000>& httpRequest);
void setConfiguration(String payload);
String getConfiguration();

#endif
