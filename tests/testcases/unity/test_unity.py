def test_unity(dut):
    '''unity framwork test'''
    dut.expect_unity_test_output(timeout=240)
