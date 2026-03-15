#!/usr/bin/python3

# PlatformIO pre-build script that injects the current time and timezone
# as build flags so the firmware can set the clock at boot.
#
# Defines:
#   BUILD_EPOCH     - current UTC epoch seconds at build time
#   BUILD_TZ_OFFSET - local timezone offset from UTC in hours

import time

Import("env")


def _get_tz_offset_hours():
    """Return the local timezone offset from UTC in whole hours."""
    utc_offset_secs = time.localtime().tm_gmtoff
    return int(utc_offset_secs / 3600)


epoch = int(time.time())
tz_offset = _get_tz_offset_hours()

env.Append(BUILD_FLAGS=[
    f"-D BUILD_EPOCH={epoch}",
    f"-D BUILD_TZ_OFFSET={tz_offset}",
])
