#!/bin/bash

function get_file_size(){
    local file="$1"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        eval `stat -s "$file"`
        local res="$?"
        echo "$st_size"
        return $res
    else
        stat --printf="%s" "$file"
        return $?
    fi
}

echo "Creating ZIP ..."
pushd "../../" >/dev/null

ZIP=arduino-k210.zip
DIR=arduino-k210

rm -rf "$ZIP"
zip -qr "$ZIP" "$DIR" -x "${DIR}/.git**"

if [ $? -ne 0 ]; then echo "ERROR: Failed to create $ZIP ($?)"; exit 1; fi

SHA=`shasum -a 256 "$ZIP" | cut -f 1 -d ' '`
SIZE=`get_file_size "$ZIP"`

# Construct JQ argument with package data
jq_arg=".packages[0].platforms[0].size = \"$SIZE\" |\
    .packages[0].platforms[0].checksum = \"SHA-256:$SHA\""

JSON_TEMPLATE="${DIR}/package/package_k210_index.template.json"
JSON_REL="${DIR}/package/package_k210_index.json"
cat "$JSON_TEMPLATE" | jq "$jq_arg" > "$JSON_REL"
