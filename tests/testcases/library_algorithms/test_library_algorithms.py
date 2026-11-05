# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026, arduino-k210 contributors


def test_library_algorithms(dut):
    """portable library algorithms on the K210"""
    dut.expect_unity_test_output(timeout=240)
