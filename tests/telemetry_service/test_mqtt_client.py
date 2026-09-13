import logging
from types import SimpleNamespace
from unittest.mock import Mock

import telemetry_service.mqtt_client as mqtt_client

import pytest

from telemetry_service.mqtt_client import (
    build_rejected_topic,
    build_validated_topic,
    extract_device_id,
    TelemetryMqttService,
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

def test_on_message_logs_processing_summary_and_publishes(monkeypatch, caplog):
    validated = [
        {
            "measurement": "internal_temperature",
            "value": 23.6,
        }
    ]
    rejected = [
        {
            "measurement": "water_temperature",
            "reason_code": "sensor_read_error",
        }
    ]

    monkeypatch.setattr(
        mqtt_client,
        "validate_raw_payload",
        Mock(return_value=(validated, rejected)),
    )

    service = object.__new__(TelemetryMqttService)
    publish_json = Mock()
    monkeypatch.setattr(service, "_publish_json", publish_json)

    message = SimpleNamespace(
        topic="microhydros/v1/devices/esp32s3-01/telemetry/raw",
        payload=b'{"test":true}',
    )

    with caplog.at_level(logging.INFO, logger=mqtt_client.__name__):
        service._on_message(None, None, message)

    assert "Received raw telemetry: device_id=esp32s3-01" in caplog.text
    assert (
        "Validation completed: device_id=esp32s3-01 "
        "validated=1 rejected=1"
    ) in caplog.text
    assert publish_json.call_count == 2
