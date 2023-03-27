import logging
from typing import Optional

from kflash import KFlash
from pytest_embedded_serial_k210.serial import EspSerial, EsptoolArgs

from .app import ArduinoApp


class ArduinoSerial(EspSerial):
    """
    Arduino serial Dut class

    Auto flash the app while starting test.
    """

    def __init__(
        self,
        app: ArduinoApp,
        target: Optional[str] = None,
        **kwargs,
    ) -> None:
        self.app = app
        super().__init__(
            target=target or self.app.target,
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
        """
        Flash the binary files to the board.
        """
        # flash_files = [
        #     (offset, open(path, 'rb')) for (offset, path, encrypted) in self.app.flash_files if not encrypted
        # ]

        # default_kwargs = {
        #     'addr_filename': flash_files,
        #     'encrypt_files': None,
        #     'no_stub': False,
        #     'compress': True,
        #     'verify': False,
        #     'ignore_flash_encryption_efuse_setting': False,
        #     'erase_all': False,
        #     'encrypt': False,
        #     'force': False,
        #     'chip': self.app.target,
        # }

        # default_kwargs.update(self.app.flash_settings)
        # flash_args = EsptoolArgs(**default_kwargs)

        for i in range(3):
            try:
                self.kflash.process(terminal=False, noansi = True, dev=self.port, board="dan", file=self.app.flash_files[0][1])
                break
                # self.stub.change_baud(self.esptool_baud)
                # esptool.detect_flash_size(self.stub, flash_args)
                # esptool.write_flash(self.stub, flash_args)
                # self.stub.change_baud(self.baud)
            except KFlash.TimeoutErr as e:
                logging.warning("Retry flash firmware.")
                continue
            except Exception:
                raise
            finally:
                pass
                # for (_, f) in flash_files:
                #     f.close()
