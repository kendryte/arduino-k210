def test_kpu(dut):
    '''Kpu run yolo face detect test'''
    dut.expect_unity_test_output(timeout=240)
