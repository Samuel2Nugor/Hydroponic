# Hardware Selection

## Current status

The sensor prototype is assembled and operational. One SHT31 and two waterproof DS18B20 probes have been read successfully by the ESP32-S3, including repeated five-second sampling and end-to-end telemetry validation.

The development host runs the six Docker Compose services. The Raspberry Pi Zero 2W is retained as optional hardware but is not part of the active deployment because it does not provide enough resource margin for the complete stack.

## Selected prototype hardware

| Component | Quantity | Purpose | Integration state |
| --- | ---: | --- | --- |
| ESP32-S3 development board | 1 | Read sensors and publish raw telemetry over Wi-Fi | Integrated and tested |
| SHT31 module | 1 | Internal air temperature and relative humidity | Integrated and tested |
| Waterproof DS18B20 probe | 1 | External temperature | Integrated and tested |
| Waterproof DS18B20 probe | 1 | Water or nutrient-solution temperature | Integrated and tested |
| Development host | 1 | Run the Docker Compose services | Active |
| Raspberry Pi Zero 2W | 1 | Optional future reduced deployment | Deferred |

The system responsibilities and deployment topology are described in [system architecture](system-architecture.md). MQTT fields, measurement names, statuses and units are defined in the [data contract](data-contract.md).

## ESP32-S3

The ESP32-S3 was selected because it provides Wi-Fi, sufficient GPIO interfaces, PSRAM support and broad ESP-IDF tooling.

The tested board reports:

- ESP32-S3 revision `v0.2`
- 16 MB SPI flash
- 8 MB PSRAM
- Two CPU cores
- USB serial access through `/dev/ttyACM0` on the development system

The module marking identifies an ESP32-S3 N16R8 configuration. The exact development-board manufacturer and product revision have not been confirmed, so documentation should not claim a more specific board model.

The firmware currently:

- Reads the SHT31 over IÂ²C
- Discovers both DS18B20 probes on one shared 1-Wire bus
- Assigns each DS18B20 to a stable role using its ROM address
- Represents missing or failed DS18B20 readings independently
- Creates the raw telemetry payload
- Maintains `boot_id`, `sequence` and `uptime_ms`
- Publishes telemetry over authenticated TLS MQTT with QoS 1
- Samples and publishes every five seconds by default

GPIO assignments, the telemetry interval, device identity and probe ROM addresses are configurable through the firmware Kconfig menu.

## Verified wiring

All sensors use the ESP32-S3 3.3 V supply and a common ground.

| Signal | ESP32-S3 connection | Notes |
| --- | --- | --- |
| SHT31 SDA | GPIO8 | IÂ²C data |
| SHT31 SCL | GPIO9 | IÂ²C clock |
| SHT31 VIN | 3.3 V | Tested successfully |
| SHT31 GND | GND | Common ground |
| Both DS18B20 data lines | GPIO5 | Shared 1-Wire bus |
| Both DS18B20 supply lines | 3.3 V | Externally powered mode |
| Both DS18B20 ground lines | GND | Common ground |
| 1-Wire pull-up | Data to 3.3 V | One `4.7 kÎ©` resistor for the shared bus |

Only one pull-up resistor is required for the shared 1-Wire bus. A second probe does not require a second resistor.

Wire colours are not treated as a general DS18B20 standard. The conductors on these specific probes were verified before connection; replacement probes must be checked independently.

## SHT31 module

The SHT31 provides:

- `internal_temperature_c`
- `internal_humidity_percent`
- Sensor ID `internal_sht31`

It is detected at IÂ²C address `0x44` and has produced repeated valid temperature and humidity readings. GPIO8 and GPIO9 are the tested defaults, but they can be changed through Kconfig.

The exact breakout-board manufacturer and whether its PCB includes IÂ²C pull-up resistors remain unconfirmed. Successful bench operation verifies the assembled prototype, not every electrical characteristic of the module.

## DS18B20 probes

Both waterproof DS18B20 probes share GPIO5 and are distinguished by their unique ROM addresses:

| Sensor role | Sensor ID | ROM address |
| --- | --- | --- |
| Water or nutrient-solution temperature | `water_ds18b20` | `BE0000006DEFD428` |
| External temperature | `external_ds18b20` | `DA00000070118128` |

The ROM-to-role mapping prevents the readings from being swapped because of discovery order. Both probes have been discovered together, assigned to the correct roles and read repeatedly.

The firmware uses the Espressif UART-based 1-Wire backend. On the current ESP32-S3 and ESP-IDF `v6.0.1` setup, an initial manual bus reset is required before the first ROM search. That hardware-specific workaround is contained in `firmware/esp32s3/main/ds18b20_manager.c`.

## Supporting hardware

| Component | Purpose |
| --- | --- |
| Breadboard | Temporary prototype assembly |
| Jumper wires | Sensor and ESP32-S3 connections |
| `4.7 kÎ©` resistor | Shared 1-Wire data pull-up |
| USB data cable | Program and power the ESP32-S3 |
| Digital multimeter | Verify voltage, continuity and uncertain probe wiring |

Raspberry Pi power and storage accessories are only required if a reduced Pi deployment is revisited.

## Verified prototype

The assembled prototype has been verified with all three sensors connected at the same time. The SHT31 was detected at `0x44`, and both DS18B20 probes were discovered on the shared GPIO5 bus and assigned to their configured roles.

The firmware sampled the sensors every five seconds, published the readings over TLS MQTT with QoS 1 and received acknowledgements from the broker. The backend accepted all four measurements with `validated=4 rejected=0`.
