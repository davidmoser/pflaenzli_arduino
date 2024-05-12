#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

void initSensor();
float readMoisture();
unsigned long getNextMeasurementTime();
void triggerMeasurement();

#endif
