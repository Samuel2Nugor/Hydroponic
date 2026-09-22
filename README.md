# MicroHydros

MicroHydros is an IoT monitoring system for collecting, validating, storing and visualizing hydroponic sensor telemetry.

The development backend runs on macOS with Docker Compose. ESP32-S3 firmware is developed using ESP-IDF on Linux Mint. Raspberry Pi deployment is deferred.

## Current status

The physical ESP32-S3 connects over Wi-Fi and publishes synthetic telemetry using MQTT over TLS.

Verified:

- Wi-Fi and MQTT connectivity on home and school networks.
- Broker discovery using a stable mDNS hostname (`<broker-hostname>.local`).
- Network time synchronization before the TLS connection.
- Broker certificate verification using an embedded CA certificate.
- MQTT username/password authentication and topic ACLs.
- TLS connections for the ESP32-S3, telemetry service, Telegraf and Node-RED.
- Validated readings reaching InfluxDB and Node-RED.
- Broker healthcheck using TLS.
- Removal of the plaintext MQTT listener and host port mapping on `1883`.

The current firmware publishes one synthetic payload per boot. Physical sensor reads and periodic publishing remain pending.

The tested payload produces three validated readings and one external-temperature rejection. The rejection cause is still under investigation.

## Data flow

1. The ESP32-S3 publishes a combined raw telemetry message to Mosquitto.
2. The Python telemetry service validates each measurement independently and assigns UTC timestamps.
3. Valid measurements and rejection information are published to separate MQTT topics.
4. Telegraf consumes validated measurements and writes them to InfluxDB.
5. Grafana queries InfluxDB for historical visualization.
6. Node-RED consumes validated measurements in parallel for visual inspection and future automation.

Rejected readings are not written into normal InfluxDB measurement series.

## Core services

| Service | Purpose | Host port |
| ------- | ------- | --------- |
| Mosquitto | TLS MQTT broker with authentication and topic ACLs | `8883` |
| Telemetry service | Validates, timestamps and separates measurements | None |
| Node-RED | Visual MQTT inspection and future automation | `1880` |
| Telegraf | Transfers validated MQTT telemetry to InfluxDB | None |
| InfluxDB | Time-series telemetry storage | `8086` |
| Grafana | Dashboards and telemetry visualization | `3000` |

## Measurement contract

| Measurement | Intended sensor |
| ----------- | --------------- |
| Internal temperature | Internal SHT31 |
| Internal relative humidity | Internal SHT31 |
| External temperature | External DS18B20 |
| Water temperature | Waterproof DS18B20 |

A failed sensor reading is rejected independently. Other valid readings from the same raw message continue through the pipeline.

Exact field names, sensor identifiers, units and rejection formats are defined in [docs/data-contract.md](docs/data-contract.md).

## Quick start

Docker Desktop must be running. Execute commands from the repository root.

### 1. Configure the environment

Create the private environment file if it does not already exist:

```bash
cp .env.example .env
```

Replace password and token placeholders with local values.

Set the telemetry service's MQTT connection settings:

```dotenv
MQTT_HOST=mosquitto
MQTT_PORT=8883
MQTT_CA_CERT=/etc/microhydros/certs/ca.crt
MQTT_USERNAME=telemetry-service
```

The CA path is inside the container and must match the Compose mount.

### 2. Provision MQTT credentials and certificates

Follow [docker/README.md](docker/README.md) to configure the MQTT accounts and local TLS files.

Required MQTT accounts:

- `esp32s3-01`
- `telemetry-service`
- `node-red`
- `telegraf`

Required local broker files:

- `docker/mosquitto/config/password_file`
- `docker/mosquitto/certs/ca.crt`
- `docker/mosquitto/certs/server.crt`
- `docker/mosquitto/certs/server.key`

Topic permissions are defined in:

```text
docker/mosquitto/config/acl_file
```

Compose does not generate certificates or credentials.

The broker certificate must cover the names used by clients. Its Subject Alternative Names (SANs) must include the broker's stable mDNS hostname, `mosquitto` and `localhost`, plus IP address `127.0.0.1`.

### 3. Validate and start

Validate Compose without printing resolved secrets:

```bash
docker compose config --quiet
```

Build and start the services:

```bash
docker compose up -d --build --wait
```

Check their status:

```bash
docker compose ps
```

## MQTT connection addresses

| Client location | Broker address |
| --------------- | -------------- |
| ESP32-S3 on the local network | `mqtts://<broker-hostname>.local:8883` |
| Backend containers | `mosquitto:8883` with TLS enabled |
| MQTT tools running on the Mac | `localhost:8883` with TLS enabled |

The ESP32-S3 has mDNS queries enabled. A hostname can resolve to a new IP address after changing networks without replacing the certificate, provided the hostname remains valid in the certificate and the network permits mDNS and client communication.

Wi-Fi credentials still need configuring for the new network.

## Development interfaces

- Node-RED: [http://localhost:1880](http://localhost:1880)
- InfluxDB: [http://localhost:8086](http://localhost:8086)
- Grafana: [http://localhost:3000](http://localhost:3000)

The provisioned Grafana dashboard is named **MicroHydros Telemetry**.

The Node-RED flow export is stored at:

```text
docker/node-red/flows/validated-telemetry.json
```

The export contains TLS settings. Configure the local Node-RED MQTT credentials after importing it.

## Verify telemetry

Press RESET/EN on the ESP32-S3 to publish a new synthetic payload.

Inspect the backend logs:

```bash
docker compose logs --since=3m --tail=60 telemetry-service telegraf
```

Check Node-RED's Debug sidebar for validated messages.

Allow about 15 seconds for Telegraf to flush, then query InfluxDB:

```flux
from(bucket: "telemetry")
  |> range(start: -5m)
  |> filter(fn: (r) => r.device_id == "esp32s3-01")
  |> filter(fn: (r) => r._field == "value")
```

Check the timestamp to distinguish fresh results from earlier tests. The current firmware publishes once per boot, so a short query window can legitimately return no data.

## Tests

Create and activate a Python virtual environment, install the project's development dependencies, and run:

```bash
python -m pytest
```

Live TLS and telemetry checks do not replace automated tests. The test suite must be rerun after configuration and code changes.

## Security scope

MQTT uses TLS with broker certificate verification, individual username/password accounts and topic ACLs.

- MQTT listens on `8883`.
- Plaintext MQTT on `1883` is disabled.
- The broker healthcheck also uses TLS.
- Clients authenticate with MQTT passwords; mutual TLS is not configured.
- The healthcheck currently reuses the telemetry-service account.

Keep `.env`, the Mosquitto password file and private keys out of Git. Public CA certificates are not secrets; firmware embeds its public CA certificate.

Remaining limitations:

- Separate least-privilege InfluxDB tokens are not configured.
- Node-RED editor authentication is not configured.
- Web interfaces and the Telegraf-to-InfluxDB connection use HTTP.
- MQTT TLS does not encrypt every connection in the system.

Do not expose these development services directly to the internet.

## Documentation

- [Data contract](docs/data-contract.md)
- [System architecture](docs/system-architecture.md)
- [Hardware selection](docs/hardware-selection.md)
- [Decision log](docs/decision-log.md)
- [Docker development environment](docker/README.md)
- [Raspberry Pi deployment](docs/raspberry-pi-deployment.md)

The Raspberry Pi deployment guide describes an earlier deployment approach and must be checked against the current TLS requirements before reuse.

## Stop the environment

Stop containers without deleting stored data:

```bash
docker compose down
```

Do not add `--volumes` unless you intentionally want to delete stored development data.