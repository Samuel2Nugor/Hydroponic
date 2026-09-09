# Docker development environment

This directory contains the Docker configuration used by the MicroHydros core services.

## Core services

| Service | Purpose | Host port |
| ------- | ------- | --------- |
| Mosquitto | Routes MQTT messages between system components | `1883` |
| Telemetry service | Validates, timestamps and separates sensor measurements | None |
| Node-RED | Displays validated measurements and supports optional alarms | `1880` |

The telemetry service and Node-RED communicate through Mosquitto. Node-RED is not responsible for validating raw telemetry.

## MQTT authentication

Mosquitto does not permit anonymous connections.

Each developer must create a local password file containing these MQTT users:

* `esp32s3-01`
* `node-red`
* `telemetry-service`

The password file is stored at:

```text
docker/mosquitto/config/password_file
```

It contains password hashes and must not be committed to Git.

Confirm that Git ignores it:

```bash
git check-ignore -v docker/mosquitto/config/password_file
```

Do not display or commit the complete password file.

## Python service configuration

Create the private environment file from the provided example:

```bash
cp .env.example .env
```

Open `.env` and replace the example MQTT password with the password created for the `telemetry-service` Mosquitto user.

The `.env` file contains credentials and must not be committed.

Confirm that Git ignores it:

```bash
git check-ignore -v .env
```

## Validate the configuration

Validate the Compose configuration without displaying resolved environment values:

```bash
docker compose config --quiet
```

Do not share the output of plain `docker compose config` because resolved environment values may include credentials.

## Build the Python service

```bash
docker compose build telemetry-service
```

## Start the services

Docker Desktop or Docker Engine must be running.

From the repository root, run:

```bash
docker compose up -d
```

Check service status:

```bash
docker compose ps
```

View recent logs:

```bash
docker compose logs --tail=50 mosquitto telemetry-service node-red
```

## Development addresses

Node-RED is available at:

```text
http://localhost:1880
```

MQTT clients running directly on the development computer connect to:

```text
localhost:1883
```

Containers on the Compose network connect to Mosquitto using:

```text
mosquitto:1883
```

The ESP32-S3 will connect using the Raspberry Pi’s local network address after deployment.

## Data flow

The core data flow is:

```text
ESP32-S3
  -> Mosquitto raw topic
  -> Python telemetry service
  -> Mosquitto validated or rejected topics
  -> Node-RED
```

A failed sensor measurement is rejected independently. Other valid measurements from the same raw message continue through the system.

## Stop the services

Stop and remove the containers and network without deleting stored data:

```bash
docker compose down
```

Do not add `--volumes` unless the stored development data is intentionally being deleted.

## Current security scope

Username and password authentication is enabled.

TLS and topic-specific access-control rules have not yet been configured. The current setup is intended for development on a trusted local network and must not be exposed directly to the internet.
