def test_core_compat(dut):
    """core compatibility smoke test"""
    dut.expect('CORE_COMPAT_PASS', timeout=240)
