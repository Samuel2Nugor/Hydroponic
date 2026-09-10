# Architecture Decision Log

This document records important technical decisions and the reasoning behind them. Decisions may be updated after team review.

## ADR-001: Use Python for telemetry validation

**Date:** 2026-09-09
**Status:** Proposed — awaiting team review

### Context

The ESP32-S3 publishes raw sensor readings to Mosquitto using MQTT. The system needs to validate each measurement independently, add a UTC timestamp, and publish either validated data or rejection information.

Node-RED was initially selected for this responsibility. However, implementing the validation in Node-RED would require JavaScript function nodes and would place important backend logic inside visual flows.

The planned project technologies are C or C++ for the ESP32-S3 firmware and Python for the backend.

### Decision

A dedicated Python telemetry service will perform backend validation and processing.

The service will:

* Subscribe to raw telemetry topics.
* Parse the JSON payload.
* Validate required message metadata.
* Validate every sensor measurement independently.
* Add a UTC timestamp.
* Publish valid measurements separately.
* Publish rejection information for invalid messages or measurements.

Node-RED will subscribe to validated MQTT topics and will be used for displaying measurements and, if included in the MVP, alarms.

### Reasoning

Using Python:

* Matches the team’s planned backend technology.
* Keeps validation logic in normal source files.
* Makes the validation easier to test with automated tests.
* Separates data processing from visualisation.
* Allows Node-RED flows to remain simple.
* Supports replacing or extending Node-RED later without changing the validation service.

### Consequences

* A Python service must be created and added to Docker Compose.
* Mosquitto, the Python service and Node-RED will run as separate containers.
* The Python service needs its own MQTT account and permissions.
* Validation rules must be covered by Python tests.
* The system architecture and data contract must identify the Python service as the timestamp and validation authority.
* Running another container increases Raspberry Pi memory and CPU usage slightly.

### Alternatives considered

#### Validate in Node-RED

Rejected because the main validation logic would depend on JavaScript function nodes and would be harder for the team to test and maintain as ordinary backend code.

#### Validate everything on the ESP32-S3

Rejected because firmware should perform basic sensor checks, but the backend still needs to verify incoming messages before forwarding them to other services.

### Implementation status

The system architecture and data contract identify the Python telemetry service as the validation and timestamp authority.

The Python validator, MQTT client, automated tests and Docker container have been implemented. Local integration testing confirmed that:

* One valid raw message produces four independently validated messages.
* A failed external sensor produces one rejected measurement.
* Valid measurements from the remaining sensors continue through the system.
* All outputs from the same raw message share the same timestamp, `boot_id` and `sequence`.

Deployment and verification on the Raspberry Pi remain pending.

## ADR-002: Use ESP-IDF for ESP32-S3 firmware

**Date:** 2026-09-10
**Status:** Proposed — awaiting team review

### Context002

The ESP32-S3 firmware must connect through Wi-Fi, authenticate with
Mosquitto, publish raw telemetry using MQTT QoS 1, configure a retained
Last Will message and recover from Wi-Fi or MQTT interruptions.

The firmware must follow the existing data contract while keeping
Wi-Fi and MQTT credentials outside Git.

### Decision002

The ESP32-S3 firmware will use ESP-IDF and will initially be developed
and verified with ESP-IDF v6.0.1.

The firmware will be written in C and use:

* ESP-IDF Wi-Fi and event APIs.
* ESP-MQTT for MQTT communication.
* cJSON for contract-compliant JSON payloads.
* CMake through the standard ESP-IDF project structure.

Because ESP-MQTT is a separate component in ESP-IDF 6.x, it will be
added as a managed project dependency.

### Reasoning002

ESP-IDF provides direct support for the required ESP32-S3 networking,
event handling and reconnect behaviour.

ESP-MQTT supports QoS 1 publishing, retained messages, authentication
and MQTT Last Will configuration. These capabilities match the
approved MicroHydros data contract.

Using C also matches the planned embedded technology and avoids adding
an unnecessary C++ abstraction layer to the initial firmware skeleton.

### Consequences002

* Firmware builds and hardware tests require an activated ESP-IDF
  environment.
* The initial build machine is the Linux Mint development laptop.
* ESP-MQTT must be added and locked as a managed dependency.
* Local Wi-Fi and MQTT credentials must not be committed.
* The firmware needs separate modules for Wi-Fi, MQTT, telemetry
  payload construction and sensor reading.
* Physical sensor drivers remain pending until the exact sensors and
  usable GPIO pins are confirmed.

### Alternatives considered002

#### Arduino framework with PubSubClient

Not selected because PubSubClient can only publish at QoS 0, while the
MicroHydros contract requires QoS 1.

Arduino remains technically possible with another MQTT library, but
that would require another library evaluation and would not provide a
clear advantage for the current project.

#### Implement everything in one source file

Rejected because Wi-Fi recovery, MQTT lifecycle handling, telemetry
construction and sensor access are separate responsibilities. However,
the first implementation will remain small and will not introduce
additional abstraction beyond those responsibilities.

### Implementation status002

Planned. The first firmware skeleton will be created after this
decision and the firmware plan have been reviewed.
