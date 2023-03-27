def test_uart_loop(dut):
    '''uart write and read loop test'''
    dut.expect_unity_test_output(timeout=240)
