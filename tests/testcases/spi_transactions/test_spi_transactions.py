# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026, arduino-k210 contributors


def test_spi_transactions(dut):
    """SPI setters preserve an active transaction across RT-Thread tasks"""
    dut.expect_unity_test_output(timeout=240)
