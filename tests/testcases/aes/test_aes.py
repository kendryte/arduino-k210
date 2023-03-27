def test_aes(dut):
    '''aes encrypt and decrypt test'''
    dut.expect_unity_test_output(timeout=240)
