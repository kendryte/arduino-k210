"""Check compiler selection and setup on persistent macOS/Linux runners."""

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import ci_prepare


class PrepareTests(unittest.TestCase):
    def test_supported_hosts_have_pinned_archives(self):
        package = json.loads((ROOT / 'package/package_k210_index.template.json').read_text())
        compiler = next(
            tool for tool in package['packages'][0]['tools'] if tool['name'] == 'xpack-riscv-none-embed-gcc'
        )
        for system, machine, suffix in [
            ('Linux', 'x86_64', 'linux-x64.tar.gz'),
            ('Darwin', 'x86_64', 'darwin-x64.tar.gz'),
            ('Darwin', 'arm64', 'darwin-x64.tar.gz'),
        ]:
            with self.subTest(system=system, machine=machine):
                host = ci_prepare.compiler_host(system, machine)
                archive = next(item for item in compiler['systems'] if item['host'] == host)
                self.assertTrue(archive['archiveFileName'].endswith(suffix))

    def test_unsupported_cpu_does_not_download_wrong_compiler(self):
        with self.assertRaises(ValueError):
            ci_prepare.compiler_host('Linux', 'aarch64')

    def test_rosetta_is_checked_only_on_apple_silicon(self):
        with patch.object(ci_prepare.subprocess, 'run') as run:
            ci_prepare.check_rosetta('Linux', 'x86_64')
            ci_prepare.check_rosetta('Darwin', 'x86_64')
            run.assert_not_called()
            ci_prepare.check_rosetta('Darwin', 'arm64')
            self.assertEqual(run.call_args.args[0], ['/usr/bin/arch', '-x86_64', '/usr/bin/true'])

    def test_missing_rosetta_explains_runner_setup(self):
        with patch.object(ci_prepare.subprocess, 'run', side_effect=subprocess.CalledProcessError(1, 'arch')):
            with self.assertRaisesRegex(RuntimeError, 'softwareupdate --install-rosetta'):
                ci_prepare.check_rosetta('Darwin', 'arm64')

    def test_persistent_runner_relinks_a_moved_checkout(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            # macOS exposes /var through /private/var. Exercise a symlinked
            # parent on every platform so the path comparison stays portable.
            checkout_parent = root / 'checkouts'
            checkout_parent.mkdir()
            alias = root / 'checkout-alias'
            alias.symlink_to(checkout_parent, target_is_directory=True)
            source = alias / 'new-checkout'
            source.mkdir()
            hardware = root / 'hardware/Kendryte/K210'
            hardware.parent.mkdir(parents=True)
            hardware.symlink_to(root / 'missing-old-checkout', target_is_directory=True)
            ci_prepare.link_core(hardware, source)
            self.assertEqual(hardware.resolve(), source.resolve())
            ci_prepare.link_core(hardware, source)
            self.assertEqual(hardware.resolve(), source.resolve())

    def test_setup_keeps_existing_real_directories(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            hardware = root / 'K210'
            hardware.mkdir()
            with self.assertRaisesRegex(ValueError, 'Refusing to replace'):
                ci_prepare.link_core(hardware, root / 'checkout')
            self.assertTrue(hardware.is_dir())


if __name__ == '__main__':
    unittest.main()
