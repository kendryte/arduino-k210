#!/usr/bin/env python3
"""List or select a hardware test UART without opening any serial ports."""

import runpy
from pathlib import Path

if __name__ == '__main__':
    # Keep the adjacent pytest_embedded_serial/serial.py from shadowing pySerial.
    runpy.run_path(
        str(Path(__file__).resolve().parents[1] / 'tests/pytest_embedded_serial/ports.py'), run_name='__main__'
    )
