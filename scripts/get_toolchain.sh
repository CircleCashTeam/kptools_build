#!/usr/bin/bash

FETCH_URL="https://github.com/open-vela/prebuilts_gcc_linux-x86_64_aarch64-none-elf"

if [ ! -f "`dirname $0`/../toolchain/bin/aarch64-none-elf-gcc" ]; then
    echo "- Fetching aarch64 toolchain ..."
    git clone $FETCH_URL `dirname $0`/../toolchain --depth=1
else
    echo "- Already fetched aarch64 toolchain"
fi
