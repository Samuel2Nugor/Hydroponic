# Hardware Selection

## Current status

The prototype hardware has arrived.

Exact board revisions, breakout-board features, pin labels and electrical requirements must be verified through physical inspection before wiring. No GPIO assignment is final yet.

The development Mac is the active Docker host. The Raspberry Pi Zero 2W is retained as optional hardware but is not part of the current active deployment.

## Selected prototype hardware

| Component | Quantity | Purpose | Integration state |
| --------- | -------- | ------- | ----------------- |
| ESP32-S3 development board | 1 | Read sensors and publish raw MQTT telemetry over Wi-Fi | Hardware received; exact model pending verification |
| SHT31 module | 1 | Internal air temperature and relative humidity | Hardware received; module pending verification |
| DS18B20 temperature sensor | 1 | External air temperature | Hardware received; wiring pending verification |
| Waterproof DS18B20 probe | 1 | Water or nutrient-solution temperature | Hardware received; probe wiring pending verification |
| Development Mac | 1 | Run the six Docker Compose services | Active |
| Raspberry Pi Zero 2W | 1 | Optional future reduced deployment | Deferred |

The system’s software responsibilities and deployment topology are defined in [system architecture](system-architecture.md). MQTT measurement names and units are defined in the [data contract](data-contract.md).

## ESP32-S3

The ESP32-S3 was selected because it provides Wi-Fi, sufficient GPIO interfaces and broad development-tool support.

Its planned responsibilities are:

- Read the internal SHT31 over I²C
- Read the external and water DS18B20 sensors over 1-Wire
- Detect basic sensor-read failures
- Create the raw telemetry payload
- Maintain `boot_id`, `sequence` and `uptime_ms`
- Publish raw MQTT telemetry
- Configure retained online status and MQTT Last Will
- Reconnect after Wi-Fi or MQTT interruption

The following details must be read from the physical board before wiring:

- Exact manufacturer and board model
- USB connector and programming interface
- Pin labels
- Safe I²C GPIO choices
- Safe 1-Wire GPIO choice
- Pins reserved for boot, flash or onboard peripherals
- Power-input and 3.3 V output limitations

## SHT31 module

The internal SHT31 measures internal air temperature and relative humidity.
Its sensor ID is `internal_sht31`.

Before wiring, verify the breakout board's pin labels, supply voltage,
I²C address and any built-in pull-up resistors.

## DS18B20 temperature sensors

Two DS18B20 sensors are planned: `external_ds18b20` measures external
air temperature, and the waterproof `water_ds18b20` probe measures water
or nutrient-solution temperature.

Each DS18B20 has a unique serial code. Record which code belongs to each
location so the firmware cannot swap external and water readings. Whether
they share a 1-Wire bus or use separate GPIOs remains a wiring decision.

A 1-Wire data line needs an appropriate pull-up to the sensor supply.
A `4.7 kΩ` resistor is planned, subject to checking the actual modules
and wiring.

Before connecting it:

- Verify the probe manufacturer or supplier wiring
- Do not assume wire colours are standardized
- Confirm supply, ground and data conductors
- Confirm the supported supply voltage
- Confirm whether the probe already contains a pull-up resistor
- Confirm that its output is safe for ESP32-S3 3.3 V logic

Both sensors must not be powered until its wiring and each sensor´s wiring independently.

## Supporting hardware

| Component | Purpose |
| --------- | ------- |
| Breadboard | Temporary prototype assembly |
| Jumper wires | Sensor and ESP32-S3 connections |
| `4.7 kΩ` resistor | Planned DS18B20 data pull-up |
| USB data cable | Program and power the ESP32-S3 |
| Multimeter | Verify continuity, voltage and uncertain probe wiring |

Raspberry Pi power and storage accessories are only required if Pi deployment is revisited.

## Verification checklist

Complete these checks before producing the wiring diagram:

- [ ] Record the exact ESP32-S3 board model and pin labels
- [ ] Identify the internal SHT31 breakout board, its I²C address and any built-in pull-up resistors
- [ ] Identify the supply, ground and data wires for both DS18B20 sensors
- [ ] Confirm each DS18B20 sensor's supply voltage and wiring
- [ ] Record each DS18B20's unique serial code and physical location
- [ ] Decide whether the DS18B20 sensors will share a 1-Wire bus or use separate GPIOs
- [ ] Confirm the required 1-Wire pull-up resistor arrangement
- [ ] Select safe ESP32-S3 GPIO pins and verify 3.3 V logic levels
- [ ] Verify the prototype power requirements
- [ ] Test each sensor independently before combining them

## Working assumptions

These assumptions are not yet verified:

- The internal SHT31 can use a suitable ESP32-S3 I²C connection
- Both DS18B20 sensors can be connected safely to the ESP32-S3
- Their serial codes can be mapped reliably to external and water locations
- All signal pull-ups operate at safe 3.3 V logic levels
- The available USB supply can power the complete prototype reliably

Resolve any failed assumption before combining all three sensors.
