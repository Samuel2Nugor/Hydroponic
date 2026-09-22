# Docker configuration

Service configuration for the MicroHydros backend. See the [root README](../README.md) for startup, shutdown, service addresses and end-to-end verification.

## Configuration files

| Path | Purpose |
| ---- | ------- |
| `mosquitto/config/mosquitto.conf` | Broker listeners, authentication and TLS |
| `mosquitto/config/acl_file` | MQTT topic permissions |
| `node-red/flows/validated-telemetry.json` | Exported telemetry inspection flow |
| `telegraf/telegraf.conf` | MQTT consumption and InfluxDB output |
| `grafana/provisioning/` | Datasource and dashboard provisioning |
| `grafana/dashboards/` | Dashboard definitions |

Run the commands below from the repository root.

## MQTT accounts

The configuration uses four accounts:

- `esp32s3-01`
- `telemetry-service`
- `node-red`
- `telegraf`

Create the password file with the first account:

```bash
docker run --rm -it \
  -v "$PWD/docker/mosquitto/config:/mosquitto/config" \
  eclipse-mosquitto:2.1.2-alpine \
  mosquitto_passwd -c /mosquitto/config/password_file esp32s3-01
```

Add each remaining account using the same command **without `-c`**, replacing `<service-name>`:

```bash
docker run --rm -it \
  -v "$PWD/docker/mosquitto/config:/mosquitto/config" \
  eclipse-mosquitto:2.1.2-alpine \
  mosquitto_passwd /mosquitto/config/password_file <service-name>
```

The `-c` option creates or overwrites the password file. Keep the file outside Git.

## Environment

Create `.env` from `.env.example` if it does not already exist. Configure:

```dotenv
MQTT_HOST=mosquitto
MQTT_PORT=8883
MQTT_CA_CERT=/etc/microhydros/certs/ca.crt
MQTT_USERNAME=telemetry-service
TELEGRAF_MQTT_USERNAME=telegraf
```

Set the corresponding passwords to match the broker accounts. Configure ESP32-S3 and Node-RED credentials in their respective clients.

Keep `.env`, password files and private keys outside Git.

## TLS certificates

Provision these files locally before starting the stack:

| Path | Purpose |
| ---- | ------- |
| `docker/mosquitto/certs/ca.crt` | Public CA certificate |
| `docker/mosquitto/certs/server.crt` | Broker certificate |
| `docker/mosquitto/certs/server.key` | Broker private key |

The broker certificate's Subject Alternative Names must cover the names used by clients: the host's stable mDNS hostname, `mosquitto`, `localhost` and IP address `127.0.0.1`.

Mosquitto mounts the certificate directory at `/mosquitto/certs`. Backend clients mount only the public CA certificate at `/etc/microhydros/certs/ca.crt`.

Keep the CA private key outside containers. Certificate generation and renewal are not automated by Compose.

MQTT uses port `8883`; the plaintext listener on `1883` is disabled.

## Topic permissions

| Account | Allowed operations |
| ------- | ------------------ |
| `esp32s3-01` | Publish its own raw telemetry and status |
| `telemetry-service` | Read raw telemetry; publish validated, rejected and broker health messages |
| `node-red` | Read validated telemetry and device status |
| `telegraf` | Read validated telemetry |

Other operations are denied. Reading rejected telemetry requires an explicit diagnostic permission.

## Broker healthcheck

The healthcheck publishes to `microhydros/health/mosquitto` over TLS using MQTT v5 and QoS 1. It currently reuses the telemetry-service credentials.

Compose uses `$$` in the command to expand credentials inside the container. An accepted publication may return reason code `0` or `16` (no matching subscribers).

Broker health does not verify the complete telemetry pipeline.

## Node-RED flow

Import `docker/node-red/flows/validated-telemetry.json` into the editor.

The exported flow uses:

- Broker: `mosquitto:8883`
- TLS with server certificate verification enabled
- CA: `/etc/microhydros/certs/ca.crt`
- Server name: `mosquitto`
- Topic: `microhydros/v1/devices/+/telemetry/validated/+`
- QoS: `1`

Enter the `node-red` MQTT credentials and deploy. Credentials are not included in the export.

The flow displays validated messages in Debug. Validation remains in the Python service. Export subsequent editor changes to keep the repository flow updated.

## Telegraf and InfluxDB

Telegraf consumes validated telemetry through `ssl://mosquitto:8883`, verifies the broker certificate and writes measurements to the `telemetry` bucket.

InfluxDB initialization settings apply only to an empty data volume. Changing them in `.env` does not update existing database credentials.

## Security scope

MQTT TLS does not protect the HTTP interfaces or the HTTP connection to InfluxDB. Node-RED editor authentication and separate restricted InfluxDB tokens remain unconfigured.
