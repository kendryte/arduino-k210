def test_digital_io_loop(dut):
    '''digtal io write and read loop test'''
    dut.expect_unity_test_output(timeout=240)
