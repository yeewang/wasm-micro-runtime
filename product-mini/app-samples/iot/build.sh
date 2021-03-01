# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

WAMR_DIR=${PWD}/../../..

echo "Build wasm app .."
${WASI_SDK_DIR}/bin/clang -O3 \
        -z stack-size=4096 -Wl,--initial-memory=65536 \
        -o iot_light.wasm main.c \
        -Wl,--export=main -Wl,--export=__main_argc_argv \
        -Wl,--export=__data_end -Wl,--export=__heap_base \
        -Wl,--strip-all,--no-entry \
        -Wl,--allow-undefined \
        -nostdlib \

echo "Build binarydump tool .."
rm -fr build && mkdir build && cd build
cmake ../../../../test-tools/binarydump-tool
make
cd ..

echo "Generate iot_light_wasm.h .."
./build/binarydump -o iot_light_wasm.h -n wasm_test_file iot_light.wasm

echo "Done"
