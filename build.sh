#!/bin/sh

CPU_COUNT=`cat /proc/cpuinfo | awk '/processor/{print $2}' | wc -l`
BUILD_CPU_COUNT=`expr $CPU_COUNT / 2`

ARCH_PLATFORM=arm
CROSS_TOOLCHAIN=arm-none-eabi-

do_build() {
	case $1 in
		*config) make ARCH=$ARCH_PLATFORM CROSS_COMPILE=$CROSS_TOOLCHAIN menuconfig;;
		default) make ARCH=$ARCH_PLATFORM CROSS_COMPILE=$CROSS_TOOLCHAIN ;;
		*)echo "no support $1,only support *config";;
	esac
}


if [ $# -gt 0 ];then
	do_build $1;
else
	do_build default
fi
