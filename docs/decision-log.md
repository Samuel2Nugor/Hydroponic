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

The architecture and data-contract documents have been updated. The Python telemetry service and its Docker container have not yet been implemented.
