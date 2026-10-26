# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026, arduino-k210 contributors


def test_wire(dut):
    """Wire lifecycle, ownership, bounds, and invalid-request behavior"""
    dut.expect_unity_test_output(timeout=240)
