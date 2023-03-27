#!/bin/bash

set -e

export ARDUINO_BUILD_DIR="$HOME/.arduino/build.tmp"

function build(){
    local board=$1
    local fqbn=$2
    local chunk_index=$3
    local chunks_cnt=$4
    local sketches=$5

    local BUILD_SKETCH="${SCRIPTS_DIR}/sketch_utils.sh build"
    local BUILD_SKETCHES="${SCRIPTS_DIR}/sketch_utils.sh chunk_build"

    local args="-ai $ARDUINO_IDE_PATH -au $ARDUINO_USR_PATH"

    args+=" -b $board -fqbn $fqbn"

    if [ "$OS_IS_LINUX" == "1" ]; then
        args+=" -p $ARDUINO_K210_PATH/libraries"
        args+=" -i $chunk_index -m $chunks_cnt"
        ${BUILD_SKETCHES} ${args}
    else
        for sketch in ${sketches}; do
            args+=" -s $(dirname $sketch)"
            if [ "$OS_IS_WINDOWS" == "1" ]; then
                local ctags_version=`ls "$ARDUINO_IDE_PATH/tools-builder/ctags/"`
                local preprocessor_version=`ls "$ARDUINO_IDE_PATH/tools-builder/arduino-preprocessor/"`
                win_opts="-prefs=runtime.tools.ctags.path=$ARDUINO_IDE_PATH/tools-builder/ctags/$ctags_version
                -prefs=runtime.tools.arduino-preprocessor.path=$ARDUINO_IDE_PATH/tools-builder/arduino-preprocessor/$preprocessor_version"
                args+=" ${win_opts}"
            fi
            ${BUILD_SKETCH} ${args}
        done
    fi
}

if [ -z "$GITHUB_WORKSPACE" ]; then
    export GITHUB_WORKSPACE="$PWD"
    export GITHUB_REPOSITORY="kendryte/arduino-k210"
fi

CHUNK_INDEX=$1
CHUNKS_CNT=$2
BUILD_PIO=0
if [ "$#" -lt 2 ] || [ "$CHUNKS_CNT" -le 0 ]; then
    CHUNK_INDEX=0
    CHUNKS_CNT=1
elif [ "$CHUNK_INDEX" -gt "$CHUNKS_CNT" ] &&  [ "$CHUNKS_CNT" -ge 2 ]; then
    CHUNK_INDEX=$CHUNKS_CNT
elif [ "$CHUNK_INDEX" -eq "$CHUNKS_CNT" ]; then
    BUILD_PIO=1
fi

SCRIPTS_DIR="./tools"
echo "Install arduino ide"
source ${SCRIPTS_DIR}/install-arduino-ide.sh

echo "Install arduino core and tools"
source ${SCRIPTS_DIR}/install-arduino-core-k210.sh

echo "Install arduino-k210 core depend libraries"
source ${SCRIPTS_DIR}/install-arduino-library.sh

FQBN_01STUDIO="Kendryte:K210:01studio_k210:clk_freq=400,burn_baudrate=1000000,enable_console=enable,stack_size=256K,enable_only_kmodel_v3=disable"
SKETCHES_01STUDIO="$ARDUINO_K210_PATH/libraries/K210_KPU/examples/kpu_face_recognization/kpu_face_recognization.ino"

build "01studio" $FQBN_01STUDIO $CHUNK_INDEX $CHUNKS_CNT $SKETCHES_01STUDIO
