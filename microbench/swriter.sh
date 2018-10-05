#!/bin/bash

rm /dev/shm/nvs_*
rm /dev/shm/unity_NVS_ROOT*
../nvstream/build/src/tools/nvsformat
echo "\n"
./build/seqwriter 1
