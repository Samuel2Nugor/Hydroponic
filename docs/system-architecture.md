# System Architecture

## Status

Draft — subject to team review and approval.

## Architecture overview

MicroHydros uses an ESP32-S3 sensor node to collect environmental measurements. The device performs basic validation and publishes measurements over Wi-Fi using MQTT.

Mosquitto and Node-RED run as separate Docker containers on a Raspberry Pi Zero 2W. Mosquitto routes MQTT messages, while Node-RED validates and processes them.

InfluxDB and Grafana are optional extensions outside the MVP. They may later run as containers on a separate laptop without requiring changes to the sensor firmware.

## Component responsibilities

| Component | Responsibility | MVP |
|-----------|----------------|-----|
| Internal SHT31 | Measure internal air temperature and relative humidity | Yes |
| External SHT31 | Measure external air temperature | Yes |
| DS18B20 | Measure water or nutrient-solution temperature | Yes |
| ESP32-S3 | Read sensors, perform initial validation and publish MQTT messages | Yes |
| Mosquitto | Route MQTT messages between publishers and subscribers | Yes |
| Node-RED | Parse, validate and process measurements | Yes |
| InfluxDB | Store validated historical measurements | No |
| Grafana | Display historical measurements and trends | No |
| Telegram | Deliver external alarm notifications | No |