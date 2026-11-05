# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026, arduino-k210 contributors


def test_neopixel(dut):
    """NeoPixel color buffering and K210 waveform output"""
    dut.expect_unity_test_output(timeout=240)
