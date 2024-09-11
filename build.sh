#!/bin/bash

export PATH=/home/martin/android/prebuilts/clang/host/linux-x86/clang-r498229b/bin:$PATH
export CROSS_COMPILE=/home/martin/android/prebuilts/clang/host/linux-x86/clang-r498229b/bin/aarch64-linux-gnu-
export CC=/home/martin/android/prebuilts/clang/host/linux-x86/clang-r498229b/bin/clang
export CLANG_TRIPLE=aarch64-linux-gnu-
export ARCH=arm64
export ANDROID_MAJOR_VERSION=r

make -C $(pwd) O=$(pwd)/out KCFLAGS=-w CONFIG_SECTION_MISMATCH_WARN_ONLY=y LLVM=1 LLVM_IAS=1 a14xm_defconfig
make -C $(pwd) O=$(pwd)/out KCFLAGS=-w CONFIG_SECTION_MISMATCH_WARN_ONLY=y LLVM=1 LLVM_IAS=1 -j16

#cp out/arch/arm64/boot/Image $(pwd)/arch/arm64/boot/Image
