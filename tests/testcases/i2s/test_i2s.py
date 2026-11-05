# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026, arduino-k210 contributors


def test_i2s(dut):
    """I2S validation, ownership, and DMA transmit behavior"""
    dut.expect_unity_test_output(timeout=240)
