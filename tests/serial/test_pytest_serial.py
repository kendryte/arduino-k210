"""Verify the hardware runner's setup path with every serial/flashing operation mocked."""

import plistlib
import runpy
import subprocess
import sys
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

import pytest
from pytest_embedded_arduino.serial import ArduinoSerial
from pytest_embedded_serial import Serial, SerialDut
from pytest_embedded_serial import ports as port_selection
from serial.tools import list_ports


def ports():
    return [
        SimpleNamespace(device='/dev/cu.K1-A', serial_number='K1-123', interface='UART A', location='20-1.0'),
        SimpleNamespace(device='/dev/cu.K1-B', serial_number='K1-123', interface='UART B', location='20-1.1'),
        SimpleNamespace(device='/dev/cu.OTHER', serial_number='OTHER', interface='UART B', location='20-2.1'),
    ]


@pytest.mark.parametrize(
    'options',
    [
        {'port_location': '20-1.1'},
        {'port_serial_number': 'K1-123', 'port_interface': 'UART B'},
        {'port': '/dev/cu.K1-B'},
        {'port': '/dev/cu.K1-B', 'esptool_baud': 460800},
    ],
)
def test_constructor_passes_selected_port_to_serial_and_flasher(options):
    app = SimpleNamespace(target='canaan_k1', flash_files=[(0, 'test.bin', False)])
    with (
        patch('pytest_embedded_serial.serial.list_serial_ports', return_value=ports()),
        patch('pytest_embedded_serial.serial.pyserial.serial_for_url') as create,
        patch.object(Serial, 'start_redirect_thread'),
        patch.object(Serial, 'disable_redirect_thread'),
        patch('kflash.KFlash.process') as flash,
    ):
        device = ArduinoSerial(app=app, pexpect_proc=MagicMock(), msg_queue=MagicMock(), **options)
        try:
            assert device.port == '/dev/cu.K1-B'
            assert create.call_args.args[0] == '/dev/cu.K1-B'
            assert create.call_args.kwargs['do_not_open'] is True
            assert flash.call_args.kwargs['dev'] == '/dev/cu.K1-B'
            assert flash.call_args.kwargs['baudrate'] == options.get('esptool_baud', 115200)
        finally:
            device.close()


@pytest.mark.parametrize(
    'option, environment, expected',
    [(None, None, 115200), (None, '460800', 460800), ('115200', '1500000', 115200)],
)
def test_flash_baud_option_overrides_environment(request, monkeypatch, option, environment, expected):
    monkeypatch.setattr(request.config.option, 'esptool_baud', option)
    if environment is None:
        monkeypatch.delenv('ESPBAUD', raising=False)
    else:
        monkeypatch.setenv('ESPBAUD', environment)
    options = request.getfixturevalue('_fixture_classes_and_options')
    assert options.kwargs['serial']['esptool_baud'] == expected


@pytest.mark.parametrize(
    'options',
    [
        {},
        {'port_serial_number': 'K1-123'},
        {'port_serial_number': 'MISSING'},
        {'port': '/dev/cu.OTHER', 'port_serial_number': 'K1-123'},
    ],
)
def test_invalid_selection_cannot_open_or_flash_any_port(options):
    app = SimpleNamespace(target='canaan_k1', flash_files=[(0, 'test.bin', False)])
    with (
        patch('pytest_embedded_serial.serial.list_serial_ports', return_value=ports()),
        patch('pytest_embedded_serial.serial.pyserial.serial_for_url') as create,
        patch('kflash.KFlash.process') as flash,
    ):
        with pytest.raises(ValueError):
            ArduinoSerial(app=app, pexpect_proc=MagicMock(), msg_queue=MagicMock(), **options)
        create.assert_not_called()
        flash.assert_not_called()


@pytest.mark.parametrize(
    'port, port_serial_number, port_interface, port_location, port_vid, port_pid',
    [
        ('/dev/cu.K1-B', 'K1-123', 'UART B', '20-1.1', '0x1A86', '0x55D2'),
    ],
    indirect=True,
)
def test_pytest_options_reach_k210_serial_and_dut(_fixture_classes_and_options, session_tempdir, pytestconfig):
    from pytest_embedded.plugin import _session_tempdir_key

    options = _fixture_classes_and_options
    assert options.classes['serial'] is ArduinoSerial
    assert options.classes['dut'] is SerialDut
    assert options.kwargs['serial']['port'] == '/dev/cu.K1-B'
    assert options.kwargs['serial']['port_serial_number'] == 'K1-123'
    assert options.kwargs['serial']['port_interface'] == 'UART B'
    assert options.kwargs['serial']['port_location'] == '20-1.1'
    assert options.kwargs['serial']['port_vid'] == '0x1A86'
    assert options.kwargs['serial']['port_pid'] == '0x55D2'
    assert pytestconfig.stash[_session_tempdir_key] == session_tempdir


def test_arduino_fqbn_selects_k1_uart_using_usb_identity():
    app = SimpleNamespace(target='k1', flash_files=[(0, 'test.bin', False)])
    candidates = [
        SimpleNamespace(device='COM23', serial_number='5363000516', vid=6790, pid=21970, location=None, interface=None),
        SimpleNamespace(
            device='COM107', serial_number='5363000516', vid=6790, pid=21970, location='3-4:x.2', interface=None
        ),
    ]
    with (
        patch('pytest_embedded_serial.serial.list_serial_ports', return_value=candidates),
        patch('pytest_embedded_serial.serial.pyserial.serial_for_url') as create,
        patch.object(Serial, 'start_redirect_thread'),
        patch.object(Serial, 'disable_redirect_thread'),
        patch('kflash.KFlash.process') as flash,
    ):
        device = ArduinoSerial(
            app=app,
            target='auto',
            pexpect_proc=MagicMock(),
            msg_queue=MagicMock(),
            port_serial_number='5363000516',
            port_vid='0x1A86',
            port_pid='0x55D2',
        )
        try:
            assert device.port == 'COM107'
            assert create.call_args.args[0] == 'COM107'
            assert flash.call_args.kwargs['dev'] == 'COM107'
            assert 'port_vid' not in create.call_args.kwargs
            assert 'port_pid' not in create.call_args.kwargs
        finally:
            device.close()


def test_usb_mismatch_never_opens_or_flashes_an_explicit_port():
    candidates = [SimpleNamespace(device='COM24', serial_number='5363000516', vid=6790, pid=21970)]
    with (
        patch('pytest_embedded_serial.serial.list_serial_ports', return_value=candidates),
        patch('pytest_embedded_serial.serial.pyserial.serial_for_url') as create,
        patch('kflash.KFlash.process') as flash,
    ):
        with pytest.raises(ValueError, match='No serial port matches'):
            ArduinoSerial(
                app=SimpleNamespace(target='k1'),
                pexpect_proc=MagicMock(),
                msg_queue=MagicMock(),
                port='COM24',
                port_serial_number='5363000516',
                port_vid='0xFFFF',
            )
        create.assert_not_called()
        flash.assert_not_called()


def test_flash_failure_preserves_original_error_and_console_settings():
    app = SimpleNamespace(target='k1', flash_files=[(0, 'test.bin', False)])
    failure = RuntimeError('K210 bootloader did not respond')
    with (
        patch('pytest_embedded_serial.serial.pyserial.serial_for_url') as create,
        patch.object(Serial, 'start_redirect_thread'),
        patch.object(Serial, 'disable_redirect_thread'),
        patch('kflash.KFlash.process', side_effect=failure) as flash,
    ):
        with pytest.raises(RuntimeError, match='K210 bootloader did not respond') as caught:
            ArduinoSerial(app=app, pexpect_proc=MagicMock(), msg_queue=MagicMock(), port='COM24')
        assert caught.value is failure
        flash.assert_called_once()
        create.return_value.apply_settings.assert_called_once_with(create.return_value.get_settings.return_value)


@pytest.mark.parametrize('dictionary_root', [False, True])
@pytest.mark.parametrize('dictionary_child', [False, True])
@pytest.mark.parametrize('interface_numbers', [(0, 2), (1, 3)])
def test_macos_enumeration_enriches_metadata_without_opening_ports(
    dictionary_root, dictionary_child, interface_numbers
):
    candidates = [
        SimpleNamespace(device=path, serial_number='5363000516', vid=6790, pid=21970, location='20-1', interface=None)
        for path in ('/dev/cu.A', '/dev/cu.B')
    ]
    root = {
        'IOObjectClass': 'IOUSBHostDevice',
        'idVendor': 6790,
        'idProduct': 21970,
        'IORegistryEntryChildren': [],
    }
    for candidate, number in zip(candidates, interface_numbers):
        serial_node = {'IOObjectClass': 'IOSerialBSDClient', 'IOCalloutDevice': candidate.device}
        root['IORegistryEntryChildren'].append(
            {
                'IOObjectClass': 'IOUSBHostInterface',
                'idVendor': 6790,
                'idProduct': 21970,
                'bInterfaceNumber': number,
                'IORegistryEntryChildren': [
                    {
                        # Apple repeats these IDs on the driver node, which is
                        # still part of the same USB device and interface.
                        'IOObjectClass': 'AppleUSBACMData',
                        'idVendor': 6790,
                        'idProduct': 21970,
                        'IORegistryEntryChildren': serial_node if dictionary_child else [serial_node],
                    }
                ],
            }
        )
    registry = root if dictionary_root else [root]
    with (
        patch.object(port_selection.sys, 'platform', 'darwin'),
        patch.object(list_ports, 'comports', return_value=candidates),
        patch.object(port_selection.subprocess, 'run', return_value=SimpleNamespace(stdout=plistlib.dumps(registry))),
        patch('serial.Serial') as open_port,
    ):
        result = port_selection.list_serial_ports()
        assert [port.interface_number for port in result] == list(interface_numbers)
        for order in (result, list(reversed(result))):
            assert port_selection.select_port(order, serial_number='5363000516', board='k1') == '/dev/cu.B'
        open_port.assert_not_called()


@pytest.mark.parametrize('registry', ['invalid root', {'IORegistryEntryChildren': ['invalid child']}])
def test_macos_invalid_registry_reports_missing_metadata_without_selecting_uart(registry, capsys):
    candidates = [
        SimpleNamespace(
            device='/dev/cu.A', serial_number='5363000516', vid=6790, pid=21970, location='20-1', interface=None
        )
    ]
    with (
        patch.object(port_selection.sys, 'platform', 'darwin'),
        patch.object(list_ports, 'comports', return_value=candidates),
        patch.object(port_selection.subprocess, 'run', return_value=SimpleNamespace(stdout=plistlib.dumps(registry))),
        patch('serial.Serial') as open_port,
    ):
        result = port_selection.list_serial_ports()
        assert 'USB interface metadata unavailable: Unexpected ioreg registry node' in capsys.readouterr().err
        with pytest.raises(ValueError, match='No serial port matches'):
            port_selection.select_port(result, serial_number='5363000516', board='k1')
        open_port.assert_not_called()


@pytest.mark.parametrize('error', [FileNotFoundError('ioreg'), subprocess.TimeoutExpired('ioreg', 10)])
def test_macos_missing_metadata_does_not_guess_uart(error):
    candidates = [
        SimpleNamespace(
            device='/dev/cu.A', serial_number='5363000516', vid=6790, pid=21970, location='20-1', interface=None
        )
    ]
    with (
        patch.object(port_selection.sys, 'platform', 'darwin'),
        patch.object(list_ports, 'comports', return_value=candidates),
        patch.object(port_selection.subprocess, 'run', side_effect=error),
    ):
        result = port_selection.list_serial_ports()
        with pytest.raises(ValueError, match='No serial port matches'):
            port_selection.select_port(result, serial_number='5363000516', board='k1')


@pytest.mark.parametrize('use_cli', [False, True])
def test_selector_cli_uses_environment_and_allows_cli_overrides(monkeypatch, capsys, use_cli):
    tool = Path(__file__).resolve().parents[2] / 'tools/select_serial_port.py'
    values = {'K210_PORT_SERIAL_NUMBER': '5363000516', 'K210_PORT_VID': '0x1A86', 'K210_PORT_PID': '0x55D2'}
    for key in ['K210_PORT', 'K210_PORT_INTERFACE', 'K210_PORT_LOCATION']:
        monkeypatch.delenv(key, raising=False)
    for key, value in values.items():
        monkeypatch.setenv(key, 'WRONG' if use_cli else value)
    args = [str(tool), '--board', 'k1']
    if use_cli:
        args += ['--port-serial-number', '5363000516', '--port-vid', '6790', '--port-pid', '21970']
    monkeypatch.setattr(sys, 'argv', args)
    monkeypatch.setattr(sys, 'platform', 'linux')  # Simulated pySerial metadata, independent of CI host.
    candidates = [
        SimpleNamespace(
            device='COM107', serial_number='5363000516', vid=6790, pid=21970, location='3-4:x.2', interface=None
        )
    ]
    with patch.object(list_ports, 'comports', return_value=candidates), patch('serial.Serial') as open_port:
        runpy.run_path(str(tool), run_name='__main__')
        assert capsys.readouterr().out.strip() == 'COM107'
        open_port.assert_not_called()
