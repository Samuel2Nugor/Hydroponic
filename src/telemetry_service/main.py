import logging

from telemetry_service.config import ConfigurationError, load_settings
from telemetry_service.mqtt_client import TelemetryMqttService


LOGGER = logging.getLogger(__name__)


def main() -> None:
    logging.basicConfig(
        level=logging.INFO,
        format=(
            "%(asctime)s %(levelname)s "
            "%(name)s: %(message)s"
        ),
    )

    try:
        settings = load_settings()
    except ConfigurationError as error:
        LOGGER.error("Configuration error: %s", error)
        raise SystemExit(1) from error

    service = TelemetryMqttService(settings)

    try:
        service.run()
    except KeyboardInterrupt:
        LOGGER.info("Telemetry service stopped by user")


if __name__ == "__main__":
    main()