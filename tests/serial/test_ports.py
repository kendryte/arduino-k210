"""Exercise multiple USB adapters/UARTs without touching real serial devices."""

import unittest
from types import SimpleNamespace

from pytest_embedded_serial.ports import macos_interfaces, select_port


def device(path, serial='K1-123', interface='UART B', location='20-1.1', *, vid=None, pid=None):
    return SimpleNamespace(device=path, serial_number=serial, interface=interface, location=location, vid=vid, pid=pid)


def k1_ports():
    # CH342 Windows metadata: no interface names, only UART B has a location.
    return [
        device('COM23', '5363000516', None, None, vid=6790, pid=21970),
        device('COM24', '5363000516', None, '1-1:x.2', vid=6790, pid=21970),
    ]


class PortTests(unittest.TestCase):
    def setUp(self):
        self.ports = [
            device('/dev/cu.Bluetooth-Incoming-Port', None, None, None),
            device('/dev/cu.usbserial-A', interface='UART A', location='20-1.0'),
            device('/dev/cu.usbserial-B'),
            device('/dev/cu.usbserial-other', serial='OTHER'),
        ]

    def test_no_implicit_first_port_even_with_single_device(self):
        for ports in [self.ports, self.ports[2:3], []]:
            with self.assertRaisesRegex(ValueError, 'Specify --port'):
                select_port(ports)

    def test_serial_and_interface_select_correct_uart_regardless_of_order(self):
        for ports in [self.ports, list(reversed(self.ports))]:
            self.assertEqual(select_port(ports, serial_number='K1-123', interface='UART B'), '/dev/cu.usbserial-B')

    def test_same_serial_on_two_interfaces_is_ambiguous(self):
        with self.assertRaisesRegex(ValueError, 'Ambiguous.*usbserial-A.*usbserial-B'):
            select_port(self.ports, serial_number='K1-123')

    def test_missing_serial_or_interface_does_not_fall_back(self):
        for filters in [dict(serial_number='MISSING'), dict(serial_number='K1-123', interface='UART C')]:
            with self.assertRaisesRegex(ValueError, 'No serial port matches'):
                select_port(self.ports, **filters)

    def test_location_resolves_to_device(self):
        self.assertEqual(select_port(self.ports, location='20-1.1', serial_number='K1-123'), '/dev/cu.usbserial-B')

    def test_non_unique_location_is_ambiguous(self):
        self.ports[1].location = self.ports[2].location
        with self.assertRaisesRegex(ValueError, 'Ambiguous'):
            select_port(self.ports, location='20-1.1')

    def test_explicit_port_works_without_usb_metadata(self):
        for path in ['/dev/cu.usbserial-B', '/dev/serial/by-id/k210', 'COM24', 'loop://']:
            self.assertEqual(select_port([], port=path), path)

    def test_explicit_port_and_metadata_must_both_match(self):
        with self.assertRaisesRegex(ValueError, 'No serial port matches'):
            select_port(self.ports, port='/dev/cu.usbserial-other', serial_number='K1-123')
        self.assertEqual(
            select_port(self.ports, port='/dev/cu.usbserial-B', serial_number='K1-123'), '/dev/cu.usbserial-B'
        )

    def test_macos_aliases_are_one_endpoint_and_prefer_callout(self):
        ports = [device('/dev/tty.usbserial-B'), device('/dev/cu.usbserial-B')]
        for order in [ports, list(reversed(ports))]:
            self.assertEqual(select_port(order, serial_number='K1-123'), '/dev/cu.usbserial-B')
        self.assertEqual(
            select_port(ports, port='/dev/tty.usbserial-B', serial_number='K1-123'), '/dev/tty.usbserial-B'
        )

    def test_occupied_port_and_its_macos_alias_are_rejected(self):
        with self.assertRaisesRegex(ValueError, 'already in use'):
            select_port(self.ports, serial_number='K1-123', interface='UART B', occupied=['/dev/tty.usbserial-B'])
        with self.assertRaisesRegex(ValueError, 'already in use'):
            select_port([], port='COM24', occupied=['COM24'])

    def test_occupied_port_does_not_resolve_ambiguous_selection(self):
        with self.assertRaisesRegex(ValueError, 'Ambiguous'):
            select_port(self.ports, serial_number='K1-123', occupied=['/dev/cu.usbserial-A'])

    def test_windows_dual_uart_selection(self):
        ports = [device('COM23', interface='USB Serial A'), device('COM24', interface='USB Serial B')]
        self.assertEqual(select_port(ports, serial_number='K1-123', interface='USB Serial B'), 'COM24')

    def test_usb_ids_accept_decimal_or_explicit_hex(self):
        ports = [device('COM24', vid=6790, pid=21970)]
        for vid, pid in [(6790, 21970), ('6790', '21970'), ('0x1A86', '0x55D2')]:
            self.assertEqual(select_port(ports, serial_number='K1-123', vid=vid, pid=pid), 'COM24')

    def test_all_usb_identity_fields_must_match(self):
        ports = [device('COM24', vid=6790, pid=21970)]
        for filters in [
            dict(serial_number='WRONG', vid=6790, pid=21970),
            dict(serial_number='K1-123', vid=1, pid=21970),
            dict(serial_number='K1-123', vid=6790, pid=1),
        ]:
            with self.assertRaisesRegex(ValueError, 'No serial port matches'):
                select_port(ports, **filters)

    def test_invalid_usb_ids_do_not_silently_disable_filter(self):
        for value in [-1, 65536, '0x10000', '1A86', 'not-an-id', True, 1.5]:
            with self.subTest(value=value), self.assertRaisesRegex(ValueError, 'USB VID/PID'):
                select_port([], port='COM24', vid=value)
        self.assertEqual(select_port([device('COM24', vid=0, pid=0)], vid=0, pid='0'), 'COM24')
        with self.assertRaisesRegex(ValueError, 'No serial port matches'):
            select_port([device('COM24')], vid=0)

    def test_k1_profile_selects_uart_b_after_port_and_usb_socket_change(self):
        for name, location in [('COM24', '1-1:x.2'), ('COM107', '3-4.5:x.2'), ('/dev/ttyACM9', '5-3.2:1.2')]:
            ports = k1_ports()
            ports[1].device = name
            ports[1].location = location
            for order in [ports, list(reversed(ports))]:
                self.assertEqual(
                    select_port(order, serial_number='5363000516', vid='0x1A86', pid='0x55D2', board='k1'), name
                )

    def test_k1_profile_can_select_by_serial_when_bridge_ids_are_detected(self):
        self.assertEqual(select_port(k1_ports(), serial_number='5363000516', board='k1'), 'COM24')

    def test_vid_pid_do_not_disambiguate_uart_without_board_profile(self):
        with self.assertRaisesRegex(ValueError, 'Ambiguous'):
            select_port(k1_ports(), serial_number='5363000516', vid=6790, pid=21970)

    def test_unknown_board_does_not_inherit_k1_uart_wiring(self):
        with self.assertRaisesRegex(ValueError, 'Ambiguous'):
            select_port(k1_ports(), serial_number='5363000516', board='other')

    def test_k1_does_not_guess_when_uart_b_or_interface_metadata_is_missing(self):
        ports = k1_ports()
        ports[1].location = None
        for candidates in [ports, ports[:1]]:
            with self.assertRaisesRegex(ValueError, 'No serial port matches.*USB interface 2'):
                select_port(candidates, serial_number='5363000516', board='k1')

    def test_duplicate_board_identity_is_still_ambiguous(self):
        ports = k1_ports() + [device('COM33', '5363000516', None, '4-1:x.2', vid=6790, pid=21970)]
        with self.assertRaisesRegex(ValueError, 'Ambiguous'):
            select_port(ports, serial_number='5363000516', board='k1')

    def test_k1_control_and_data_interfaces_do_not_resolve_duplicate_endpoints(self):
        ports = k1_ports() + [device('/dev/cu.duplicate', '5363000516', None, None, vid=6790, pid=21970)]
        ports[-1].interface_number = 3
        with self.assertRaisesRegex(ValueError, 'Ambiguous'):
            select_port(ports, serial_number='5363000516', board='k1')

    def test_k1_does_not_select_uart_a_data_interface(self):
        port = device('/dev/cu.A', '5363000516', None, None, vid=6790, pid=21970)
        port.interface_number = 1
        with self.assertRaisesRegex(ValueError, 'No serial port matches'):
            select_port([port], serial_number='5363000516', board='k1')

    def test_k1_profile_preserves_explicit_endpoint_override(self):
        self.assertEqual(
            select_port(k1_ports(), port='COM23', serial_number='5363000516', vid=6790, pid=21970, board='k1'), 'COM23'
        )

    def test_macos_registry_maps_same_location_uart_interfaces(self):
        registry = [
            {
                'idVendor': 6790,
                'idProduct': 21970,
                'IORegistryEntryChildren': [
                    {'bInterfaceNumber': 0, 'IORegistryEntryChildren': [{'IOCalloutDevice': '/dev/cu.A'}]},
                    {
                        'bInterfaceNumber': 2,
                        'IORegistryEntryChildren': [{'IORegistryEntryChildren': [{'IOCalloutDevice': '/dev/cu.B'}]}],
                    },
                ],
            }
        ]
        mapping = macos_interfaces(registry)
        self.assertEqual(mapping, {'/dev/cu.A': 0, '/dev/cu.B': 2})
        ports = [device(name, '5363000516', None, '20-1', vid=6790, pid=21970) for name in mapping]
        for port in ports:
            port.interface_number = mapping[port.device]
        self.assertEqual(select_port(ports, serial_number='5363000516', board='k1'), '/dev/cu.B')

    def test_macos_registry_does_not_apply_another_usb_devices_interface(self):
        registry = [
            {
                'bInterfaceNumber': 2,
                'IORegistryEntryChildren': [
                    {'idVendor': 1, 'idProduct': 1, 'IORegistryEntryChildren': [{'IOCalloutDevice': '/dev/cu.other'}]},
                ],
            }
        ]
        self.assertEqual(macos_interfaces(registry), {})

    def test_macos_nested_device_with_same_ids_starts_new_interface_namespace(self):
        for boundary in ({'IOObjectClass': 'IOUSBHostDevice'}, {'bDeviceClass': 0}):
            with self.subTest(boundary=boundary):
                registry = {
                    'idVendor': 6790,
                    'idProduct': 21970,
                    'bInterfaceNumber': 3,
                    'IORegistryEntryChildren': [
                        {
                            **boundary,
                            'idVendor': 6790,
                            'idProduct': 21970,
                            'IORegistryEntryChildren': [{'IOCalloutDevice': '/dev/cu.other'}],
                        }
                    ],
                }
                self.assertEqual(macos_interfaces(registry), {})


if __name__ == '__main__':
    unittest.main()
