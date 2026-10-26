"""Exercise KFlash's byte protocol without accessing a physical serial port."""

import binascii
import hashlib
import io
import struct
import sys
from collections import deque
from types import SimpleNamespace

import pytest
from kflash import kflash


class Clock:
    def __init__(self):
        self.now = 0.0

    def time(self):
        return self.now

    def monotonic(self):
        return self.now

    def sleep(self, duration):
        self.now += duration


class BootloaderPort:
    def __init__(self, clock):
        self.clock = clock
        self.baudrate = 115200
        self.is_open = True
        self.writes = []
        self.requests = []
        self.memory = {}
        self.flash = {}
        self.flash_requests = []
        self.flash_replies = deque()
        self.default_flash_reply = b'\xd4\xe0'
        self.command_replies = {}
        self.max_flash_requests = None
        self.replies = deque()
        self.default_reply = b'\xc3\xe0'
        self.read_buffer = bytearray()
        self.packet = bytearray()
        self.max_write = None
        self.usb_limit = None

    def write(self, data):
        assert self.is_open
        if self.usb_limit is not None:
            assert len(data) <= self.usb_limit, 'USB bridge transmit buffer overrun'
        written = len(data) if self.max_write is None else min(self.max_write, len(data))
        data = data[:written]
        self.writes.append((self.clock.time(), bytes(data), self.baudrate))
        for byte in data:
            if byte != 0xC0:
                self.packet.append(byte)
            elif self.packet:
                packet = bytes(self.packet).replace(b'\xdb\xdc', b'\xc0').replace(b'\xdb\xdd', b'\xdb')
                self.packet.clear()
                self.handle_packet(packet)
        return written

    def handle_packet(self, packet):
        opcode = packet[0]
        if opcode in (0xC3, 0xD4):
            checksum, address, size = struct.unpack_from('<III', packet, 4)
            payload = packet[16:]
            assert size == len(payload)
            assert checksum == binascii.crc32(packet[8:]) & 0xFFFFFFFF
            if opcode == 0xC3:
                self.requests.append((address, payload))
                reply = self.replies.popleft() if self.replies else self.default_reply
                if reply == b'\xc3\xe0':
                    self.memory[address] = payload
            else:
                self.flash_requests.append((address, payload))
                if self.max_flash_requests is not None:
                    assert len(self.flash_requests) <= self.max_flash_requests, 'Unbounded flash retries'
                reply = self.flash_replies.popleft() if self.flash_replies else self.default_flash_reply
                if reply == b'\xd4\xe0':
                    self.flash[address] = payload
        elif opcode == 0xC5:
            return  # RAM handoff has no response.
        else:
            assert opcode in (0xC2, 0xD2, 0xD5, 0xD6, 0xD7)
            replies = self.command_replies.get(opcode)
            reply = replies.popleft() if replies else bytes((opcode, 0xE0))
        if reply is not None:
            self.read_buffer.extend(
                b'\xc0' + reply.replace(b'\xdb', b'\xdb\xdd').replace(b'\xc0', b'\xdb\xdc') + b'\xc0'
            )

    def read(self, size):
        if not self.read_buffer:
            self.clock.sleep(0.1)
        data = bytes(self.read_buffer[:size])
        del self.read_buffer[:size]
        return data

    def isOpen(self):
        return self.is_open

    def open(self):
        self.is_open = True

    def close(self):
        self.is_open = False

    def reset_input_buffer(self):
        self.read_buffer.clear()

    flushInput = reset_input_buffer

    def flush(self):
        pass

    flushOutput = flush

    def setDTR(self, value):
        pass

    def setRTS(self, value):
        pass


@pytest.fixture
def uploader(monkeypatch):
    clock = Clock()
    port = BootloaderPort(clock)
    monkeypatch.setattr(kflash, 'time', clock)
    monkeypatch.setattr(
        kflash, 'sys', SimpleNamespace(platform='darwin', version_info=sys.version_info, stdout=io.StringIO())
    )
    monkeypatch.setattr('serial.Serial', lambda **kwargs: port)
    # An unrelated CH342 must not enable pacing for the selected port.
    metadata = SimpleNamespace(device='/dev/cu.K1-B', vid=0x1A86, pid=0x55D2)
    monkeypatch.setattr('serial.tools.list_ports.comports', lambda: [metadata])
    return kflash.KFlash(), port, metadata


def initialize_loader(uploader, tmp_path):
    flash, port, _ = uploader
    # Instantiate the real nested loader without beginning an upload.
    with pytest.raises(Exception, match='Unable to find the firmware'):
        flash.process(terminal=False, dev='/dev/cu.K1-B', board='dan', file=str(tmp_path / 'missing.bin'))
    port.open()
    return flash.loader


@pytest.mark.parametrize('reply', [b'\xc3\xe2', b'\xc3\xe3', b'\x7a\xe0', b'\xc3\x00', b'', None])
def test_isp_retry_preserves_address_and_payload(uploader, tmp_path, reply):
    loader = initialize_loader(uploader, tmp_path)
    port = uploader[1]
    port.replies.append(reply)
    firmware = bytes(range(256)) * 9
    loader.flash_dataframe(firmware)
    assert port.requests[0] == port.requests[1]
    assert [address for address, _ in port.requests] == [0x80000000, 0x80000000, 0x80000400, 0x80000800]
    assert b''.join(port.memory[address] for address in sorted(port.memory)) == firmware


@pytest.mark.parametrize('reply', [b'\xc3\xe2', b'\x7a\xe0', None])
def test_isp_persistent_failure_is_bounded(uploader, tmp_path, reply):
    loader = initialize_loader(uploader, tmp_path)
    port = uploader[1]
    port.default_reply = reply
    with pytest.raises(RuntimeError, match='ISP RAM upload failed at 0x80000000 after 10 attempts'):
        loader.flash_dataframe(b'firmware')
    assert len(port.requests) == 10
    assert len(set(port.requests)) == 1
    assert not port.is_open
    assert not port.memory


@pytest.mark.parametrize('partial_write', [None, 7])
def test_macos_ch342_writes_are_paced_and_preserve_slip_bytes(uploader, tmp_path, partial_write):
    loader = initialize_loader(uploader, tmp_path)
    port = uploader[1]
    port.usb_limit = 64
    port.max_write = partial_write
    firmware = bytes(range(256)) * 5
    loader.flash_dataframe(firmware)
    assert b''.join(port.memory[address] for address in sorted(port.memory)) == firmware
    assert len(port.writes) > 2
    for (timestamp, data, baud), (next_timestamp, _, _) in zip(port.writes, port.writes[1:]):
        assert next_timestamp - timestamp >= len(data) * 10 / baud - 1e-9


@pytest.mark.parametrize(
    'platform, selected_device, vid, pid',
    [
        ('linux', '/dev/cu.K1-B', 0x1A86, 0x55D2),
        ('win32', '/dev/cu.K1-B', 0x1A86, 0x55D2),
        ('darwin', '/dev/cu.OTHER', 0x1A86, 0x55D2),
        ('darwin', '/dev/cu.K1-B', 0x1A86, 0x7523),
        ('darwin', '/dev/cu.K1-B', 0x0403, 0x55D2),
    ],
)
def test_other_adapters_and_hosts_keep_normal_writes(uploader, tmp_path, platform, selected_device, vid, pid):
    kflash.sys.platform = platform
    uploader[2].device, uploader[2].vid, uploader[2].pid = selected_device, vid, pid
    loader = initialize_loader(uploader, tmp_path)
    loader.flash_dataframe(bytes(range(256)))
    assert len(uploader[1].writes) == 1


def test_macos_tty_alias_uses_ch342_pacing(uploader, tmp_path):
    uploader[2].device = '/dev/tty.K1-B'
    loader = initialize_loader(uploader, tmp_path)
    loader.flash_dataframe(bytes(range(256)))
    assert len(uploader[1].writes) > 1


def test_zero_length_write_fails_instead_of_looping(uploader, tmp_path):
    loader = initialize_loader(uploader, tmp_path)
    uploader[1].max_write = 0
    with pytest.raises(IOError, match='Serial write made no progress'):
        loader.flash_dataframe(b'firmware')
    assert not uploader[1].is_open


@pytest.mark.parametrize('baud', [115200, 921600])
def test_full_flash_process_with_paced_ch342_transport(uploader, tmp_path, baud):
    flash, port, _ = uploader
    port.usb_limit = 64
    port.replies.append(b'\xc3\xe2')  # Reject the first ISP packet once.
    firmware = bytes(range(256)) * 16
    binary = tmp_path / 'firmware.bin'
    binary.write_bytes(firmware)
    flash.process(terminal=False, dev='/dev/cu.K1-B', board='dan', baudrate=baud, file=str(binary), noansi=True)
    assert port.requests[0] == port.requests[1]
    programmed = b''.join(port.flash[address] for address in sorted(port.flash))
    image = b'\x02' + struct.pack('<I', len(firmware)) + firmware  # DIO flash header
    assert programmed == image + hashlib.sha256(image).digest()
    assert port.baudrate == baud
    assert not port.is_open


@pytest.mark.parametrize('reply', [b'\xd4\xe2', b'\xd4\xe7', b'\xd4\xe6', b'\x7a\xe0', b'\xd4\x00', b''])
def test_flash_retry_preserves_address_and_payload(uploader, tmp_path, reply):
    loader = initialize_loader(uploader, tmp_path)
    port = uploader[1]
    port.flash_replies.append(reply)
    firmware = bytes(range(256)) * 9
    loader.dump_to_flash(firmware, address=0x10000, size=1024)
    assert port.flash_requests[0] == port.flash_requests[1]
    assert [address for address, _ in port.flash_requests] == [0x10000, 0x10000, 0x10400, 0x10800]
    assert b''.join(port.flash[address] for address in sorted(port.flash)) == firmware


@pytest.mark.parametrize('reply', [b'\xd4\xe2', b'\xd4\xe6', b'\x7a\xe0', b'\xd4\x00', b''])
def test_flash_persistent_rejection_is_bounded(uploader, tmp_path, reply):
    loader = initialize_loader(uploader, tmp_path)
    port = uploader[1]
    port.default_flash_reply = reply
    port.max_flash_requests = 11
    with pytest.raises(RuntimeError, match='Flash write at 0x00010000 failed after 10 attempts at 115200 baud'):
        loader.dump_to_flash(b'firmware', address=0x10000)
    assert len(port.flash_requests) == 10
    assert len(set(port.flash_requests)) == 1
    assert not port.flash
    assert not port.is_open


@pytest.mark.parametrize('reply', [None, b'\xd4\xe7'])
def test_flash_timeout_and_persistent_busy_have_a_deadline(uploader, tmp_path, reply):
    loader = initialize_loader(uploader, tmp_path)
    port = uploader[1]
    port.default_flash_reply = reply
    port.max_flash_requests = 200 if reply else 2
    start = port.clock.monotonic()
    with pytest.raises(RuntimeError, match='Flash write at 0x00000000 failed'):
        loader.dump_to_flash(b'firmware')
    elapsed = port.clock.monotonic() - start
    assert 90 <= elapsed < 91
    assert len(set(port.flash_requests)) == 1
    assert not port.flash
    assert not port.is_open


def test_flash_init_recovers_after_a_response_timeout(uploader, tmp_path):
    loader = initialize_loader(uploader, tmp_path)
    port = uploader[1]
    port.command_replies[0xD7] = deque([None])
    loader.init_flash(1)
    assert port.is_open
    assert len(port.writes) == 2


def test_flash_init_failure_reports_the_selected_baud(uploader, tmp_path):
    loader = initialize_loader(uploader, tmp_path)
    port = uploader[1]
    port.baudrate = 921600
    port.command_replies[0xD7] = deque([None] * 10)
    with pytest.raises(RuntimeError, match='Flash initialization failed after 10 attempts at 921600 baud'):
        loader.init_flash(1)
    assert not port.is_open
