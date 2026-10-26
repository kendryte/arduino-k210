# Hardware testcase coverage

These sketches run on a K210 target through `pytest-embedded`. Compile coverage
is reported separately by `tests/build_compat.py`; this table records runtime
behavior exercised by the hardware suite.

## Selecting the hardware test port

The Actions workflow compiles these sketches on Ubuntu and uploads the binaries
as `k210-test-firmware`. After the build passes, trusted `main` runs download that
artifact on the Mac and use pytest to flash and execute these testcases on the
configured Canaan K1. PRs and tags only run checks on Ubuntu. The selector reads
USB metadata; the subsequent pytest step uploads firmware and checks board output.
Reports and serial logs are saved as `k210-onboard-results`, including on failure.
For repository variables and runner setup, see
[CI board detection](../../.github/CI.md#board-detection).

On the Mac, list paths and exact USB metadata without opening any port (requires
`pyserial` in your Python environment):

```sh
python3 tools/select_serial_port.py --list
```

For a Canaan K1 with the CH342 adapter, configure its USB serial number, VID, and
PID. Paths and USB socket locations may change; these identity fields remain the
selection criteria. Decimal IDs and `0x`-prefixed hexadecimal IDs are accepted:

```sh
export K210_PORT_SERIAL_NUMBER='your-board-usb-serial-number'
export K210_PORT_VID='0x1A86'
export K210_PORT_PID='0x55D2'
python3 tools/select_serial_port.py --board k1
```

Both CH342 UARTs share these identity fields. The `k1` profile selects its
upload/console UART B by USB interface **2** (CDC control) or **3** (CDC data),
independent of the port path or USB socket. Windows/Linux report the control
interface; macOS exposes the data interface. Pytest infers the profile from the
sketch's `k1` FQBN; the standalone tool
uses `--board k1`. On macOS, `ioreg` supplies interface metadata missing from
pySerial. Check `interface_number` in `--list` to see the mapping. This wiring rule
applies only to Canaan K1 with VID/PID `1A86:55D2`.

These settings are read by the selector CLI, pytest, and both `tools/tests_run*.sh`
scripts. CLI options `--port-serial-number`, `--port-vid`, and `--port-pid` override
their corresponding environment values. All configured filters must match.
Unplugged boards, incorrect IDs, duplicate matches, or unavailable K1 UART B
metadata cause an error before opening or flashing any port.

For other boards or drivers without interface numbers, these optional selectors
override the K1 UART profile while still checking the configured USB identity:

| Environment variable | CLI option | Value |
| --- | --- | --- |
| `K210_PORT_INTERFACE` | `--port-interface` | Exact USB interface description |
| `K210_PORT_LOCATION` | `--port-location` | Unique USB location from `--list` |
| `K210_PORT` | `--port` | Explicit path, such as `/dev/cu.usbserial-XXXXX` or `COM24` |

Clear unused identity variables when switching boards: all configured filters
must agree with the selected port. Keep these overrides unset for automatic K1
selection.

For a single hardware test, after configuring the identity variables above,
building its sketch, and installing the hardware test dependencies, run from the
repository root:

```sh
PYTHONPATH=tests python3 -m pytest tests/testcases/core_compat \
  --build-dir /absolute/path/to/core_compat/build
```

The hardware runner flashes the selected board by default; use
`--skip-autoflash yes` only when the matching test firmware is already loaded.
USB identity selects the adapter/UART, not the chip connected behind it. Verify
the adapter's UART wiring when first configuring the board. Metadata availability
depends on the driver; see [pySerial's USB port metadata](https://pyserial.readthedocs.io/en/latest/tools.html#serial-tools-list-ports).

## Bundled libraries

| Library | Runtime testcase | Coverage |
| --- | --- | --- |
| FFat | `ffat` | Mount, directory, file, rename, delete, and I/O |
| FS | `ffat`, `spiffs` | Shared filesystem API through both backends |
| GC0328 | `camera` | Identification, controls, buffers, and snapshots |
| I2S | `i2s`, `library_algorithms` | Validation, ownership, TX DMA, and buffering |
| Image | `image` | Conversion, crop, grayscale, and imlib adapter |
| K210_8080 | `display` | ST7789 command, data, and refresh path |
| K210_AES | `aes` | AES-128 ECB encryption and decryption |
| K210_CAMERA | `camera`, `camera_ov2640` | DVP buffers and capture path |
| K210_KPU | `kpu`, `library_algorithms` | Model execution and activation/post-processing logic |
| K210_SHA256 | `sha256` | Hardware SHA-256 result |
| NoePixelWrap | `neopixel` | Color buffer and K210 signal output |
| OV2640 | `camera_ov2640` | Identification and snapshot when installed |
| SPI | `spi_loop`, `spi_transactions` | Byte/word/buffer loopback and setter locking across tasks |
| SPIFFS | `spiffs` | Mount, directory, file, rename, delete, and I/O |
| ST7789V | `display` | Geometry, framebuffer, bitmap, and refresh |
| ScopedTimer | `library_algorithms`, `core_compat` | Monotonic timing and formatting |
| Ticker | `ticker` | One-shot, repeating, typed callback, and rejection paths |
| TouchScreen | `touchscreen`, `library_algorithms` | Events, filters, calibration, and NS2009 detection |
| Wire | `wire` | Validation, ownership, bounds, and invalid requests |
| imlib | `image`, `library_algorithms` | Image adapter and geometry helpers |

## Core and drivers

| Area | Runtime testcase |
| --- | --- |
| Arduino compatibility, strings, time, math | `core_compat` |
| GPIO and GPIOHS interrupts | `digital_io_loop` |
| UART | `uart_loop` |
| SPI | `spi_loop` |
| I2C | `wire`, camera and touchscreen suites |
| I2S and DMA | `i2s`, `dma` |
| DVP camera | `camera`, `camera_ov2640` |
| TFT/8080 bus | `display` |
| Flash filesystems | `ffat`, `spiffs` |
| AES, SHA-256, and KPU | `aes`, `sha256`, `kpu` |

## Hardware conditions

- `digital_io_loop`, `uart_loop`, and `spi_loop` require GPIO 34 connected to GPIO 35.
- `spi_transactions` checks setter locking with two tasks and needs no loopback wiring.
- `ffat` requires an SD card with a mountable FAT filesystem.
- I2S transmit is automated. Receive is ignored until an external I2S source is connected.
- NS2009 access is ignored when the controller is not detected on GPIO 30/31.
- OV2640 capture is ignored when that sensor is not installed. GC0328 is tested separately.
- NeoPixel data is emitted on GPIO 18; buffer behavior is asserted, but electrical timing needs a logic analyzer or a connected pixel for independent verification.
- ST7789 framebuffer and bus calls are asserted; visible panel output has no readback channel.
- The KPU testcase provisions the models stored at flash offsets 4 MiB and 5 MiB into SPIFFS, then loads and runs both through the filesystem API.
- Filesystem tests create, rename, and delete test files. SPIFFS may format its partition if mounting fails; use a dedicated test board.
