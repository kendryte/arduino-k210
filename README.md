# Arduino core for K210

Arduino support for Kendryte K210 boards.

[![CI](https://github.com/kendryte/arduino-k210/actions/workflows/push.yml/badge.svg)](https://github.com/kendryte/arduino-k210/actions/workflows/push.yml) [![Docs](https://github.com/kendryte/arduino-k210/actions/workflows/docs.yml/badge.svg)](https://github.com/kendryte/arduino-k210/actions/workflows/docs.yml)

## Install

1. In Arduino IDE preferences, add the [stable package URL](https://raw.githubusercontent.com/kendryte/arduino-k210/gh-pages/package_k210_index.json) to **Additional Boards Manager URLs**.
2. Open **Boards Manager**, search for **K210**, and install the package by **Kendryte**.
3. Select your board and serial port, then upload an example.

For development builds, use the [development package URL](https://raw.githubusercontent.com/kendryte/arduino-k210/gh-pages/package_k210_dev_index.json) instead.

## Guides

- [Documentation and API reference](https://kendryte.github.io/arduino-k210/main/)
- [Add support for a board](variants/README.md)
- [CI, hardware tests, and releases](.github/CI.md)
