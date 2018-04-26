#!/bin/bash

rm /dev/shm/shm
rm /dev/shm/unity_NVS_ROOT*
../nvstream/build/src/tools/nvsformat
echo "\n"
./seqwriter 1
