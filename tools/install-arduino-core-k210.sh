#!/bin/bash

export ARDUINO_K210_PATH="$ARDUINO_USR_PATH/hardware/Kendryte/K210"

if [ -L "$ARDUINO_K210_PATH" ]; then
    rm -r "$ARDUINO_K210_PATH"
fi

if [ ! -d "$ARDUINO_K210_PATH" ]; then
    echo "Installing K210 Arduino Core ..."
    script_init_path="$PWD"
    mkdir -p "$ARDUINO_USR_PATH/hardware/Kendryte"
    cd "$ARDUINO_USR_PATH/hardware/Kendryte"

    echo "Installing Python Serial ..."
    pip install pyserial > /dev/null

    if [ "$OS_IS_WINDOWS" == "1" ]; then
        echo "Installing Python Requests ..."
        pip install requests > /dev/null
    fi

    if [ ! -z "$GITHUB_REPOSITORY" ];  then
        echo "Linking Core..."
        ln -sf $GITHUB_WORKSPACE K210
    else
        echo "Cloning Core Repository..."
        git clone https://github.com/kendryte/arduino-k210.git K210 > /dev/null 2>&1
    fi

    # #echo "Updating Submodules ..."
    # cd K210
    # #git submodule update --init --recursive > /dev/null 2>&1

    # echo "Installing Platform Tools ..."
    # cd tools && python get.py
    cd $script_init_path

    echo "K210 Arduino has been installed in '$ARDUINO_K210_PATH'"
    echo ""
fi

ARDUINO_K210_TOOLS_PATH="$ARDUINO_USR_PATH/tools"
if [ ! -d "$ARDUINO_K210_TOOLS_PATH" ] || [ ! -d "${ARDUINO_K210_TOOLS_PATH}/xpack-riscv-none-embed-gcc" ] || [ ! -d "${ARDUINO_K210_TOOLS_PATH}/kflash_py" ]; then
    script_init_path="$PWD"

    echo "Download tools... to " ${ARDUINO_K210_TOOLS_PATH}
    mkdir -p "${ARDUINO_K210_TOOLS_PATH}" && cd "${ARDUINO_K210_TOOLS_PATH}"
    python ${ARDUINO_K210_PATH}/tools/get.py "${ARDUINO_K210_TOOLS_PATH}/place_holder"
    cd $script_init_path
fi

if [ -L "${ARDUINO_K210_PATH}/tools/kflash_py" ]; then
    rm -r "${ARDUINO_K210_PATH}/tools/kflash_py"
fi
ln -sf "${ARDUINO_K210_TOOLS_PATH}/kflash_py" "${ARDUINO_K210_PATH}/tools/kflash_py"

if [ -L "${ARDUINO_K210_PATH}/tools/xpack-riscv-none-embed-gcc" ]; then
    rm -r "${ARDUINO_K210_PATH}/tools/xpack-riscv-none-embed-gcc"
fi
ln -sf "${ARDUINO_K210_TOOLS_PATH}/xpack-riscv-none-embed-gcc" "${ARDUINO_K210_PATH}/tools/xpack-riscv-none-embed-gcc"
