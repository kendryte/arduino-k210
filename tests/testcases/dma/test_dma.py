# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026, arduino-k210 contributors


def test_dma(dut):
    """DMA channel allocation and memory-transfer behavior"""
    dut.expect_unity_test_output(timeout=240)
