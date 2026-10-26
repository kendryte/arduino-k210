import logging
from typing import Optional

from pytest_embedded_serial_k210.serial import EspSerial

from .app import ArduinoApp


class ArduinoSerial(EspSerial):
    """Upload the compiled Arduino sketch before starting the serial console."""

    def __init__(self, app: ArduinoApp, target: Optional[str] = None, **kwargs) -> None:
        self.app = app
        super().__init__(
            target=target or self.app.target,
            port_board=self.app.target,
            **kwargs,
        )

    def _start(self):
        if self.skip_autoflash:
            logging.info('Skipping auto flash...')
            super()._start()
        else:
            self.flash()

    @EspSerial.use_esptool()
    def flash(self) -> None:
        """Upload with KFlash's built-in retries and propagate any failure."""
        logging.info('Flashing %s at %d baud', self.app.flash_files[0][1], self.flash_baud)
        self.kflash.process(
            terminal=False,
            noansi=True,
            dev=self.port,
            board='dan',
            baudrate=self.flash_baud,
            file=self.app.flash_files[0][1],
        )
