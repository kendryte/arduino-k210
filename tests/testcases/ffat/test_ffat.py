def test_ffat(dut):
    '''fat filesystem test'''
    dut.expect_unity_test_output(timeout=240)
