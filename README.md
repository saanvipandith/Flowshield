# FlowShield — Flood Early Warning & Response Dashboard

FlowShield is an ESP32-connected flood early-warning prototype that combines sensor telemetry, an interactive map, a time-based flood simulation, risk classification, affected-population estimates, and a nearest-safe-region evacuation route.

## What the project demonstrates

- **ESP32 / Velxio telemetry:** rainfall/inflow, drainage, water-level, GPS latitude/longitude and elevation data.
- **Interactive map:** clicking a location selects a point and sends its latitude/longitude to the ESP32 through MQTT.
- **Flood simulation:** models water accumulation and movement across a connected region grid over time.
- **Risk classification:** Safe, Warning and Critical regions.
- **Decision-support panels:** maximum water level, critical-region count, affected population and time to critical conditions.
- **Evacuation support:** identifies a nearest safe region and displays a simple grid route.
- **Scenario comparison:** Light, Monsoon and Extreme scenarios.
- **Citizen alert:** generates a local, template-based warning message from the current model state.

> **Prototype note:** The flood model is a demonstration model, not a calibrated hydrological or official emergency-warning system. Outputs should not be treated as real-world evacuation instructions.

## Repository structure

```text
FlowShield-Flood-Early-Warning/
├── dashboard/
│   ├── index.html
│   ├── app.js
│   ├── map-core.js
│   ├── styles.css
│   ├── server.js
│   └── package.json
├── esp32/
│   ├── sketch.ino
│   ├── diagram.json
│   ├── libraries.json
│   └── libraries.txt
├── docs/
│   └── HACKATHON_SUBMISSION.md
├── submission/
│   └── FlowShield_MapToESP32_Final_Fix.zip
├── .gitignore
└── README.md
```

## Run the dashboard

### Requirements

- Node.js 18 or newer
- A modern browser such as Chrome or Edge
- Internet access if the browser needs to load external MQTT/map resources used by the dashboard

### Start

```bash
cd dashboard
npm start
```

Then open:

```text
http://localhost:3000
```

Do **not** open the dashboard HTML with `file://` when testing the live MQTT workflow; use the local server.

## ESP32 / Velxio setup

The `esp32/` folder contains the ESP32 sketch and the Velxio simulation files.

1. Open the Velxio simulation.
2. Use `esp32/sketch.ino` as the ESP32 sketch.
3. Keep the supplied `diagram.json`, `libraries.json`, and `libraries.txt` with the simulation.
4. Start the ESP32 simulation and wait for the MQTT connection/subscription messages in Serial Monitor.
5. Start the FlowShield dashboard at `http://localhost:3000`.
6. Click a point on the dashboard map.
7. The selected latitude/longitude is published to the ESP32 through MQTT.
8. The ESP32 can then provide telemetry back to the dashboard.

### MQTT topics used by FlowShield

```text
Telemetry:  flowshield/node1/telemetry
Location:   flowshield/node1/command/location
Broker:     broker.hivemq.com
```

The browser uses MQTT over secure WebSockets, while the ESP32 sketch uses MQTT on the broker's TCP endpoint.

## Main sensor inputs

The ESP32 sketch is based on the supplied FlowShield hardware configuration, including:

- DHT22 temperature/humidity input
- Upstream and downstream ultrasonic water-level sensing
- Analog inflow and drainage inputs
- GPS on the second hardware serial interface
- RGB LED, buzzer and servo outputs

## How to demonstrate the project

A simple hackathon demo flow:

1. Start the Velxio ESP32 simulation.
2. Start the dashboard.
3. Select a location on the map.
4. Show the selected latitude/longitude being sent to the ESP32.
5. Change the simulated sensor controls.
6. Run the flood simulation.
7. Move the flood-time slider to show risk progression.
8. Show vulnerable regions, affected population, time to critical and the safe-region route.
9. Compare Light, Monsoon and Extreme scenarios.

## Submission archive

The `submission/` folder contains the packaged final ZIP for a quick hackathon upload.

## Team / hackathon information

team name:xtra
members: ramyasri badarawada,saanvi athanikar,saanvi pandith,suhani athanikar
 and submission URL here before publishing if your hackathon requires them.
  
## sources
ESP32 documentation — used for the IoT hardware, GPIO, Wi-Fi and sensor interfacing.
Adafruit SSD1306 / GFX libraries — used for the OLED display.
DHT sensor library — used for temperature and humidity readings.
TinyGPSPlus — used for GPS coordinate processing.
ESP32Servo — used for servo/drainage-gate control.
PubSubClient — used for MQTT communication between the ESP32 and dashboard.
Leaflet.js — used for the interactive flood map.
HiveMQ MQTT broker — used for MQTT communication during the ESP32/dashboard integration.
chatgpt for coding