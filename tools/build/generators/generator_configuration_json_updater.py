#!/usr/bin/python3

from enum import Enum

generator_configuration_format_version = "1.0"

class UpdateStatus(Enum):
    NoUpdateNeeded = 1
    Updated = 2
    Error = 3


def update_generator_configuration_json(json):
    return json, UpdateStatus.NoUpdateNeeded
