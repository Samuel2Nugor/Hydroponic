# MicroHydros

MicroHydros is an IoT monitoring system for collecting, validating, storing and visualizing hydroponic sensor telemetry.

The current development environment runs on macOS with Docker Compose. Raspberry Pi deployment is documented but deferred while hardware integration is completed.

## Data flow

```text
ESP32-S3
  -> Mosquitto raw MQTT topic
  -> Python telemetry validation service
  -> Mosquitto validated or rejected topics
  -> Telegraf
  -> InfluxDB
  -> Grafana
```

Node-RED subscribes to validated MQTT telemetry in parallel and is used for visual inspection and future automation.

## Core services

| Service | Purpose | Host port |
| --- | --- | --- |
| Mosquitto | Authenticated MQTT broker | `1883` |
| Telemetry service | Validates and separates sensor measurements | None |
| Node-RED | Visual MQTT inspection and future automation | `1880` |
| Telegraf | Transfers validated MQTT telemetry to InfluxDB | None |
| InfluxDB | Time-series telemetry storage | `8086` |
| Grafana | Dashboards and telemetry visualization | `3000` |

## Current measurements

- Internal temperature
- Internal relative humidity
- External temperature
- Water temperature

A failed sensor reading is rejected independently. Other valid readings from the same raw message continue through the pipeline.

## Quick start

Docker Desktop must be running.

Create the local environment file:

```bash
cp .env.example .env
```

Replace every password and token placeholder in `.env` with local values. Create the required Mosquitto users by following [docker/README.md](docker/README.md).

Validate the Compose configuration without printing resolved secrets:

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

## Development interfaces

- Node-RED: [http://localhost:1880](http://localhost:1880)
- InfluxDB: [http://localhost:8086](http://localhost:8086)
- Grafana: [http://localhost:3000](http://localhost:3000)

The provisioned Grafana dashboard is named **MicroHydros Telemetry**.

## Tests

Create and activate a Python virtual environment, install the project development dependencies, and run:

```bash
python -m pytest
```

## Documentation

- [Data contract](docs/data-contract.md)
- [System architecture](docs/system-architecture.md)
- [Hardware selection](docs/hardware-selection.md)
- [Decision log](docs/decision-log.md)
- [Docker development environment](docker/README.md)
- [Raspberry Pi deployment](docs/raspberry-pi-deployment.md)

## Security scope

The local development environment uses MQTT username/password authentication. Secrets are stored in `.env` and the local Mosquitto password file; neither file may be committed.

TLS, MQTT topic ACLs and separate least-privilege InfluxDB tokens are not yet configured. Do not expose the development services directly to the internet.

## Stop the environment

Stop containers without deleting stored data:

```bash
docker compose down
```

Do not add `--volumes` unless the stored development data is intentionally being deleted.
