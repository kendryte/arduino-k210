def test_sha256(dut):
    '''sha256 compute test'''
    dut.expect_unity_test_output(timeout=240)
