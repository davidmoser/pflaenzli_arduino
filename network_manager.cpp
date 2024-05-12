#include "network_manager.h"
#include "command_handler.h"
#include "arduino_secrets.h"

#include <WiFiS3.h>
#include <WiFiServer.h>
#include <ArduinoHttpServer.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

WiFiServer server(80); // Initialize the server to listen on port 80
WiFiClient wifiClient;
HttpClient client(wifiClient, SERVER_IP, 8000);
bool receivedConfiguration = false;

void setupNetwork() {
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        return;
    }

    Serial.println("Attempting to connect to SSID: " + String(NETWORK_SSID));
    WiFi.begin(NETWORK_SSID, NETWORK_PASSWORD);
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected");
    } else {
      Serial.println("WiFi disconnected");
      return;
    }
    
    client.setHttpResponseTimeout(1000);

    server.begin();  // Start the server
    Serial.println("Server started");
    printWiFiStatus();
}

void reconnectNetwork() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.println("Connection lost, reconnecting");
  receivedConfiguration = false;
  setupNetwork();
}

void checkForCommands() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    WiFiClient client = server.available();   // Listen for incoming clients

    if (client) {                             // If a new client connects,
        ArduinoHttpServer::StreamHttpRequest<1000> httpRequest(client);
        if(httpRequest.readRequest()) {
          CommandResult result = handleCommands(httpRequest);
          if(result.success) {
            ArduinoHttpServer::StreamHttpReply httpReply(client, "application/json");
            httpReply.send(result.body, "");
            Serial.println("Command executed:" + httpRequest.getResource()[0] + "/" + httpRequest.getResource()[1]);
          } else {
            ArduinoHttpServer::StreamHttpErrorReply httpReply(client, "application/json", "500");
            httpReply.send(result.body);
            Serial.println("Error handling command: " + result.body);  
          }
        }
        // Clear the client's request and close the connection
        client.stop();
    }
}

void retrieveConfiguration() {
  if (WiFi.status() != WL_CONNECTED || receivedConfiguration) return;
  Serial.println("Retrieving configuration");

  client.beginRequest();
  client.get("/api/configuration/" + String(configuration_id));
  client.sendHeader("Content-Type", "application/json");
  client.endRequest();

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  if (statusCode < 0) {
    Serial.println("Status code: " + String(statusCode));
    Serial.println("Can't retrieve configuration");
    return;
  }

  Serial.println("Received configuration");
  setConfiguration(response);
  receivedConfiguration = true;
}


void sendSensorData(float moisture) {
  JsonDocument json;
  json["moisture_level"] = moisture;
  String requestBody;
  serializeJson(json, requestBody);
  sendData("/api/sensor", requestBody);
}

void sendPumpStatus(bool pumpStatus) {
  JsonDocument json;
  json["action"] = pumpStatus;
  String requestBody;
  serializeJson(json, requestBody);
  sendData("/api/pump", requestBody);
}

void sendData(String endpoint, String body) {
  if (WiFi.status() != WL_CONNECTED) return;
  Serial.println("Sending data to " + endpoint);

  client.beginRequest();
  client.post(endpoint);
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", body.length());
  client.beginBody();
  client.print(body);
  client.endRequest();

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  if(statusCode < 0) {
    Serial.println("Status code: " + String(statusCode));
    Serial.println("Response: " + response);
  }
}


void printWiFiStatus() {
    Serial.println("SSID: " + String(WiFi.SSID()));
    Serial.println("IP Address: " + WiFi.localIP().toString());
}
