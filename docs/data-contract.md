# Data Contract

## Status

Contract version `v1` is implemented by the Python telemetry validator and verified using automated and end-to-end tests.

The raw, validated and rejected telemetry formats are implemented. The device-status format is defined for ESP32-S3 firmware integration but has not yet been verified with the physical device.

## Purpose

This document is the source of truth for MicroHydros MQTT topics, JSON payloads, field names, data types, units and rejection codes.

Messages that do not follow this contract must not be published to validated telemetry topics.

## MQTT topics

| Topic | Publisher | Subscriber | Purpose |
| ----- | --------- | ---------- | ------- |
| `microhydros/v1/devices/{device_id}/telemetry/raw` | ESP32-S3 | Python telemetry service | Combined sensor readings and statuses |
| `microhydros/v1/devices/{device_id}/telemetry/validated/{measurement}` | Python telemetry service | Node-RED and Telegraf | One independently validated measurement |
| `microhydros/v1/devices/{device_id}/telemetry/rejected` | Python telemetry service | Diagnostic consumers | Message-level or measurement-level rejection |
| `microhydros/v1/devices/{device_id}/status` | ESP32-S3 or Mosquitto LWT | Node-RED | Device availability |

The first prototype device identifier is:

```text
esp32s3-01
```

The Python telemetry service subscribes to every compatible device using:

```text
microhydros/v1/devices/+/telemetry/raw
```

Node-RED and Telegraf subscribe to validated measurements using:

```text
microhydros/v1/devices/+/telemetry/validated/+
```

The first `+` represents one device identifier. The second represents one measurement name.

## Raw telemetry

### MQTT settings

| Property | Value |
| -------- | ----- |
| Topic | `microhydros/v1/devices/{device_id}/telemetry/raw` |
| QoS | `1` |
| Retained | `false` |
| Payload | UTF-8 JSON |
| Target publishing interval | 30 seconds |

The ESP32-S3 publishes one combined message for each measurement cycle.

### Example

```json
{
  "schema_version": 1,
  "device_id": "esp32s3-01",
  "boot_id": "a3f82c10",
  "sequence": 42,
  "uptime_ms": 185430,
  "measurements": {
    "internal_temperature_c": 23.6,
    "internal_humidity_percent": 61.4,
    "external_temperature_c": 18.9,
    "water_temperature_c": 20.7
  },
  "sensor_status": {
    "internal_sht31": "ok",
    "external_ds18b20": "ok",
    "water_ds18b20": "ok"
  }
}
```

### Common fields

| Field | Type | Required | Description |
| ----- | ---- | -------- | ----------- |
| `schema_version` | Integer | Yes | Data-contract version |
| `device_id` | String | Yes | Publishing-device identifier |
| `boot_id` | String | Yes | Identifier generated when the device starts |
| `sequence` | Integer | Yes | Measurement-cycle number during the current boot |
| `uptime_ms` | Integer | Yes | Milliseconds since the device started |
| `measurements` | Object | Yes | Sensor measurement fields |
| `sensor_status` | Object | Yes | Status of each physical sensor |

### Measurement fields

| Field | Type | Unit | Required |
| ----- | ---- | ---- | -------- |
| `internal_temperature_c` | Number or `null` | Degrees Celsius | Yes |
| `internal_humidity_percent` | Number or `null` | Percent relative humidity | Yes |
| `external_temperature_c` | Number or `null` | Degrees Celsius | Yes |
| `water_temperature_c` | Number or `null` | Degrees Celsius | Yes |

All four fields must be present. A failed measurement uses `null`; its field must not be omitted.

### Sensor-status fields

| Field | Affected measurement |
| ----- | -------------------- |
| `internal_sht31` | Internal temperature and internal humidity |
| `external_ds18b20` | External temperature |
| `water_ds18b20` | Water temperature |

Supported status values are:

| Value | Meaning |
| ----- | ------- |
| `ok` | Sensor returned a usable reading |
| `read_error` | Sensor was detected but the read failed |
| `not_detected` | Sensor could not be detected |
| `invalid_value` | Sensor returned an unusable value |

If a sensor status is not `ok`, every affected measurement must be `null`.

### Plausibility ranges

These limits identify technically implausible readings. They are deliberately broader than growing-condition alarm thresholds.

| Measurement | Minimum | Maximum |
| ----------- | ------- | ------- |
| `internal_temperature_c` | `-10.0` | `60.0` |
| `internal_humidity_percent` | `0.0` | `100.0` |
| `external_temperature_c` | `-40.0` | `60.0` |
| `water_temperature_c` | `0.0` | `50.0` |

A value outside its range is rejected with `out_of_plausible_range`. These ranges may be revised after physical testing and calibration.

### Identifier rules

- The payload `device_id` must match the MQTT topic.
- `boot_id` is generated once when the ESP32-S3 starts.
- `sequence` starts at `0`.
- `sequence` increases by one for every raw message.
- `sequence` resets only when the device restarts.
- `uptime_ms` must be a non-negative integer.
- A new device boot must generate a new `boot_id`.

The validator currently carries `boot_id` and `sequence` through the pipeline but does not detect duplicates or sequence gaps.

## Validated telemetry

The Python telemetry service validates every measurement independently. One failed measurement does not block other valid measurements from the same raw message.

### MQTT settings_

| Property | Value |
| -------- | ----- |
| Topic | `microhydros/v1/devices/{device_id}/telemetry/validated/{measurement}` |
| QoS | `1` |
| Retained | `false` |
| Payload | UTF-8 JSON |

### Measurement mapping

| Measurement | Source field | Sensor | Unit |
| ---- -------| ------------ | ------ | ---- |
| `internal_temperature` | `internal_temperature_c` | `internal_sht31` | `celsius` |
| `internal_humidity` | `internal_humidity_percent` | `internal_sht31` | `percent_rh` |
| `external_temperature` | `external_temperature_c` | `external_ds18b20` | `celsius` |
| `water_temperature` | `water_temperature_c` | `water_ds18b20` | `celsius` |

### Examples

Topic:

```text
microhydros/v1/devices/esp32s3-01/telemetry/validated/water_temperature
```

Payload:

```json
{
  "schema_version": 1,
  "device_id": "esp32s3-01",
  "boot_id": "a3f82c10",
  "sequence": 42,
  "uptime_ms": 185430,
  "timestamp": "2026-09-08T10:15:30Z",
  "measurement": "water_temperature",
  "value": 20.7,
  "unit": "celsius",
  "sensor_id": "water_ds18b20"
}
```

### Fields

| Field | Type | Required | Description |
| ----- | ---- | -------- | ----------- |
| `schema_version` | Integer | Yes | Supported contract version |
| `device_id` | String | Yes | Source-device identifier |
| `boot_id` | String | Yes | Device boot session |
| `sequence` | Integer | Yes | Sequence from the raw message |
| `uptime_ms` | Integer | Yes | Uptime from the raw message |
| `timestamp` | String | Yes | UTC reception time assigned by the Python service |
| `measurement` | String | Yes | Validated measurement name |
| `value` | Number | Yes | Validated finite value |
| `unit` | String | Yes | Measurement unit |
| `sensor_id` | String | Yes | Physical sensor identifier |

All valid measurements from the same raw message receive the same `timestamp`, `boot_id`, `sequence` and `uptime_ms`.

`null`, `NaN`, positive infinity and negative infinity must never appear on validated topics.

## Rejected telemetry

The Python telemetry service publishes failures to:

```text
microhydros/v1/devices/{device_id}/telemetry/rejected
```

| Property | Value |
| -------- | ----- |
| QoS | `1` |
| Retained | `false` |
| Payload | UTF-8 JSON |

There are two rejection scopes:

- `message`: the complete raw message cannot be trusted.
- `measurement`: one measurement failed, but other valid measurements may continue.

### Message-level rejection

A complete message is rejected for failures such as:

- Empty payload
- Invalid JSON
- Unsupported schema version
- Missing or invalid common metadata
- Missing or invalid device identifier
- Device identifier not matching the MQTT topic
- Missing or invalid boot identifier
- Missing or invalid sequence
- Missing or invalid uptime

When a message-level rejection occurs, none of its measurements may be published to validated topics.

Example:

```json
{
  "schema_version": 1,
  "device_id": "esp32s3-01",
  "timestamp": "2026-09-08T10:15:30Z",
  "rejection_scope": "message",
  "reason_code": "invalid_json",
  "description": "The raw MQTT payload could not be parsed as JSON"
}
```

### Measurement-level rejection

A measurement is rejected when its field, value or corresponding sensor status is invalid.

Example:

```json
{
  "schema_version": 1,
  "device_id": "esp32s3-01",
  "boot_id": "a3f82c10",
  "sequence": 43,
  "timestamp": "2026-09-08T10:16:00Z",
  "rejection_scope": "measurement",
  "measurement": "external_temperature",
  "sensor_id": "external_ds18b20",
  "received_value": null,
  "reason_code": "sensor_not_detected",
  "description": "The external DS18B20 sensor could not be detected"
}
```

### Rejected fields

| Field | Type | Required | Description |
| ----- | ---- | -------- | ----------- |
| `schema_version` | Integer | Yes | Rejected-message contract version |
| `device_id` | String | Yes | Device identifier extracted from the topic |
| `boot_id` | String | Measurement rejection | Source boot session |
| `sequence` | Integer | Measurement rejection | Source sequence number |
| `timestamp` | String | Yes | UTC rejection time |
| `rejection_scope` | String | Yes | `message` or `measurement` |
| `measurement` | String | Measurement rejection | Failed measurement |
| `sensor_id` | String | Measurement rejection | Associated sensor |
| `received_value` | Any JSON type | Measurement rejection | Received value |
| `reason_code` | String | Yes | Machine-readable reason |
| `description` | String | Yes | Human-readable explanation |

### Reason codes

| Reason code | Scope | Meaning |
| ----------- | ----- | ------- |
| `empty_payload` | Message | MQTT payload was empty |
| `invalid_json` | Message | Payload was not valid JSON |
| `unsupported_schema` | Message | Schema version is unsupported |
| `device_id_mismatch` | Message | Payload and topic device identifiers differ |
| `missing_metadata` | Message | Required common metadata is missing |
| `invalid_metadata` | Message | Common metadata has an invalid type or value |
| `missing_measurement` | Measurement | Required measurement field is missing |
| `missing_sensor_status` | Measurement | Required sensor-status field is missing |
| `invalid_sensor_status` | Measurement | Sensor status is unsupported |
| `invalid_type` | Measurement | Measurement is not numeric |
| `sensor_read_error` | Measurement | Sensor read failed |
| `sensor_not_detected` | Measurement | Sensor could not be detected |
| `invalid_value` | Measurement | Sensor returned an unusable value |
| `out_of_plausible_range` | Measurement | Value is outside its plausibility range |

Rejected telemetry is diagnostic information and must not be written into normal InfluxDB measurement series.

## Device status

This contract is defined for the upcoming ESP32-S3 firmware integration and has not yet been verified with the physical device.

### MQTT settings__

| Property | Value |
| -------- | ----- |
| Topic | `microhydros/v1/devices/{device_id}/status` |
| QoS | `1` |
| Retained | `true` |
| Payload | UTF-8 JSON |

Node-RED can subscribe to:

```text
microhydros/v1/devices/+/status
```

### Online payload

After connecting successfully, the ESP32-S3 publishes:

```json
{
  "schema_version": 1,
  "device_id": "esp32s3-01",
  "boot_id": "a3f82c10",
  "status": "online"
}
```

### Offline payload

Before connecting, the ESP32-S3 configures this MQTT Last Will and Testament payload:

```json
{
  "schema_version": 1,
  "device_id": "esp32s3-01",
  "boot_id": "a3f82c10",
  "status": "offline"
}
```

If the device unexpectedly loses power, Wi-Fi or MQTT connectivity, Mosquitto publishes the retained offline message.

### Status rules

- `status` must be either `online` or `offline`.
- The payload device identifier must match the MQTT topic.
- The ESP32-S3 configures its Last Will before connecting.
- The ESP32-S3 publishes `online` only after connecting successfully.
- A planned shutdown should publish `offline` before disconnecting.
- Status messages are retained.
- Telemetry messages are not retained.
- Status payloads must not contain credentials or secrets.

## General rules

- Payloads use UTF-8 JSON.
- Field names are case-sensitive.
- Field names use `snake_case`.
- Measurements are JSON numbers, not strings.
- Boolean values are not accepted as numeric measurements.
- `NaN` and infinity must never be published.
- Invalid common metadata rejects the complete message.
- A sensor error rejects only its affected measurements.
- The current contract version is `v1`.
- Passwords, Wi-Fi credentials and tokens must never appear in MQTT payloads.
