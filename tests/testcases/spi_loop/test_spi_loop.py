# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026, arduino-k210 contributors


def test_spi_loop(dut):
    """SPI1 MOSI 34 to MISO 35 hardware loopback"""
    dut.expect_unity_test_output(timeout=240)
