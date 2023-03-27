#!/bin/bash

ARDUINO_LIB_PATH="$ARDUINO_USR_PATH/libraries"

if [ ! -d ${ARDUINO_LIB_PATH} ]; then
    mkdir -p "${ARDUINO_LIB_PATH}"
fi

function install_lib(){ #install_lib <path_name> <git_url>
    local path=$1
    local url=$2

    if [ ! -d "${ARDUINO_LIB_PATH}/${path}" ]; then
        echo "Install ${path}"
        script_init_path="$PWD"

        cd "${ARDUINO_LIB_PATH}"
        for i in {1..5};
        do
            git clone ${url} --depth 1 ${path} > /dev/null 2>&1
            if [ $? -eq 0 ]; then
                break
            fi
            echo "Retry install..."
        done
        cd ${script_init_path}
    fi
    echo "lib ${path} installed"
}

install_lib "Adafruit_GFX_Library" "https://github.com/adafruit/Adafruit-GFX-Library.git"
install_lib "Adafruit_BusIO" "https://github.com/adafruit/Adafruit_BusIO.git"
install_lib "Adafruit_NeoPixel" "https://github.com/adafruit/Adafruit_NeoPixel.git"
