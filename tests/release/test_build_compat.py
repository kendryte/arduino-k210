"""Check compile coverage when Arduino CLI runs from a sketch directory."""

import contextlib
import io
import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import build_compat


class BuildCoverageTests(unittest.TestCase):
    def test_relative_and_absolute_outputs_collect_core_and_transitive_libraries(self):
        for relative_output in (True, False):
            with self.subTest(relative_output=relative_output), tempfile.TemporaryDirectory() as directory:
                root = Path(directory).resolve()
                sketch = root / 'tests/testcases/camera'
                sources = {
                    'tests/testcases/camera/camera.ino': '#include <Image.h>\n',
                    'libraries/Image/library.properties': 'name=Image\n',
                    'libraries/Image/src/Image.h': '#include <Camera.h>\n',
                    'libraries/Camera/library.properties': 'name=Camera\n',
                    'libraries/Camera/src/Camera.h': '',
                    'cores/k210/Main.cpp': '',
                    'cores/k210/k210-sdk/drivers/k210_dvp.c': '',
                    'cores/k210/k210-hal/k210-hal-gpio.c': '',
                }
                for name, contents in sources.items():
                    path = root / name
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text(contents)

                def compile_sketch(command, *, cwd, **kwargs):
                    self.assertEqual(cwd, sketch)
                    # Relative build paths are interpreted by the child process
                    # from the sketch's directory, just as Arduino CLI does.
                    build = cwd / command[command.index('--build-path') + 1]
                    for name in (
                        'core/Main.cpp.o',
                        'core/k210-sdk/drivers/k210_dvp.c.o',
                        'core/k210-hal/k210-hal-gpio.c.o',
                        'libraries/Image/Image.cpp.o',
                        'libraries/Camera/Camera.cpp.o',
                    ):
                        obj = build / name
                        obj.parent.mkdir(parents=True, exist_ok=True)
                        obj.touch()
                    (build / 'camera.ino.bin').write_bytes(b'test firmware')
                    (build / 'build.options.json').write_text(
                        json.dumps({'fqbn': 'Kendryte:K210:k1', 'sketchLocation': str(sketch)})
                    )
                    return subprocess.CompletedProcess(command, 0)

                output = Path('build/compat') if relative_output else root / 'build/compat'
                argv = ['build_compat.py', '--toolchain', str(root / 'compiler'), '--output', str(output)]
                with (
                    contextlib.chdir(root),
                    contextlib.redirect_stdout(io.StringIO()),
                    patch.object(build_compat, 'ROOT', root),
                    patch.object(sys, 'argv', argv),
                    patch.object(build_compat.subprocess, 'run', side_effect=compile_sketch),
                ):
                    self.assertEqual(build_compat.main(), 0)

                coverage = json.loads((root / 'build/compat/coverage.json').read_text())
                self.assertEqual(coverage['sketches']['passed'], 1)
                self.assertTrue(all(library['covered'] for library in coverage['libraries']))
                for area in ('core', 'sdk_drivers', 'hal_drivers'):
                    self.assertEqual(coverage[area]['total'], 1)
                    self.assertEqual(len(coverage[area]['covered']), 1)
                    self.assertEqual(coverage[area]['missing'], [])
                self.assertFalse((sketch / 'build').exists())

                # Simulate downloading just the artifact into a fresh checkout
                # on a different host, without the compiler's original paths.
                download = root / 'mac-checkout/tests/testcases'
                shutil.copytree(root / 'build/compat/test-firmware', download)
                shutil.rmtree(root / 'build')
                self.assertEqual((download / 'camera/build/camera.ino.bin').read_bytes(), b'test firmware')
                self.assertEqual(
                    json.loads((download / 'camera/build/build.options.json').read_text())['fqbn'], 'Kendryte:K210:k1'
                )
                self.assertEqual(
                    sorted(path.name for path in (download / 'camera/build').iterdir()),
                    ['build.options.json', 'camera.ino.bin'],
                )

    def test_missing_test_binary_fails_export(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with patch.object(build_compat, 'ROOT', root), self.assertRaises(FileNotFoundError):
                build_compat.export_test_firmware(root / 'tests/testcases/aes', root / 'build', root / 'output')

    def test_library_examples_are_not_in_the_hardware_artifact(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with patch.object(build_compat, 'ROOT', root):
                build_compat.export_test_firmware(
                    root / 'libraries/SPI/examples/loopback', root / 'build', root / 'output'
                )
            self.assertFalse((root / 'output').exists())


if __name__ == '__main__':
    unittest.main()
