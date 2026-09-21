import os
from dataclasses import dataclass


class ConfigurationError(RuntimeError):
    """Raised when required service configuration is invalid."""


@dataclass(frozen=True)
class Settings:
    mqtt_host: str
    mqtt_port: int
    mqtt_username: str
    mqtt_password: str
    mqtt_client_id: str
    mqtt_keepalive_seconds: int
    mqtt_ca_cert: str


def load_settings() -> Settings:
    return Settings(
        mqtt_ca_cert=_required_environment_value("MQTT_CA_CERT"),
        mqtt_host=_required_environment_value("MQTT_HOST"),
        mqtt_port=_integer_environment_value(
            "MQTT_PORT",
            minimum=1,
            maximum=65535,
        ),
        mqtt_username=_required_environment_value("MQTT_USERNAME"),
        mqtt_password=_required_environment_value("MQTT_PASSWORD"),
        mqtt_client_id=_required_environment_value("MQTT_CLIENT_ID"),
        mqtt_keepalive_seconds=_integer_environment_value(
            "MQTT_KEEPALIVE_SECONDS",
            minimum=1,
            maximum=65535,
        ),
    )


def _required_environment_value(name: str) -> str:
    value = os.environ.get(name)

    if value is None or not value.strip():
        raise ConfigurationError(
            f"Required environment variable is missing: {name}"
        )

    return value


def _integer_environment_value(
    name: str,
    *,
    minimum: int,
    maximum: int,
) -> int:
    raw_value = _required_environment_value(name)

    try:
        value = int(raw_value)
    except ValueError as error:
        raise ConfigurationError(
            f"{name} must be an integer"
        ) from error

    if not minimum <= value <= maximum:
        raise ConfigurationError(
            f"{name} must be between {minimum} and {maximum}"
        )

    return value