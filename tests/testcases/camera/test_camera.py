def test_camera(dut):
    '''camera reset and snapshot test'''
    dut.expect_unity_test_output(timeout=240)
