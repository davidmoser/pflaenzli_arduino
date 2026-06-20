# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Pflaenzli Arduino is firmware for an Arduino Uno R4 WiFi that automates plant watering. It reads soil moisture from an analog sensor, controls a solenoid valve and pump, and communicates with a companion backend server (`pflaenzli_backend`) over HTTP.

## Build and Upload

This is an Arduino sketch (`.ino`). Use the Arduino IDE or `arduino-cli` to compile and upload:

```bash
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi pflaenzli_arduino.ino
arduino-cli upload --fqbn arduino:renesas_uno:unor4wifi -p /dev/ttyACM0 pflaenzli_arduino.ino
```

There are no tests or linting tools configured.

## Required Libraries

- WiFiS3 (bundled with Arduino UNO R4 WiFi board package)
- ArduinoHttpServer
- ArduinoHttpClient
- ArduinoJson

## Secrets

Copy `arduino_secrets_example.h` to `arduino_secrets.h` and fill in WiFi credentials and the backend server IP. `arduino_secrets.h` is gitignored.

## Architecture

The main loop in `pflaenzli_arduino.ino` orchestrates four modules:

- **`sensor_manager`** — reads moisture from analog pin A0, sends readings to the backend via `network_manager`. Tracks the next measurement time internally.
- **`pump_controller`** — drives a solenoid valve (pin 7) and pump (pin 8). The valve opens first, then the pump activates after `valveDuration` seconds. The pump runs for `pumpDuration` seconds then both shut off. Uses deferred timers (`pumpSwitchOnTime` / `pumpSwitchOffTime`) checked each loop iteration.
- **`network_manager`** — manages WiFi (via WiFiS3), runs an HTTP server on port 80 for incoming commands, and an HTTP client that talks to the backend on port 8000. Uses a custom `sendHttpReply()` instead of `ArduinoHttpServer::StreamHttpReply` to avoid a ~10s stall bug.
- **`command_handler`** — routes incoming HTTP requests: `GET/PUT /configuration` for reading/writing runtime config, `PUT /triggerMeasurement` to force a reading, `PUT /pump/start|stop|reset` for manual pump control.

## Configuration

Runtime parameters (defined in `config.h`, initialized in `config.cpp`) are fetched from the backend on startup via `GET /api/configuration/{id}` and can be updated at runtime through the local HTTP server. Key parameters: `measurementInterval`, `pumpDuration`, `valveDuration`, `measurementEnabled`, `pumpEnabled`.

## Backend API Endpoints (outbound)

The Arduino calls these on the backend (port 8000):
- `GET /api/configuration/{id}` — fetch config on boot
- `POST /api/sensor` — send moisture readings (`{"moisture_level": float}`)
- `POST /api/pump` — send pump state changes (`{"action": bool}`)

## Related Projects

- **pflaenzli_backend** (`../pflaenzli_backend`) — Django REST API that stores sensor data, pump actions, and configuration. Runs on port 8000.
- **pflaenzli_frontend** (`../pflaenzli_frontend`) — Vue 3 SPA that displays moisture/pump charts and provides controls for the watering system.
