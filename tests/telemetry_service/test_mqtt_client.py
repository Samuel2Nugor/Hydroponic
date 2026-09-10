import pytest

from telemetry_service.mqtt_client import (
    build_rejected_topic,
    build_validated_topic,
    extract_device_id,
)


def test_extract_device_id_from_raw_topic():
    topic = "microhydros/v1/devices/esp32s3-01/telemetry/raw"

    assert extract_device_id(topic) == "esp32s3-01"


@pytest.mark.parametrize(
    "topic",
    [
        "",
        "microhydros/v1/devices//telemetry/raw",
        "microhydros/v1/devices/esp32s3-01/telemetry",
        "microhydros/v1/devices/esp32s3-01/status",
        "another/v1/devices/esp32s3-01/telemetry/raw",
    ],
)
def test_invalid_raw_topic_returns_none(topic):
    assert extract_device_id(topic) is None


def test_build_validated_topic():
    topic = build_validated_topic(
        device_id="esp32s3-01",
        measurement="water_temperature",
    )

    assert topic == (
        "microhydros/v1/devices/esp32s3-01/"
        "telemetry/validated/water_temperature"
    )


def test_build_rejected_topic():
    topic = build_rejected_topic(device_id="esp32s3-01")

    assert topic == (
        "microhydros/v1/devices/esp32s3-01/telemetry/rejected"
    )