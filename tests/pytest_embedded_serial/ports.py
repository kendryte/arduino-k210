"""Select a configured serial endpoint without opening or probing any device."""

import argparse
import json
import os
import plistlib
import re
import subprocess
import sys

K1_BOARD_NAMES = ('k1', 'canaan_k1')
K1_USB_IDS = (0x1A86, 0x55D2)
# UART B's CDC control interface is 2; macOS exposes its data interface 3.
K1_UPLOAD_INTERFACES = (2, 3)


def usb_id(value):
    """Accept decimal USB IDs or explicit 0x-prefixed hexadecimal IDs."""
    if value is None or value == '':
        return None
    try:
        if isinstance(value, str):
            value = int(value, 16 if value.strip().lower().startswith('0x') else 10)
        if type(value) is int and 0 <= value <= 0xFFFF:
            return value
    except ValueError:
        pass
    raise ValueError('USB VID/PID must be an integer from 0 to 65535 (decimal or 0x-prefixed hex)')


def interface_number(port):
    number = getattr(port, 'interface_number', None)
    if type(number) is int and 0 <= number <= 255:
        return number
    # pySerial includes the USB interface in Linux/Windows location strings.
    location = getattr(port, 'location', None) or ''
    match = re.search(r':(?:x|\d+)\.(\d+)$', location)
    if match:
        return int(match.group(1))
    return None


def macos_interfaces(registry):
    """Map call-out paths to their nearest USB interface in an ioreg plist tree."""
    interfaces = {}

    def visit(node, number=None, usb_ids=None):
        # A full ioreg archive has a dictionary root; filtered archives may
        # contain an array. Children can likewise be a single dictionary.
        if isinstance(node, list):
            for child in node:
                visit(child, number, usb_ids)
            return
        if not isinstance(node, dict):
            raise ValueError('Unexpected ioreg registry node: expected a dictionary or list')
        # AppleUSBACMData repeats its parent's VID/PID without repeating the
        # interface number. Only a new USB device starts a new namespace.
        if 'idVendor' in node and 'idProduct' in node:
            node_ids = (node['idVendor'], node['idProduct'])
            node_class = node.get('IOObjectClass', node.get('IOClass'))
            if node_ids != usb_ids or node_class in ('IOUSBDevice', 'IOUSBHostDevice') or 'bDeviceClass' in node:
                number = None
            usb_ids = node_ids
        if 'bInterfaceNumber' in node:
            number = node['bInterfaceNumber']
        device = node.get('IOCalloutDevice')
        if device and type(number) is int and 0 <= number <= 255:
            interfaces[device] = number
        visit(node.get('IORegistryEntryChildren', []), number, usb_ids)

    visit(registry)
    return interfaces


def list_serial_ports():
    from serial.tools import list_ports

    ports = list_ports.comports()
    if sys.platform == 'darwin':
        # pySerial 3.5 may omit interface names and reports the same location for
        # both UARTs. Read their USB descriptor ancestry without opening a port.
        try:
            result = subprocess.run(
                ['/usr/sbin/ioreg', '-a', '-l', '-p', 'IOService'], check=True, capture_output=True, timeout=10
            )
            interfaces = macos_interfaces(plistlib.loads(result.stdout))
        except (OSError, subprocess.SubprocessError, ValueError) as error:
            print(f'USB interface metadata unavailable: {error}', file=sys.stderr)
        else:
            for port in ports:
                port.interface_number = interfaces.get(endpoint(port.device))
    return ports


def endpoint(device):
    # macOS call-in and call-out names refer to the same serial endpoint.
    if device.startswith('/dev/tty.'):
        return '/dev/cu.' + device[len('/dev/tty.') :]
    return device


def port_info(port):
    info = {
        name: getattr(port, name, None) for name in ('device', 'serial_number', 'location', 'interface', 'vid', 'pid')
    }
    info['interface_number'] = interface_number(port)
    return info


def select_port(
    ports, *, port=None, serial_number=None, vid=None, pid=None, location=None, interface=None, board=None, occupied=()
):
    """Require an explicit path or exactly one endpoint matching all USB filters.

    Serial numbers identify USB adapters, not necessarily their individual UARTs.
    The Canaan K1 profile selects its CH342 upload UART by USB interface number.
    Other ambiguous adapters need an interface, location, or explicit path.
    """
    filters = {
        name: value
        for name, value in dict(
            serial_number=serial_number, location=location, interface=interface, vid=usb_id(vid), pid=usb_id(pid)
        ).items()
        if value is not None and value != ''
    }
    if not port and not filters:
        raise ValueError(
            'Specify --port or USB identity filters (--port-serial-number, --port-vid, --port-pid); '
            'serial ports are never selected by enumeration order.'
        )

    if filters:
        ports = list(ports)
        matches = {}
        for candidate in ports:
            if port and endpoint(candidate.device) != endpoint(port):
                continue
            if not all(getattr(candidate, name, None) == value for name, value in filters.items()):
                continue
            key = endpoint(candidate.device)
            # Prefer /dev/cu.* when both macOS aliases are listed.
            if key not in matches or candidate.device.startswith('/dev/cu.'):
                matches[key] = candidate
        profile = None
        if (
            board in K1_BOARD_NAMES
            and not (port or location or interface)
            and matches
            and all((getattr(item, 'vid', None), getattr(item, 'pid', None)) == K1_USB_IDS for item in matches.values())
        ):
            # Canaan K1's CH342 UART B is its upload/console UART. Windows/Linux
            # report CDC control interface 2, while macOS reports data interface 3.
            # Do not infer this wiring for other boards that use the same bridge.
            profile = 'Canaan K1 CH342 UART B (USB interface 2 control or 3 data)'
            matches = {key: item for key, item in matches.items() if interface_number(item) in K1_UPLOAD_INTERFACES}
        if len(matches) != 1:
            reason = 'No serial port matches' if not matches else 'Ambiguous serial port selection'
            details = [port_info(item) for item in (matches.values() if matches else ports)]
            raise ValueError(
                f'{reason}: port={port!r}, filters={filters!r}, profile={profile!r}. '
                'Set --port or refine --port-interface/--port-location. '
                f'Ports: {json.dumps(details)}'
            )
        port = port or next(iter(matches.values())).device

    if endpoint(port) in {endpoint(device) for device in occupied}:
        raise ValueError(f'Serial port is already in use by another test: {port}')
    return port


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--list', action='store_true', help='List USB metadata without opening any port')
    parser.add_argument('--board', help='Board profile, e.g. k1 for Canaan K1 (inferred from FQBN in pytest)')
    parser.add_argument('--port', default=os.getenv('K210_PORT'))
    parser.add_argument('--port-serial-number', default=os.getenv('K210_PORT_SERIAL_NUMBER'))
    parser.add_argument('--port-vid', default=os.getenv('K210_PORT_VID'), help='Decimal or 0x-prefixed USB vendor ID')
    parser.add_argument('--port-pid', default=os.getenv('K210_PORT_PID'), help='Decimal or 0x-prefixed USB product ID')
    parser.add_argument('--port-location', default=os.getenv('K210_PORT_LOCATION'))
    parser.add_argument('--port-interface', default=os.getenv('K210_PORT_INTERFACE'))
    args = parser.parse_args()
    ports = list_serial_ports()
    if args.list:
        print(json.dumps([port_info(port) for port in sorted(ports, key=lambda item: item.device)], indent=2))
    else:
        try:
            print(
                select_port(
                    ports,
                    port=args.port,
                    serial_number=args.port_serial_number,
                    vid=args.port_vid,
                    pid=args.port_pid,
                    board=args.board,
                    location=args.port_location,
                    interface=args.port_interface,
                )
            )
        except ValueError as error:
            parser.error(str(error))


if __name__ == '__main__':
    main()
