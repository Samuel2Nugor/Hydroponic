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
| SHT31 module | 1 | External air temperature | Hardware received; module pending verification |
| Waterproof DS18B20 probe | 1 | Water or nutrient-solution temperature | Hardware received; probe wiring pending verification |
| Development Mac | 1 | Run the six Docker Compose services | Active |
| Raspberry Pi Zero 2W | 1 | Optional future reduced deployment | Deferred |

The system’s software responsibilities and deployment topology are defined in [system architecture](system-architecture.md). MQTT measurement names and units are defined in the [data contract](data-contract.md).

## ESP32-S3

The ESP32-S3 was selected because it provides Wi-Fi, sufficient GPIO interfaces and broad development-tool support.

Its planned responsibilities are:

- Read both SHT31 modules over I²C
- Read the DS18B20 over 1-Wire
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

## SHT31 modules

The SHT31 is designed to measure both air temperature and relative humidity.

Current use:

| Sensor ID | Used measurements |
| --------- | ----------------- |
| `internal_sht31` | Internal temperature and internal relative humidity |
| `external_sht31` | External temperature |

The external SHT31 is also physically capable of measuring relative humidity, but external humidity is not part of the current data contract.

### I²C addressing

SHT31 devices normally support addresses `0x44` and `0x45`.

Both modules can share one I²C bus only if they can be configured with different addresses. Physical inspection must confirm whether the breakout modules expose an address-selection pad or pin.

If both modules are fixed to the same address, the alternatives are:

- Use separate ESP32-S3 I²C controllers or buses
- Add an I²C multiplexer
- Replace one module with an address-configurable version

### Environmental protection

A normal SHT31 breakout board is not waterproof.

The external sensor needs:

- Protection from rain and splashes
- Protection from direct sunlight
- Ventilation around the sensing element
- Placement that reduces condensation risk

A sealed enclosure without airflow would distort temperature and humidity readings.

## DS18B20 probe

The waterproof DS18B20 is selected for water or nutrient-solution temperature.

It communicates over 1-Wire and normally requires a pull-up resistor between its data and supply lines. A `4.7 kΩ` resistor is the current planned value.

Before connecting it:

- Verify the probe manufacturer or supplier wiring
- Do not assume wire colours are standardized
- Confirm supply, ground and data conductors
- Confirm the supported supply voltage
- Confirm whether the probe already contains a pull-up resistor
- Confirm that its output is safe for ESP32-S3 3.3 V logic

The probe must not be powered until its wiring has been identified.

## Supporting hardware

| Component | Purpose |
| --------- | ------- |
| Breadboard | Temporary prototype assembly |
| Jumper wires | Sensor and ESP32-S3 connections |
| `4.7 kΩ` resistor | Planned DS18B20 data pull-up |
| USB data cable | Program and power the ESP32-S3 |
| External-sensor enclosure or radiation shield | Protect the external SHT31 while permitting airflow |
| Multimeter | Verify continuity, voltage and uncertain probe wiring |

Raspberry Pi power and storage accessories are only required if Pi deployment is revisited.

## Verification checklist

Complete these checks before producing the wiring diagram:

- [ ] Record the exact ESP32-S3 board model
- [ ] Photograph or record all ESP32-S3 pin labels
- [ ] Identify the exact SHT31 breakout modules
- [ ] Confirm both SHT31 default addresses
- [ ] Confirm whether either SHT31 address can be changed
- [ ] Confirm whether the SHT31 modules include I²C pull-up resistors
- [ ] Identify the DS18B20 supply, ground and data wires
- [ ] Confirm DS18B20 supply voltage
- [ ] Confirm whether an external 1-Wire pull-up resistor is required
- [ ] Select safe ESP32-S3 GPIO pins
- [ ] Calculate or verify the prototype power requirements
- [ ] Test each sensor independently before combining them

## Working assumptions

These assumptions are not yet verified:

- Both SHT31 modules can operate on one I²C bus
- One module can use `0x44` and the other `0x45`
- The DS18B20 can use a dedicated ESP32-S3 GPIO
- All signal pull-ups operate at a safe 3.3 V logic level
- The available USB supply can power the complete prototype reliably

Any failed assumption must be resolved before combining all three sensors.
