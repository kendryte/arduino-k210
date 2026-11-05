# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026, arduino-k210 contributors


def test_touchscreen(dut):
    """Touch events, calibration, and optional NS2009 hardware"""
    dut.expect_unity_test_output(timeout=240)
