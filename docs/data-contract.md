# Data contract

## Purpose and scope

This document defines the MicroHydros MQTT topics, JSON payloads, field names, data types, units and rejection codes.

Firmware produces raw telemetry according to this contract. The Python telemetry service validates it and publishes accepted measurements or diagnostic rejections. Downstream consumers use the validated format.

Messages that do not satisfy the validation rules must not be published to validated telemetry topics.

Deployment instructions, certificate paths and account configuration are documented in the [Docker README](../docker/README.md).

## Contract status

The current contract version is `v1`, represented by `"schema_version": 1` in payloads.

Raw, validated and rejected telemetry formats are implemented by the Python telemetry service. Device availability messages are defined here but have not yet been verified with the physical device.

The current ESP32-S3 firmware publishes one synthetic payload per boot. Physical sensor reads and periodic publishing remain pending.

The latest verified synthetic payload produced three validated measurements and one external-temperature rejection. The rejection remains under investigation and does not establish a physical sensor failure.

## General payload rules

These rules apply throughout the contract:

- Payloads use UTF-8 JSON.
- Field names are case-sensitive and use `snake_case`.
- Numeric fields contain JSON numbers, not numeric strings.
- Boolean values are not accepted as numeric measurements or integer metadata.
- Numeric values must be finite; `NaN` and infinity are prohibited.
- Required fields must be present.
- `null` represents an unavailable raw measurement, not a valid measured value.
- Passwords, Wi-Fi credentials, tokens and private keys must never appear in payloads.

The distinction between an integer and a measurement number matters. For example, `sequence` is an integer, while a temperature may be expressed as either `20` or `20.7`.

## MQTT topics

| Topic | Publisher | Consumer | Purpose |
| ----- | --------- | -------- | ------- |
| `microhydros/v1/devices/{device_id}/telemetry/raw` | ESP32-S3 | Python telemetry service | Combined measurements and sensor statuses |
| `microhydros/v1/devices/{device_id}/telemetry/validated/{measurement}` | Python telemetry service | Node-RED and Telegraf | One accepted measurement |
| `microhydros/v1/devices/{device_id}/telemetry/rejected` | Python telemetry service | Authorized diagnostic consumers | Message-level or measurement-level rejection |
| `microhydros/v1/devices/{device_id}/status` | ESP32-S3 or broker Last Will | Node-RED | Device availability |

Braces identify placeholders and are not part of an actual topic.

The examples use `esp32s3-01` as the device identifier.

### Subscription filters

The telemetry service receives raw messages using:

```text
microhydros/v1/devices/+/telemetry/raw
```

Node-RED and Telegraf receive accepted measurements using:

```text
microhydros/v1/devices/+/telemetry/validated/+
```

Node-RED can receive availability messages using:

```text
microhydros/v1/devices/+/status
```

Each `+` matches one topic level. In the validated filter, the first matches the device identifier and the second matches the measurement name.

### Transport and authorization

MQTT connections use TLS on port `8883`, broker certificate verification and account authentication. Topic ACLs determine which accounts may publish or subscribe.

Transport security does not replace payload validation.

The current ACL allows the telemetry service to publish rejections, but no application account has permission to read them. A diagnostic consumer requires an explicit read permission.

## Raw telemetry

A raw message represents one measurement cycle. It combines readings with the status of the sensors that produced them.

### Publication settings

| Property | Value |
| -------- | ----- |
| Topic | `microhydros/v1/devices/{device_id}/telemetry/raw` |
| QoS | `1` |
| Retained | `false` |
| Target interval | 30 seconds |

The interval is a firmware target, not the behavior of the current one-message-per-boot implementation.

### Example payload

```json
{
  "schema_version": 1,
  "device_id": "esp32s3-01",
  "boot_id": "a3f82c10",
  "sequence": 42,
  "uptime_ms": 1265430,
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

| Field | Type | Required | Meaning |
| ----- | ---- | -------- | ------- |
| `schema_version` | Integer | Yes | Contract version; currently `1` |
| `device_id` | String | Yes | Identifier of the publishing device |
| `boot_id` | String | Yes | Identifier generated for the current device boot |
| `sequence` | Integer | Yes | Measurement-cycle number within that boot |
| `uptime_ms` | Integer | Yes | Milliseconds since the device started |
| `measurements` | Object | Yes | Combined measurement values |
| `sensor_status` | Object | Yes | Status of each sensor |

### Device and cycle identity

The payload `device_id` must match the identifier in the MQTT topic.

For example, a payload containing `"device_id": "esp32s3-01"` belongs on:

```text
microhydros/v1/devices/esp32s3-01/telemetry/raw
```

The remaining identity rules are:

- Generate a new `boot_id` when the device starts.
- Keep that `boot_id` for the entire boot session.
- Start `sequence` at `0`.
- Increase `sequence` by one for each new measurement cycle.
- Reset `sequence` only when a new boot session starts.
- Use non-negative integers for `sequence` and `uptime_ms`.

A retransmission of an existing cycle retains the same identity. It must not be presented as a new measurement cycle.

The combination `(device_id, boot_id, sequence)` identifies a cycle. A sequence number alone is insufficient because it repeats after a restart.

### Measurement fields

| Field | Type | Unit | Required |
| ----- | ---- | ---- | -------- |
| `internal_temperature_c` | Number or `null` | Degrees Celsius | Yes |
| `internal_humidity_percent` | Number or `null` | Percent relative humidity | Yes |
| `external_temperature_c` | Number or `null` | Degrees Celsius | Yes |
| `water_temperature_c` | Number or `null` | Degrees Celsius | Yes |

All four fields must be present. If a reading is unavailable, publish `null` and the corresponding sensor status.

Do not substitute zero for an unavailable value. Zero may be a legitimate measurement and would hide the failure.

### Sensor-status mapping

| Status field | Affected measurements |
| ------------ | --------------------- |
| `internal_sht31` | Internal temperature and internal humidity |
| `external_ds18b20` | External temperature |
| `water_ds18b20` | Water temperature |

The internal SHT31 produces two measurements, so its status applies to both.

The external-temperature status key is `external_ds18b20`. The older name `external_sht31` is not part of this contract.

### Supported sensor statuses

| Status | Meaning |
| ------ | ------- |
| `ok` | Sensor returned a reading for validation |
| `read_error` | Sensor was detected, but the read failed |
| `not_detected` | Sensor could not be detected |
| `invalid_value` | Sensor returned an unusable value |

If a sensor status is not `ok`, every affected measurement must be `null`.

An `ok` status does not guarantee acceptance. The backend still checks the value's type, finiteness and plausibility.

### Example with an unavailable sensor

```json
{
  "schema_version": 1,
  "device_id": "esp32s3-01",
  "boot_id": "a3f82c10",
  "sequence": 43,
  "uptime_ms": 1295430,
  "measurements": {
    "internal_temperature_c": 23.6,
    "internal_humidity_percent": 61.4,
    "external_temperature_c": null,
    "water_temperature_c": 20.7
  },
  "sensor_status": {
    "internal_sht31": "ok",
    "external_ds18b20": "not_detected",
    "water_ds18b20": "ok"
  }
}
```

This payload allows three valid measurements to continue. External temperature is rejected with `sensor_not_detected`.

### Plausibility ranges

| Measurement | Minimum | Maximum |
| ----------- | ------- | ------- |
| `internal_temperature_c` | `-10.0` | `60.0` |
| `internal_humidity_percent` | `0.0` | `100.0` |
| `external_temperature_c` | `-40.0` | `60.0` |
| `water_temperature_c` | `0.0` | `50.0` |

A value outside its range is rejected with `out_of_plausible_range`.

These limits identify technically implausible values. They are not recommended growing conditions, alarm thresholds or guarantees of sensor accuracy.

Changes to the ranges must remain consistent between this contract, the validator and its tests.

## Validation behavior

Validation has two scopes:

1. Common metadata determines whether the raw message can be processed.
2. Individual measurement checks determine which readings can continue.

Invalid common metadata rejects the complete message. Once common metadata passes, each measurement is checked independently.

Examples:

| Condition | Result |
| --------- | ------ |
| Four valid measurements | Four validated messages |
| External sensor reports `not_detected` | Three validated messages and one rejection |
| Internal SHT31 reports `read_error` | Internal temperature and humidity rejected; other valid readings continue |
| One value is outside its range | Only that measurement is rejected |
| Payload device identifier differs from the topic | Complete message rejected |

Independent validation prevents one sensor failure from discarding useful readings from other sensors.

## Validated telemetry

Each validated message contains one accepted measurement and the metadata needed to associate it with its source cycle.

### Publication settings

| Property | Value |
| -------- | ----- |
| Topic | `microhydros/v1/devices/{device_id}/telemetry/validated/{measurement}` |
| QoS | `1` |
| Retained | `false` |

### Measurement mapping

| Measurement name | Raw source field | Sensor identifier | Unit |
| ---------------- | ---------------- | ----------------- | ---- |
| `internal_temperature` | `internal_temperature_c` | `internal_sht31` | `celsius` |
| `internal_humidity` | `internal_humidity_percent` | `internal_sht31` | `percent_rh` |
| `external_temperature` | `external_temperature_c` | `external_ds18b20` | `celsius` |
| `water_temperature` | `water_temperature_c` | `water_ds18b20` | `celsius` |

The measurement name appears in both the topic suffix and the payload. These values must agree.

### Example

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
  "uptime_ms": 1265430,
  "timestamp": "2026-09-08T10:15:30Z",
  "measurement": "water_temperature",
  "value": 20.7,
  "unit": "celsius",
  "sensor_id": "water_ds18b20"
}
```

### Fields

| Field | Type | Required | Meaning |
| ----- | ---- | -------- | ------- |
| `schema_version` | Integer | Yes | Contract version |
| `device_id` | String | Yes | Source-device identifier |
| `boot_id` | String | Yes | Source boot session |
| `sequence` | Integer | Yes | Cycle number from the raw message |
| `uptime_ms` | Integer | Yes | Uptime from the raw message |
| `timestamp` | String | Yes | Backend-assigned UTC timestamp |
| `measurement` | String | Yes | Measurement name from the mapping table |
| `value` | Number | Yes | Accepted finite numeric value |
| `unit` | String | Yes | Unit from the mapping table |
| `sensor_id` | String | Yes | Sensor identifier from the mapping table |

Validated values cannot be `null`.

### Timestamp meaning

The Python telemetry service assigns a UTC timestamp when processing the raw message.

All accepted measurements from that message share the same timestamp and source-cycle metadata.

The timestamp uses ISO 8601 with a UTC indicator, for example:

```text
2026-09-08T10:15:30Z
```

It represents backend processing time, not an independently verified physical sampling time. `uptime_ms` provides device-relative timing but is not a wall-clock timestamp.

Synchronizing the device clock for TLS does not change this timestamp authority.

## Rejected telemetry

Rejected messages explain why a raw message or individual measurement was not accepted.

### Publication settings

| Property | Value |
| -------- | ----- |
| Topic | `microhydros/v1/devices/{device_id}/telemetry/rejected` |
| QoS | `1` |
| Retained | `false` |

Rejected telemetry is diagnostic information. It must not be written into the normal validated measurement series.

### Message-level rejection

A `message` rejection means that the complete raw message cannot be trusted.

Typical causes include:

- Empty or invalid JSON payload
- Unsupported schema version
- Missing or invalid common metadata
- Device identifier mismatch

No validated measurements are published from a message rejected at this scope.

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

Boot and sequence metadata may be unavailable or untrustworthy at this stage, so they are not required for a message-level rejection.

### Measurement-level rejection

A `measurement` rejection applies to one reading whose field, value or associated sensor status failed validation.

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

Descriptions in examples illustrate the failure. Consumers should use `reason_code` for programmatic decisions rather than matching description text.

### Rejection fields

| Field | Type | Required | Meaning |
| ----- | ---- | -------- | ------- |
| `schema_version` | Integer | Always | Rejection-format version |
| `device_id` | String | Always | Device identifier extracted from the topic |
| `boot_id` | String | Measurement rejection | Source boot session |
| `sequence` | Integer | Measurement rejection | Source cycle number |
| `timestamp` | String | Always | Backend-assigned UTC rejection time |
| `rejection_scope` | String | Always | `message` or `measurement` |
| `measurement` | String | Measurement rejection | Failed measurement name |
| `sensor_id` | String | Measurement rejection | Associated sensor identifier |
| `received_value` | Any JSON type | Measurement rejection | Value associated with the failed reading |
| `reason_code` | String | Always | Machine-readable rejection reason |
| `description` | String | Always | Human-readable explanation |

`received_value` has a broader type than a valid measurement because diagnostics may need to represent an incorrect string, Boolean, object or `null`. It must still satisfy the general JSON rules.

### Reason codes

| Reason code | Scope | Meaning |
| ----------- | ----- | ------- |
| `empty_payload` | Message | MQTT payload was empty |
| `invalid_json` | Message | Payload could not be parsed as valid JSON |
| `unsupported_schema` | Message | Schema version is unsupported |
| `device_id_mismatch` | Message | Payload and topic identifiers differ |
| `missing_metadata` | Message | Required common metadata is missing |
| `invalid_metadata` | Message | Common metadata has an invalid type or value |
| `missing_measurement` | Measurement | Required measurement field is missing |
| `missing_sensor_status` | Measurement | Required sensor-status field is missing |
| `invalid_sensor_status` | Measurement | Sensor status is unsupported |
| `invalid_type` | Measurement | Measurement is not an accepted numeric type |
| `sensor_read_error` | Measurement | Sensor read failed |
| `sensor_not_detected` | Measurement | Sensor could not be detected |
| `invalid_value` | Measurement | Sensor returned an unusable value |
| `out_of_plausible_range` | Measurement | Reading is outside its permitted range |

A rejection reports the validation failure encountered. It should not be interpreted as an exhaustive list of every possible defect in the payload.

## Delivery and duplicate handling

Raw, validated and rejected telemetry use QoS 1. Duplicate delivery is therefore possible.

The telemetry service identifies previously processed raw cycles using:

```text
(device_id, boot_id, sequence)
```

Its cache stores up to 4,096 recent identities in memory. The cache is cleared when the service restarts, and older identities can be evicted.

This limits duplicate processing within the cache window. It does not provide persistent deduplication or end-to-end exactly-once delivery.

Sequence-gap detection is not implemented. A valid sequence value does not prove that every earlier cycle reached the backend.

## Device availability

This section defines the intended availability behavior. It has not yet been verified with the physical ESP32-S3.

### Publication settings

| Property | Value |
| -------- | ----- |
| Topic | `microhydros/v1/devices/{device_id}/status` |
| QoS | `1` |
| Retained | `true` |

Availability is retained so that a new subscriber can receive the most recently published status. Telemetry messages are not retained.

### Online payload

After establishing its MQTT connection, the device publishes:

```json
{
  "schema_version": 1,
  "device_id": "esp32s3-01",
  "boot_id": "a3f82c10",
  "status": "online"
}
```

### Offline payload

Before connecting, the device configures this retained Last Will payload:

```json
{
  "schema_version": 1,
  "device_id": "esp32s3-01",
  "boot_id": "a3f82c10",
  "status": "offline"
}
```

The broker publishes the Last Will when it detects an unexpected connection loss. Detection is not necessarily immediate.

For a planned shutdown, the device should publish `offline` before disconnecting normally.

### Fields

| Field | Type | Required | Meaning |
| ----- | ---- | -------- | ------- |
| `schema_version` | Integer | Yes | Contract version |
| `device_id` | String | Yes | Device identifier matching the topic |
| `boot_id` | String | Yes | Boot session associated with the connection |
| `status` | String | Yes | `online` or `offline` |

Availability indicates MQTT connection state. An `online` message does not prove that sensors are healthy or that new readings are reaching storage.

## Contract maintenance

Changes to field names, sensor mappings, units, validation rules or payload structure must be reviewed together with the producer, validator, consumers and tests.

Do not weaken validation solely to make a rejected payload pass. First determine whether the producer violates the contract or whether the contract itself needs an intentional change.

Breaking changes require an explicit versioning decision. Keep the topic version and `schema_version` consistent with the format being published.