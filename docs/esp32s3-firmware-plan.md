# ESP32-S3 Firmware Plan

## Purpose

This document defines the planned ESP32-S3 firmware structure, responsibilities and implementation order for the MicroHydros MVP.

The firmware will collect sensor measurements, perform basic read checks and publish raw telemetry to Mosquitto over Wi-Fi. The Python telemetry service remains responsible for authoritative validation and timestamping.

## Technology

- ESP-IDF v6.0.1
- C
- ESP-IDF Wi-Fi and event APIs
- ESP-MQTT managed component
- cJSON
- CMake

The initial firmware build will be performed on the Linux Mint development machine where ESP-IDF v6.0.1 is installed.

## Firmware responsibilities

The firmware must:

- Connect and reconnect to Wi-Fi.
- Connect and reconnect to Mosquitto using authentication.
- Configure the retained offline Last Will before connecting.
- Publish retained online status after connecting.
- Read the configured sensors every 30 seconds.
- Represent failed readings with `null` and an appropriate sensor status.
- Publish combined raw telemetry using QoS 1 without retention.
- Maintain a boot identifier, sequence number and uptime.
- Keep Wi-Fi and MQTT credentials outside Git.

The firmware must not:

- Perform authoritative backend validation.
- Add UTC timestamps.
- Publish directly to validated MQTT topics.
- Store measurements permanently.
- Contain committed credentials.
- Control pumps or other actuators during the MVP.

## Planned project structure

```text
hardware/esp32s3/
├── CMakeLists.txt
├── README.md
├── sdkconfig.defaults
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    ├── main.c
    ├── wifi_manager.c
    ├── wifi_manager.h
    ├── mqtt_manager.c
    ├── mqtt_manager.h
    ├── telemetry_payload.c
    ├── telemetry_payload.h
    ├── sensor_reader.c
    ├── sensor_reader.h
    └── secrets.example.h
```

A local secrets.h file will contain development credentials and must be ignored by Git. The example file will contain placeholders only.

The firmware should remain small. Additional modules should only be introduced when they represent a clear hardware or protocol responsibility`

## Runtime sequence

1. Initialize NVS.
2. Generate a new boot_id.
3. Initialize the sensor interface.
4. Connect to Wi-Fi.
5. Configure the MQTT client.
6. Configure the retained offline Last Will before connecting.
7. Connect to Mosquitto.
8. Publish retained online status after the MQTT connection succeeds.
9. Every 30 seconds:
    - Read each sensor.
    _ Perform basic read-success checks.
    - Assign the appropriate sensor status.
    - Build one combined raw JSON payload.
    _ Publish it with QoS 1 and retention disabled.
    - Increment the sequence number once.
10. Continue reconnecting after Wi-Fi or MQTT interruption.

MQTT retransmissions must not increment the application sequence number.

## Network configuration

The ESP32-S3 runs outside Docker. Therefore, its MQTT broker address must be the Raspberry Pi’s LAN IP address or resolvable hostname—not the Docker Compose service name mosquitto.

The following values require local configuration:

| Setting | Example |
| ------- | ------- |
| Wi-Fi SSID | replace-with-local-ssid |

| Wi-Fi password | replace-with-local-password |

| MQTT host | Raspberry Pi LAN IP |

| MQTT port | 1883 |

| MQTT username | esp32s3-01 |

| MQTT password | Local Mosquitto password |

| Device ID | esp32s3-01 |

## Development before sensors are available

The firmware skeleton must not invent realistic-looking sensor measurements.

Until physical sensors are connected, the sensor interface will return:

| Measurement | Value | Sensor status |
| ----------- | ----- | ------------- |
| Internal temperature | `null` | `not_detected` |
| Internal humidity | `null` | `not_detected` |
| External temperature | `null` | `not_detected` |
| Water temperature | `null` | `not_detected` |

This payload is valid raw telemetry, but the Python telemetry service will reject each unavailable measurement independently. That is expected and allows the MQTT path and rejection handling to be tested honestly.

Simulated valid measurements must not be added unless the data contract is extended so consumers can clearly distinguish simulated data from physical readings.

## Planned sensor mapping

| Contract fields | Physical sensor |
| --------------- | --------------- |
| `internal_temperature_c` | Internal SHT31 |
| `internal_humidity_percent` | Internal SHT31 |
| `external_temperature_c` | External SHT31 |
| `water_temperature_c` | Waterproof DS18B20 |

The exact I2C addresses, OneWire pin and ESP32-S3 GPIO assignments remain pending until the development board and sensors are physically confirmed.

## Security boundaries

- Wi-Fi and MQTT credentials must not be committed.
- `secrets.h`, `sdkconfig`, `sdkconfig.old` and build output must be ignored.
- `secrets.example.h` must contain placeholders only.
- Credentials will still be compiled into the development firmware binary, so firmware binaries must not be published as public artifacts.
- TLS is outside the current local-network MVP scope.
- The router must not forward MQTT port `1883` from the internet.

## Implementation acceptance criteria

The initial firmware implementation is complete when:

- The project builds for the ESP32-S3 using ESP-IDF v6.0.1.
- No credential or local network secret is tracked by Git.
- A new `boot_id` is generated on every boot.
- The sequence starts at `0` and advances once per raw telemetry message.
- Wi-Fi and MQTT reconnect after interruption.
- The MQTT Last Will is configured before connecting.
- Offline and online status messages use QoS 1 and are retained.
- Raw telemetry uses QoS 1 and is not retained.
- One combined raw payload is published every 30 seconds.
- The JSON matches `docs/data-contract.md`.
- Missing sensors produce `null` values with `not_detected` status.
- The existing Python telemetry service accepts the message structure and produces the expected rejection messages for unavailable sensors.

## Pending hardware decisions

The following must be confirmed before implementing physical sensor drivers:

- Exact ESP32-S3 development-board model.
- Available and safe GPIO pins.
- SHT31 I2C addresses.
- Whether both SHT31 devices can use different addresses on one I2C bus.
- DS18B20 OneWire GPIO and pull-up resistor placement.
- Power-supply and signal-voltage compatibility.
