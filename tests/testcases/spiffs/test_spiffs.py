def test_spiffs(dut):
    '''spiffs filesystem test'''
    dut.expect_unity_test_output(timeout=240)
