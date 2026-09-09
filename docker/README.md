# Docker development environment

This directory contains configuration for the MicroHydros Docker services.

## Core services

| Service   | Purpose                                           | Port   |
| --------- | ------------------------------------------------- | ------ |
| Mosquitto | Routes MQTT messages between system components    | `1883` |
| Node-RED  | Displays validated data and supports simple flows | `1880` |

The Python telemetry service will be added after its validation logic has been implemented and tested.

## MQTT authentication

Mosquitto does not permit anonymous connections.

Each developer must create a local password file containing the required MQTT users:

* `esp32s3-01`
* `node-red`
* `telemetry-service` will be added with the Python service

The password file is stored at:

```text
docker/mosquitto/config/password_file
```

This file contains password hashes and must not be committed to Git.

Confirm that Git ignores it:

```bash
git check-ignore -v docker/mosquitto/config/password_file
```

## Start the services

Docker Desktop must be running before starting the services.

From the repository root, run:

```bash
docker compose up -d
```

Check their status:

```bash
docker compose ps
```

View recent logs:

```bash
docker compose logs --tail=50 mosquitto node-red
```

## Access during local development

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

## Stop the services

Stop and remove the containers and network without deleting stored data:

```bash
docker compose down
```

Do not add `--volumes` unless the stored development data is intentionally being deleted.

## Current security scope

Username and password authentication is enabled. TLS and topic-specific access-control rules have not yet been configured.

The current setup is intended for development on a trusted local network and must not be exposed directly to the internet.
