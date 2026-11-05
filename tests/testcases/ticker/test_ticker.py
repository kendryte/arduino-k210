# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026, arduino-k210 contributors


def test_ticker(dut):
    """hardware timer allocation and callback tests"""
    dut.expect_unity_test_output(timeout=240)
