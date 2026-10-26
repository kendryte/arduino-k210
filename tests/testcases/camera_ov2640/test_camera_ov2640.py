# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026, arduino-k210 contributors


def test_camera_ov2640(dut):
    """OV2640 validation and snapshot when that sensor is installed"""
    dut.expect_unity_test_output(timeout=240)
