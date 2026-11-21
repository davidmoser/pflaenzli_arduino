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

The main loop in `pflaenzli_arduino.ino` orchestrates four modules. A hardware watchdog (`WDT`, 5 s) resets the board if the loop hangs. Every 60 s the loop reconnects WiFi if needed and retries the config fetch if it hasn't succeeded yet. Network I/O uses 1 s timeouts, and `WiFi.begin()` is capped at 3 s (library default 10 s), so nothing blocks long enough to trip the watchdog. At boot, `logResetCause()` prints why the MCU last reset (power-on, brownout, watchdog, software, reset button) from the RA4M1 reset status registers; the 60 s check also prints WiFi status and RSSI. Serial output is the only place these diagnostics appear.

- **`sensor_manager`** — reads moisture from analog pin A0, sends readings to the backend via `network_manager`. Tracks the next measurement time internally.
- **`pump_controller`** — drives a solenoid valve (pin 7) and pump (pin 8). The valve opens first, then the pump activates after `valveDuration` seconds. The pump runs for `pumpDuration` seconds then both shut off. `deactivatePump()` also cancels a start still waiting on the valve delay, and only reports `{"action": false}` to the backend if the pump was actually running, so boots, reconnects and redundant stops don't create pump-off rows. `pumpEnabled` is stored but doesn't gate starts. Uses deferred timers (`pumpSwitchOnTime` / `pumpSwitchOffTime`) checked each loop iteration.
- **`network_manager`** — manages WiFi (via WiFiS3), runs an HTTP server on port 80 for incoming commands, and an HTTP client that talks to the backend on port 8000. Uses a custom `sendHttpReply()` instead of `ArduinoHttpServer::StreamHttpReply` to avoid a ~10s stall bug.
- **`command_handler`** — routes incoming HTTP requests: `GET/PUT /configuration` for reading/writing runtime config, `PUT /triggerMeasurement` to force a reading, `PUT /pump/start|stop` for pump control. The backend uses `pump/start` for both manual and scheduled waterings.

## Configuration

Runtime parameters (defined in `config.h`, initialized in `config.cpp`) are fetched from the backend via `GET /api/configuration/{id}` (id 1) at boot, retried every 60 s until it succeeds, and can be updated at runtime through the local HTTP server. Key parameters: `measurementInterval`, `pumpDuration`, `valveDuration`, `measurementEnabled`, `pumpEnabled`.

## Backend API Endpoints (outbound)

The Arduino calls these on the backend (port 8000):
- `GET /api/configuration/{id}` — fetch config on boot
- `POST /api/sensor` — send moisture readings (`{"moisture_level": float}`)
- `POST /api/pump` — send pump state changes (`{"action": bool}`)

## System Overview

Pflaenzli is split across three sibling repos under `../` (`pflaenzli_arduino`, `pflaenzli_backend`, `pflaenzli_frontend`). Each has its own CLAUDE.md; this section is kept identical in all three.

```
browser ──HTTPS──▶ nginx (basic auth) ──/api/──▶ backend (gunicorn :8000) ◀── cron: POST /internal/tick (every minute)
                                                    │                ▲
                     PUT /configuration,            │                │  GET /api/configuration/1 (boot, retried until success)
                     PUT /pump/start|stop,          ▼                │  POST /api/sensor   (each reading)
                     PUT /triggerMeasurement     Arduino (HTTP :80) ─┘  POST /api/pump     (each pump on/off)
```

- **Backend is the hub.** The frontend talks only to the backend (never to the Arduino). The backend pushes commands to the Arduino, and the Arduino reports back to the backend.
- **Scheduling lives in the backend.** The minutely tick plans the day once (Open-Meteo ET0 → number of pump events spread over the watering window) and fires due events by sending `pump/start` to the Arduino. The Arduino knows nothing about schedules: each start runs one cycle (open valve, wait `valve_duration`, pump for `pump_duration`, close both). Automatic moisture measurements run on the Arduino's own timer (`measurement_interval`).
- **`PumpAction` rows reflect the real pump state.** They are only created when the Arduino reports via `POST /api/pump`. Manual starts and scheduled starts only relay the command.
- **Shared contract:** the JSON config fields `measurement_interval`, `pump_duration`, `valve_duration`, `measurement_enabled`, `pump_enabled` are used by all three repos, so renaming one touches all three. The irrigation fields (`pump_seconds_per_mm`, `calc_time`, `window_start`, `window_end`) are used only by the backend and frontend.
- **Monitoring:** each tick reports to healthchecks.io (dead-man's switch: stale moisture readings, scheduled starts the Arduino never confirmed).
- **Production** runs on a Raspberry Pi on the LAN: nginx serves the built frontend and proxies `/api/` to the gunicorn systemd service. `/internal/` is not proxied. Deploys are pull-based: push `main` to GitHub, then `git pull` on the Pi. The repos are public, so IPs, coordinates and secrets stay in gitignored files (`arduino_secrets.h`, backend `.env`, frontend `.env.production`).
