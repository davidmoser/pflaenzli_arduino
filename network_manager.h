#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "config.h"

void setupNetwork();
void sendSensorData(float moisture);
void sendPumpStatus(bool pumpStatus);
void sendData(String endpoint, String body);
void checkForCommands();
void printWiFiStatus();
void reconnectNetwork();
void retrieveConfiguration();

#endif
