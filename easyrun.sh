#!/bin/bash

# xv6 디렉토리로 이동
cd xv6

make clean

# kernelmemfs 빌드
make kernelmemfs

# 상위 디렉토리로 이동
cd ..

# kernelmemfs를 image/kernel로 복사
cp xv6/kernelmemfs image/kernel

./run.sh