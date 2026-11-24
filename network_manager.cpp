#include "network_manager.h"
#include "command_handler.h"
#include "arduino_secrets.h"

#include <WiFiS3.h>
#include <WiFiServer.h>
#include <ArduinoHttpServer.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include "WDT.h"
#include "logger.h"

WiFiServer server(80); // Initialize the server to listen on port 80
WiFiClient wifiClient;
HttpClient client(wifiClient, SERVER_IP, 8000);
bool receivedConfiguration = false;
static unsigned long incomingCount = 0, outboundOk = 0, outboundFail = 0;

static String macToString(const uint8_t* mac) {
    String s;
    for (int i = 0; i < 6; i++) {
        if (i) s += ":";
        if (mac[i] < 0x10) s += "0";
        s += String(mac[i], HEX);
    }
    return s;
}

static String currentBSSID() {
    uint8_t bssid[6] = {0};
    return WiFi.BSSID(bssid) ? macToString(bssid) : "?";
}

void setupNetwork() {
    if (WiFi.status() == WL_NO_MODULE) {
        logInfo("Communication with WiFi module failed!");
        return;
    }

    uint8_t mac[6] = {0};
    WiFi.macAddress(mac);
    logInfo("WiFi module firmware " + String(WiFi.firmwareVersion()) + ", MAC " + macToString(mac));
    logInfo("Attempting to connect to SSID: " + String(NETWORK_SSID));
    WiFi.setTimeout(3000);  // default 10 s exceeds the 5 s watchdog
    WDT.refresh();
    unsigned long beginStart = millis();
    WiFi.begin(NETWORK_SSID, NETWORK_PASSWORD);
    logDebug("WiFi.begin took " + String(millis() - beginStart) + " ms, status " + String(WiFi.status()));
    
    if (WiFi.status() == WL_CONNECTED) {
      logInfo("WiFi connected");
    } else {
      logInfo("WiFi disconnected");
      return;
    }

    // WiFi.begin() can return at association while DHCP is still in flight
    // (localIP() == 0.0.0.0). Starting the server then binds it to no address and
    // it never accepts incoming connections. Wait for a real lease first.
    unsigned long ipDeadline = millis() + 10000;
    while (WiFi.localIP() == IPAddress(0, 0, 0, 0) && millis() < ipDeadline) {
      WDT.refresh();
      delay(100);
    }

    logDebug("IP " + WiFi.localIP().toString() + " after " + String(millis() - beginStart) + " ms, gateway " + WiFi.gatewayIP().toString());
    client.setHttpResponseTimeout(1000);

    server.begin();  // Start the server (now that we have a valid IP)
    logInfo("Server started");
    printWiFiStatus();
}

void reconnectNetwork() {
  uint8_t status = WiFi.status();
  if (status == WL_CONNECTED) return;
  logInfo("Connection lost (status " + String(status) + "), reconnecting");
  receivedConfiguration = false;
  WiFi.disconnect();
  setupNetwork();
}

// Replaces ArduinoHttpServer::StreamHttpReply which stalls ~10s due to a drain loop in sendHeader().
// See: https://github.com/QuickSander/ArduinoHttpServer/issues/26
void sendHttpReply(WiFiClient& client, int statusCode, const String& body) {
    String statusText = statusCode == 200 ? "OK" : "Internal Server Error";
    client.print("HTTP/1.1 " + String(statusCode) + " " + statusText + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print("Connection: close\r\n");
    client.print("Content-Length: " + String(body.length()) + "\r\n");
    client.print("\r\n");
    client.print(body);
}

void checkForCommands() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    WiFiClient client = server.available();   // Listen for incoming clients

    if (client) {                             // If a new client connects,
        unsigned long start = millis();
        incomingCount++;
        logDebug("Incoming connection from " + client.remoteIP().toString());
        client.setTimeout(1000);

        // Drop silent / half-open connections that send nothing. Without this an
        // incoming connection that never delivers a request blocks readRequest()
        // and wedges the single-threaded loop (root cause of the 2026-06-28 hang).
        unsigned long deadline = millis() + 1000;
        while (client.connected() && !client.available() && millis() < deadline) {
            delay(1);
        }
        if (!client.available()) {
            logInfo("Incoming connection sent no data within 1 s, dropped");
            client.stop();
            return;
        }

        ArduinoHttpServer::StreamHttpRequest<1000> httpRequest(client);
        if(httpRequest.readRequest()) {
          logDebug("Request " + httpRequest.getResource().toString());
          CommandResult result = handleCommands(httpRequest);
          if(result.success) {
            sendHttpReply(client, 200, result.body);
            logInfo("Command executed: " + httpRequest.getResource()[0] + "/" + httpRequest.getResource()[1]);
          } else {
            sendHttpReply(client, 500, result.body);
            logInfo("Error handling command: " + result.body);
          }
        } else {
          logInfo("Failed to read incoming request");
        }
        logDebug("Incoming connection handled in " + String(millis() - start) + " ms");
        // Clear the client's request and close the connection
        client.stop();
    }
}

void retrieveConfiguration() {
  if (WiFi.status() != WL_CONNECTED || receivedConfiguration) return;
  logInfo("Retrieving configuration");
  unsigned long start = millis();

  client.beginRequest();
  client.get("/api/configuration/" + String(configuration_id));
  client.sendHeader("Content-Type", "application/json");
  client.endRequest();

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  if (statusCode < 0) {
    outboundFail++;
    logInfo("Can't retrieve configuration, status " + String(statusCode) + " after " + String(millis() - start) + " ms");
    return;
  }

  outboundOk++;
  logInfo("Received configuration (HTTP " + String(statusCode) + ", " + String(millis() - start) + " ms)");
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
  logInfo("Sending data to " + endpoint);
  unsigned long start = millis();

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

  if (statusCode < 0) {
    outboundFail++;
    logInfo("Send to " + endpoint + " failed, status " + String(statusCode) + " after " + String(millis() - start) + " ms");
  } else {
    outboundOk++;
    logDebug("Sent to " + endpoint + ": HTTP " + String(statusCode) + ", " + String(millis() - start) + " ms");
  }
}


void printWiFiStatus() {
    static String lastBSSID;
    String bssid = currentBSSID();
    String line = "WiFi status " + String(WiFi.status()) + ", SSID " + String(WiFi.SSID()) + ", BSSID " + bssid +
                  ", IP " + WiFi.localIP().toString() + ", RSSI " + String(WiFi.RSSI()) + " dBm";
    if (lastBSSID.length() && bssid != lastBSSID) line += " (roamed from " + lastBSSID + ")";
    lastBSSID = bssid;
    logInfo(line);
    logDebug("Unix time " + String(WiFi.getTime()) + ", incoming " + String(incomingCount) +
             ", outbound ok " + String(outboundOk) + " / failed " + String(outboundFail));
}
