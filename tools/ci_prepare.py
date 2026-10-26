#!/usr/bin/env python3
"""Prepare a Linux/macOS Arduino build with the compiler pinned in the package index."""

import argparse
import hashlib
import json
import os
import platform
import subprocess
import tarfile
from pathlib import Path
from urllib.request import urlretrieve

ROOT = Path(__file__).resolve().parents[1]


def compiler_host(system, machine):
    if system == 'Darwin' and machine.lower() in {'x86_64', 'amd64', 'arm64', 'aarch64'}:
        # The pinned compiler predates native Apple Silicon builds.
        return 'x86_64-apple-darwin'
    if system == 'Linux' and machine.lower() in {'x86_64', 'amd64'}:
        return 'x86_64-linux-gnu'
    raise ValueError(f'No pinned K210 compiler for {system}/{machine}')


def check_rosetta(system, machine):
    if system == 'Darwin' and machine.lower() in {'arm64', 'aarch64'}:
        try:
            subprocess.run(
                ['/usr/bin/arch', '-x86_64', '/usr/bin/true'],
                check=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
            )
        except (OSError, subprocess.CalledProcessError) as error:
            raise RuntimeError(
                'The K210 compiler requires Rosetta on Apple Silicon. '
                'Install it on the runner with: softwareupdate --install-rosetta'
            ) from error


def link_core(hardware, source):
    # A persistent runner may have moved its checkout since the previous job.
    if hardware.is_symlink():
        if hardware.resolve() == source.resolve():
            return
        hardware.unlink()
    elif hardware.exists():
        raise ValueError(f'Refusing to replace an existing hardware directory: {hardware}')
    hardware.parent.mkdir(parents=True, exist_ok=True)
    hardware.symlink_to(source, target_is_directory=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    directory = args.directory.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    host = compiler_host(platform.system(), platform.machine())
    check_rosetta(platform.system(), platform.machine())
    tools = json.loads((ROOT / 'package/package_k210_index.template.json').read_text())['packages'][0]['tools']
    compiler = next(tool for tool in tools if tool['name'] == 'xpack-riscv-none-embed-gcc')
    system = next(system for system in compiler['systems'] if system['host'] == host)
    toolchain = directory / 'tools' / f'{compiler["name"]}-{compiler["version"]}'
    if not (toolchain / 'bin/riscv-none-embed-g++').is_file():
        archive = directory / system['archiveFileName']
        urlretrieve(system['url'], archive)
        with archive.open('rb') as stream:
            checksum = 'SHA-256:' + hashlib.file_digest(stream, 'sha256').hexdigest()
        if checksum != system['checksum']:
            raise ValueError('Toolchain checksum mismatch')
        with tarfile.open(archive) as bundle:
            bundle.extractall(directory / 'tools', filter='data')
        archive.unlink()
    subprocess.run([str(toolchain / 'bin/riscv-none-embed-g++'), '--version'], check=True)
    user = directory / 'sketchbook'
    hardware = user / 'hardware/Kendryte/K210'
    link_core(hardware, ROOT)
    config = directory / 'arduino-cli.yaml'
    config.write_text(
        json.dumps(
            {
                'directories': {
                    'data': str(directory / 'data'),
                    'downloads': str(directory / 'downloads'),
                    'user': str(user),
                }
            }
        )
    )

    def cli(*args):
        subprocess.run(['arduino-cli', '--config-file', str(config), *args], check=True)

    cli('core', 'update-index')
    cli('lib', 'install', 'Adafruit GFX Library@1.13.0', 'Adafruit BusIO@1.17.4', 'Adafruit NeoPixel@1.15.5')
    if 'GITHUB_ENV' in os.environ:
        with open(os.environ['GITHUB_ENV'], 'a') as stream:
            stream.write(f'ARDUINO_CONFIG={config}\nK210_TOOLCHAIN={toolchain}\n')
    print(f'Config: {config}\nToolchain: {toolchain}')


if __name__ == '__main__':
    main()
