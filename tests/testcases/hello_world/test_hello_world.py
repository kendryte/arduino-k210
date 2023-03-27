def test_hello_world(dut):
    '''hello world.'''
    dut.expect('Hello Arduino!')
