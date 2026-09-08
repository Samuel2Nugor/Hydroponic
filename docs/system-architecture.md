# System Architecture

## Status

Draft — subject to team review and approval.

## Architecture overview

MicroHydros uses an ESP32-S3 sensor node to collect environmental measurements. The device performs basic validation and publishes measurements over Wi-Fi using MQTT.

Mosquitto and Node-RED run as separate Docker containers on a Raspberry Pi Zero 2W. Mosquitto routes MQTT messages, while Node-RED validates and processes them.

InfluxDB and Grafana are optional extensions outside the MVP. They may later run as containers on a separate laptop without requiring changes to the sensor firmware.

## Component responsibilities

| Component | Responsibility | MVP |
| --------- | -------------- | --- |
| Internal SHT31 | Measure internal air temperature and relative humidity | Yes |
| External SHT31 | Measure external air temperature | Yes |
| DS18B20 | Measure water or nutrient-solution temperature | Yes |
| ESP32-S3 | Read sensors, perform initial validation and publish MQTT messages | Yes |
| Mosquitto | Route MQTT messages between publishers and subscribers | Yes |
| Node-RED | Parse, validate and process measurements | Yes |
| InfluxDB | Store validated historical measurements | No |
| Grafana | Display historical measurements and trends | No |
| Telegram | Deliver external alarm notifications | No |

## Deployment architecture

The ESP32-S3 and Raspberry Pi Zero 2W communicate through the same local network. The Raspberry Pi hosts the core backend services using Docker Compose.

Optional historical-storage and visualisation services run on a separate laptop to avoid overloading the Raspberry Pi Zero 2W.

```mermaid
```

flowchart TD
    Sensors["2 × SHT31 + DS18B20"]
    ESP["ESP32-S3<br/>Sensor node"]

    subgraph Pi["Raspberry Pi Zero 2W — Core MVP"]
        MQTT["Mosquitto container<br/>MQTT broker"]
        NodeRED["Node-RED container<br/>Validation and processing"]
    end

    subgraph Laptop["Separate laptop — Optional extension"]
        InfluxDB["InfluxDB container<br/>Historical storage"]
        Grafana["Grafana container<br/>Visualisation"]
    end

    Sensors --> ES`
    ESP -->|"MQTT over Wi-Fi"| MQTT
    MQTT --> NodeRED
    NodeRED -->|"Validated MQTT data"| MQTT
    MQTT -.->|"After MVP"| InfluxDB
    InfluxDB -.-> Grafana

## Docker deployment

The core services are managed using Docker Compose.

- Mosquitto and Node-RED run as separate containers.
- Containers restart automatically after failure or host restart.
- Persistent volumes preserve configuration and Node-RED data.
- Mosquitto configuration, Node-RED flow exports and Docker Compose files may be version controlled.
- Passwords, tokens, .env files and credential secrets must not be committed.

## Measurement data flow

1. The ESP32-S3 reads all three sensors every 30 seconds.
2. It performs initial checks for sensor communication or conversion failures.
3. It creates a JSON payload containing the measurements, sensor statuses, device ID, sequence number and uptime.
4. It publishes the payload to the device's raw MQTT topic using QoS 1.
5. Node-RED subscribes to the raw topics and validates each payload.
6. Node-RED adds a UTC timestamp representing when the message was received.
7. A complete and valid payload is published to the validated topic.
8. An incomplete or invalid payload is published to the rejected topic and is not forwarded to storage or visualisation.
9. Optional services can later consume validated measurements without requiring changes to the ESP32 firmware.

## MQTT topics

```text
```

microhydros/v1/devices/esp32s3-01/telemetry/raw
microhydros/v1/devices/esp32s3-01/telemetry/validated
microhydros/v1/devices/esp32s3-01/telemetry/rejected
microhydros/v1/devices/esp32s3-01/status

Node-RED subscribes to raw measurements from every compatible device using:

```text
```

microhydros/v1/devices/+/telemetry/raw

## Delivery Behaviour

| Property               | Decision           |
| ---------------------- | ------------------ |
| Measurement interval   | 30 seconds         |
| MQTT QoS               | 1                  |
| Retained telemetry     | No                 |
| Retained device status | Yes                |
| Timestamp authority    | Node-RED           |
| Timestamp format       | UTC using ISO 8601 |
| Offline buffering      | Outside the MVP    |

QoS 1 can deliver a message more than once, downstream components can use **device_id** and **sequence** together to identify duplicates.

The timestamp should look like this:

```text
```
2026-09-08T10:15:30

## Validation and failure handling

Validation is performed at two levels.

### ESP32-S3 validation

The ESP32-S3 checks whether each sensor was read successfully before publishing.

If a sensor fails:

- Its measurement value is set to `null`.
- Its sensor status is set to an error value such as `read_error`.
- `NaN`, infinity or invented replacement values must not be published.
- The failed payload may reach the raw topic for diagnostics, but it cannot become validated telemetry.

### Node-RED validation

Node-RED rejects the message if:

- The payload is empty or is not valid JSON.
- A required field is missing.
- A required measurement is `null`.
- A measurement is not a finite number.
- The schema version is unsupported.
- The device ID is empty or does not match the MQTT topic.
- The sequence number is not a non-negative integer.
- A required sensor status is not `ok`.
- A measurement is outside the configured plausible sensor range.

Rejected messages must not be forwarded to InfluxDB, Grafana or other consumers of validated data.

### Validation versus alarms

Validation determines whether data is trustworthy enough to process process.

AlarmThis should be:

Validation determines whether data is trustworthy enough to process.

An alarm determines whether a valid measurement is outside the desired growing conditions conditions.

### Device availability

The ESP32-S3 publishes its availability to:

```text
```

microhydros/v1/devices/esp32s3-01/status

The online status and MQTT Last Will and Testament message are retained.

```JSON
```

{
  "status": "online"
}

If the ESP32-S3 disconnects unexpectedly, Mosquitto publishes:

```JSON
```

{
  "status": "offline"
}

This allows Node-RED to distinguish between stable measurements and a device that has stopped communicating.