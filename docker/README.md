# Docker development environment

This directory contains the Docker configuration for the MicroHydros development environment.

The active environment currently runs on the development Mac. Raspberry Pi deployment is optional and currently deferred. See the [Raspberry Pi deployment guide](../docs/raspberry-pi-deployment.md) for the earlier deployment procedure.

## Core services

| Service | Purpose | Host port |
| ------- | ------- | --------- |
| Mosquitto | Authenticated MQTT broker | `1883` |
| Telemetry service | Validates, timestamps and separates raw sensor measurements | None |
| Node-RED | Displays validated MQTT messages and supports future automation | `1880` |
| Telegraf | Reads validated MQTT telemetry and writes it to InfluxDB | None |
| InfluxDB | Stores time-series telemetry | `8086` |
| Grafana | Displays provisioned telemetry dashboards | `3000` |

## Service dependencies

- The telemetry service waits for Mosquitto to become healthy.
- Node-RED waits for Mosquitto to become healthy.
- Telegraf waits for Mosquitto and InfluxDB to become healthy.
- Grafana waits for InfluxDB to become healthy.

A running container does not always prove that its application is processing messages. Inspect service logs and perform an end-to-end telemetry test when verifying the environment.

## MQTT authentication

Mosquitto does not permit anonymous connections.

Each developer must create a local password file containing these MQTT users:

- `esp32s3-01`
- `node-red`
- `telemetry-service`
- `telegraf`

The password file is stored at:

```text
docker/mosquitto/config/password_file
```

To create the file and first user from the repository root:

```bash
docker run --rm -it \
  -v "$PWD/docker/mosquitto/config:/mosquitto/config" \
  eclipse-mosquitto:2.1.2-alpine \
  mosquitto_passwd -c /mosquitto/config/password_file esp32s3-01
```

Add the remaining users without the `-c` option:

```bash
docker run --rm -it \
  -v "$PWD/docker/mosquitto/config:/mosquitto/config" \
  eclipse-mosquitto:2.1.2-alpine \
  mosquitto_passwd /mosquitto/config/password_file node-red
```

```bash
docker run --rm -it \
  -v "$PWD/docker/mosquitto/config:/mosquitto/config" \
  eclipse-mosquitto:2.1.2-alpine \
  mosquitto_passwd /mosquitto/config/password_file telemetry-service
```

```bash
docker run --rm -it \
  -v "$PWD/docker/mosquitto/config:/mosquitto/config" \
  eclipse-mosquitto:2.1.2-alpine \
  mosquitto_passwd /mosquitto/config/password_file telegraf
```

The password file contains password hashes and must not be committed.

Confirm that Git ignores it:

```bash
git check-ignore -v docker/mosquitto/config/password_file
```

Do not display or commit the complete password file.

## Environment configuration

Create the private environment file:

```bash
cp .env.example .env
```

Replace every password and token placeholder in `.env`.

The following values must match the corresponding Mosquitto password-file accounts:

- `MQTT_USERNAME=telemetry-service`
- `MQTT_PASSWORD`
- `TELEGRAF_MQTT_USERNAME=telegraf`
- `TELEGRAF_MQTT_PASSWORD`

The ESP32 and Node-RED MQTT passwords are configured in their respective clients and are not stored in the repository.

The `.env` file also initializes InfluxDB and configures Grafana. It contains credentials and must not be committed.

Confirm that Git ignores it:

```bash
git check-ignore -v .env
```

## Validate the configuration

Validate Compose without displaying resolved environment values:

```bash
docker compose config --quiet
```

Do not share the output of plain `docker compose config`, because resolved values may contain credentials.

## Start the environment

Docker Desktop or Docker Engine must be running.

From the repository root, run:

```bash
docker compose up -d --build --wait
```

Check service status:

```bash
docker compose ps
```

## Inspect logs

View recent logs from every service:

```bash
docker compose logs --tail=50
```

View the telemetry validation flow:

```bash
docker compose logs --tail=50 telemetry-service
```

For each successfully processed raw message, the telemetry service logs:

- The device and raw MQTT topic received
- The payload size
- The number of validated and rejected measurements
- Every MQTT topic queued for publishing

The logs do not include MQTT passwords, tokens or complete payload contents.

## Development addresses

- Node-RED: [http://localhost:1880](http://localhost:1880)
- InfluxDB: [http://localhost:8086](http://localhost:8086)
- Grafana: [http://localhost:3000](http://localhost:3000)

MQTT clients running directly on the Mac connect to:

```text
localhost:1883
```

Containers on the Compose network connect to:

```text
mosquitto:1883
```

The physical ESP32-S3 must connect to the Mac’s local network IP address, not `localhost`. The Mac and ESP32-S3 must be reachable on the same network.

## Data flow

```text
ESP32-S3
  -> microhydros/v1/devices/{device_id}/telemetry/raw
  -> Mosquitto
  -> Python telemetry service
  -> validated or rejected MQTT topics
```

Validated measurements are consumed in parallel:

```text
Validated MQTT
  -> Node-RED for visual inspection and future automation
  -> Telegraf
  -> InfluxDB
  -> Grafana
```

A failed sensor measurement is rejected independently. Other valid measurements from the same raw message continue through the system.

## Node-RED flow

The example flow is stored at:

```text
docker/node-red/flows/validated-telemetry.json
```

Import it through the Node-RED editor and select **Deploy**.

Configure its MQTT broker locally:

- Server: `mosquitto`
- Port: `1883`
- Username: `node-red`
- Password: the local `node-red` MQTT password
- Topic: `microhydros/v1/devices/+/telemetry/validated/+`
- QoS: `1`

Node-RED is not responsible for validating raw telemetry.

## Telegraf

Telegraf subscribes to:

```text
microhydros/v1/devices/+/telemetry/validated/+
```

It parses validated JSON messages and writes their measurements, fields and tags to the InfluxDB `telemetry` bucket.

Its configuration is stored at:

```text
docker/telegraf/telegraf.conf
```

## InfluxDB

InfluxDB is initialized using the values in `.env`.

The development bucket is:

```text
telemetry
```

InfluxDB initialization values are applied only when its data volume is empty. Changing initialization credentials in `.env` does not automatically update an already initialized volume.

## Grafana

The InfluxDB datasource is automatically provisioned from:

```text
docker/grafana/provisioning/datasources/influxdb.yaml
```

The **MicroHydros Telemetry** dashboard is automatically provisioned from:

```text
docker/grafana/dashboards/microhydros-telemetry.json
```

The dashboard contains panels for:

- Internal temperature
- Internal relative humidity
- External temperature
- Water temperature

## Persistent data

Docker named volumes preserve:

- Mosquitto data
- Node-RED data
- InfluxDB data and configuration
- Grafana data

Stop and remove containers and the Compose network without deleting stored data:

```bash
docker compose down
```

Do not add `--volumes` or `-v` unless the stored development data is intentionally being deleted.

## Current limitations

- TLS is not configured.
- MQTT topic-specific ACLs are not configured.
- Telegraf and Grafana currently use the development InfluxDB token rather than separate least-privilege tokens.
- Node-RED editor authentication is not configured.
- Sequence-gap detection is not implemented.
- Automatic silent-hang recovery is not implemented.

The environment is intended for development on a trusted local network and must not be exposed directly to the internet.
