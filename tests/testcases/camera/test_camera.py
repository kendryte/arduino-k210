def test_camera(dut):
    """GC0328 identification, controls, buffers, and snapshots"""
    dut.expect_unity_test_output(timeout=240)
