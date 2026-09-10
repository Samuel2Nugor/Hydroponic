# Raspberry Pi deployment

This guide deploys the MicroHydros core services to a Raspberry Pi Zero 2W:

- Mosquitto
- Python telemetry service
- Node-RED

For detailed MQTT usage and data-flow information, see
[`docker/README.md`](../docker/README.md).

## 1. Verify the Raspberry Pi

The supported deployment uses a 64-bit operating system.

```bash
uname -m
free -h
df -h /
```

Expected architecture:

```text
aarch64
```

Verify Docker Engine and Docker Compose:

```bash
docker --version
docker compose version
```

If Docker is not installed, follow the official
[Docker Engine installation instructions for Debian](https://docs.docker.com/engine/install/debian/).

## 2. Check for a native Mosquitto conflict

Only the Dockerized Mosquitto broker should use port `1883`.

```bash
sudo systemctl status mosquitto --no-pager
sudo ss -tulpn | grep ':1883'
```

If the native Mosquitto service is active, stop and disable it:

```bash
sudo systemctl disable --now mosquitto
```

Do not uninstall it unless removal is intentionally required.

## 3. Clone the repository

```bash
git clone https://github.com/Samuel2Nugor/Hydroponic.git
cd Hydroponic
git switch main
git pull --ff-only origin main
```

## 4. Create local MQTT credentials

The credential files must only exist on the Raspberry Pi.

Create the first MQTT user:

```bash
docker run --rm -it \
  -v "$PWD/docker/mosquitto/config:/mosquitto/config" \
  eclipse-mosquitto:2.1.2-alpine \
  mosquitto_passwd -c /mosquitto/config/password_file esp32s3-01
```

Add the Node-RED user:

```bash
docker run --rm -it \
  -v "$PWD/docker/mosquitto/config:/mosquitto/config" \
  eclipse-mosquitto:2.1.2-alpine \
  mosquitto_passwd /mosquitto/config/password_file node-red
```

Add the telemetry-service user:

```bash
docker run --rm -it \
  -v "$PWD/docker/mosquitto/config:/mosquitto/config" \
  eclipse-mosquitto:2.1.2-alpine \
  mosquitto_passwd /mosquitto/config/password_file telemetry-service
```

Create the private environment file:

```bash
cp .env.example .env
```

Set `MQTT_PASSWORD` in `.env` to the password created for the
`telemetry-service` user.

Confirm that both secret files are ignored:

```bash
git check-ignore -v .env
git check-ignore -v docker/mosquitto/config/password_file
```

Never display, copy into documentation or commit either secret file.

## 5. Validate and start the services

Validate the configuration without printing resolved credentials:

```bash
docker compose config --quiet
```

Build and start the stack:

```bash
docker compose up -d --build
```

Check the containers and recent logs:

```bash
docker compose ps
docker compose logs --tail=50 mosquitto telemetry-service node-red
```

## 6. Configure Node-RED

Find the Raspberry Pi IP address:

```bash
hostname -I
```

From a browser on the same trusted network, open:

```text
http://<PI_IP>:1880
```

Import:

```text
docker/node-red/flows/validated-telemetry.json
```

Configure the MQTT broker node locally:

- Server: `mosquitto`
- Port: `1883`
- Username: `node-red`
- Password: the local Node-RED MQTT password
- Topic: `microhydros/v1/devices/+/telemetry/validated/+`
- QoS: `1`

Select **Deploy**.

## 7. Verify operation

Check service state:

```bash
docker compose ps
```

Follow logs:

```bash
docker compose logs -f mosquitto telemetry-service node-red
```

Check Raspberry Pi resource usage:

```bash
docker stats --no-stream
free -h
```

Use the MQTT test procedure in [`docker/README.md`](../docker/README.md)
to verify communication from the Raspberry Pi and then from the laptop.

## 8. Restart and update

Restart the stack:

```bash
docker compose restart
```

Stop the stack without deleting stored data:

```bash
docker compose down
```

Update the deployment:

```bash
git switch main
git pull --ff-only origin main
docker compose up -d --build
```

Do not add `--volumes` to `docker compose down` unless the stored
Mosquitto and Node-RED data is intentionally being deleted.

## Security scope

This configuration is for a trusted local development network.

- Do not expose ports `1883` or `1880` through router port forwarding.
- MQTT authentication is required.
- The Node-RED editor must not be exposed to the public internet.
- Docker-published ports may bypass normal UFW rules.
- `.env` and `password_file` must remain local to each deployment.