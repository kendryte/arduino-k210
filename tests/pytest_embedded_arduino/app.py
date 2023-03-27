import json
import os
from typing import List, Tuple

from pytest_embedded.app import App


class ArduinoApp(App):
    """
    Arduino App class

    Attributes:
        sketch (str): Sketch name.
        fqbn (str): Fully Qualified Board Name.
        target (str) : ESPxx chip.
        flash_files (Tuple): [int, str, str]: (offset, file path, encrypted) files need to be flashed in.
    """

    def __init__(
        self,
        **kwargs,
    ):
        super().__init__(**kwargs)

        self.sketch = os.path.basename(self.app_path)
        self.fqbn = self._get_fqbn(self.binary_path)
        self.target = self.fqbn.split(':')[2]
        self.flash_files = self._get_bin_files(self.binary_path, self.sketch)

    def _get_fqbn(self, build_path) -> str:
        options_file = os.path.realpath(os.path.join(build_path, 'build.options.json'))
        with open(options_file) as f:
            options = json.load(f)
        fqbn = options['fqbn']
        return fqbn

    def _get_bin_files(self, build_path, sketch) -> List[Tuple[int, str, bool]]:
        app = os.path.realpath(os.path.join(build_path, sketch + '.ino.bin'))
        return [(0, app, False)]
