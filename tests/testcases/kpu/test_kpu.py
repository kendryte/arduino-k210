def test_kpu(dut):
    """Load and run KPU models from SPIFFS."""
    dut.expect_unity_test_output(timeout=240)
