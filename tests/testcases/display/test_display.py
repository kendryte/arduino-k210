# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026, arduino-k210 contributors


def test_display(dut):
    """ST7789 framebuffer, rotation, and K210 8080 bus behavior"""
    dut.expect_unity_test_output(timeout=240)
