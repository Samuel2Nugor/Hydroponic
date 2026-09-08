# Hardware Selection

## Status

Draft — component models and wiring must be verified before implementation.

## Purpose

This document records the hardware selected for the MicroHydros prototype, the reason for each selection and any checks that must be completed before wiring.

## Core hardware

| Component | Quantity | Purpose | MVP |
| --------- | -------- | ------- | --- |
| ESP32-S3 development board | 1 | Read sensors, validate measurements and publish MQTT data over Wi-Fi | Yes |
| SHT31 sensor module | 1 | Measure internal air temperature and relative humidity | Yes |
| SHT31 sensor module | 1 | Measure external air temperature | Yes |
| Waterproof DS18B20 probe | 1 | Measure water or nutrient-solution temperature | Yes |
| Raspberry Pi Zero 2W | 1 | Run Mosquitto and Node-RED as Docker containers | Yes |
| Separate laptop | 1 | Development and optional future InfluxDB/Grafana hosting | Supporting |

## Component selection

### ESP32-S3

The ESP32-S3 was selected because it provides built-in Wi-Fi and is widely supported and documented.

Its responsibilities are:

- Read both SHT31 sensors over I²C.
- Read the DS18B20 over 1-Wire.
- Perform initial sensor-error checks.
- Create the raw JSON payload.
- Publish measurements to Mosquitto every 30 seconds.
- Reconnect automatically after Wi-Fi or MQTT interruption.

The exact ESP32-S3 development-board model and usable GPIO pins must be confirmed before the wiring diagram is finalised.

### SHT31

Two SHT31 modules are used:

- Internal SHT31: internal air temperature and relative humidity.
- External SHT31: external air temperature.

Both sensors use I²C. They must use different I²C addresses if connected to the same bus. The selected modules must therefore support configuration of addresses `0x44` and `0x45`.

The external sensor requires protection from rain and direct sunlight while remaining exposed to airflow.

### DS18B20

A waterproof DS18B20 probe is used for the water or nutrient-solution temperature.

The sensor communicates using 1-Wire and requires a pull-up resistor between its data and supply lines. The resistor value and wiring must be confirmed using the selected probe’s documentation before assembly.

### Raspberry Pi Zero 2W

The Raspberry Pi Zero 2W is the Docker host for the MVP.

It runs:

- Mosquitto
- Node-RED

InfluxDB and Grafana are excluded from the Raspberry Pi deployment because of its limited resources.

## Supporting hardware

| Component | Purpose |
| --------- | ------- |
| Breadboard | Temporary prototype assembly |
| Jumper wires | Connect sensors to the ESP32-S3 |
| 4.7 kΩ resistor | Pull-up resistor for the DS18B20 data line |
| USB data cable | Program and power the ESP32-S3 |
| Raspberry Pi power supply | Provide stable power to the Docker host |
| MicroSD card | Store the Raspberry Pi operating system and Docker data |
| External-sensor enclosure | Protect the external SHT31 from rain and direct sunlight |

## Hardware checks before wiring

The following details remain unconfirmed because the sensors have not arrived:

- Exact ESP32-S3 development-board model
- Available and safe GPIO pins
- Exact SHT31 breakout-board model
- Whether each SHT31 module exposes address selection
- Default I²C address of each SHT31
- Exact DS18B20 probe wiring and supply-voltage requirements
- Whether the SHT31 modules already contain I²C pull-up resistors
- Power requirements for the complete prototype

These checks must be completed before the final wiring diagram and pin-assignment table are approved.

## Current hardware assumptions

- Both SHT31 sensors can share one I²C bus using separate addresses.
- The DS18B20 uses a separate 1-Wire GPIO.
- All sensor signal levels are compatible with the ESP32-S3.
- The Raspberry Pi Zero 2W can run Mosquitto and Node-RED simultaneously.

These are working assumptions and must be verified through module documentation and physical testing.

## Pending verification

Exact board models, GPIO assignments, sensor addresses and wiring will be documented after the ordered components arrive and can be physically inspected.