import contextlib
import functools
import logging
from typing import Optional

from kflash import KFlash
from pytest_embedded.log import MessageQueue, PexpectProcess
from pytest_embedded.utils import Meta
from pytest_embedded_serial.serial import Serial


class EspSerial(Serial):
    """K210 serial transport; retain the legacy class name for plugin compatibility."""

    ESPTOOL_DEFAULT_BAUDRATE = 921600

    def __init__(
        self,
        pexpect_proc: PexpectProcess,
        msg_queue: MessageQueue,
        target: Optional[str] = None,
        beta_target: Optional[str] = None,
        port: Optional[str] = None,
        port_mac: str = None,
        baud: int = Serial.DEFAULT_BAUDRATE,
        esptool_baud: int = ESPTOOL_DEFAULT_BAUDRATE,
        skip_autoflash: bool = False,
        erase_all: bool = False,
        meta: Optional[Meta] = None,
        **kwargs,
    ) -> None:
        # Accept the shared plugin's ESP options without forwarding them to pySerial.
        self._meta = meta
        self.target = target
        self.skip_autoflash = skip_autoflash
        self.erase_all = False
        self.kflash = KFlash()
        super().__init__(msg_queue=msg_queue, port=port, baud=115200, meta=meta, **kwargs)

    def _post_init(self):
        if self._meta:
            self._meta.set_port_target_cache(self.port, self.target)
        if self.erase_all:
            self.erase_flash()
        super()._post_init()

    def use_esptool(hard_reset_after: bool = True, no_stub: bool = False):
        """Suspend serial redirection for KFlash, then restore the console settings.

        The decorator name and arguments are retained for plugin compatibility.
        KFlash handles the K210 boot sequence; this wrapper does not reset it.
        """

        def decorator(func):
            @functools.wraps(func)
            def wrapper(self, *args, **kwargs):
                with self.disable_redirect_thread(), contextlib.redirect_stdout(self._q):
                    settings = self.proc.get_settings()
                    try:
                        return func(self, *args, **kwargs)
                    finally:
                        self.proc.apply_settings(settings)

            return wrapper

        return decorator

    def _start(self):
        pass

    @use_esptool()
    def hard_reset(self):
        """Standalone reset is not implemented; KFlash controls reset during upload."""
        pass

    @use_esptool()
    def erase_flash(self):
        """Invalidate the app cache; standalone flash erasure is not implemented."""
        logging.debug('Invalidating the K210 app cache')
        if self._meta:
            self._meta.drop_port_app_cache(self.port)
