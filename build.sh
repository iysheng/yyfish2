#!/bin/sh

CPU_COUNT=`cat /proc/cpuinfo | awk '/processor/{print $2}' | wc -l`
BUILD_CPU_COUNT=`expr $CPU_COUNT / 2`

ARCH_PLATFORM=arm
CROSS_TOOLCHAIN=arm-none-eabi-
#CROSS_TOOLCHAIN=arm-linux-

usage() {
	echo "
	*config: just config the project
	default: just build
	openocd: openocd connet with board
	telnet: use telnet connect with board
	"
}

do_build() {
	case $1 in
		*config) make ARCH=$ARCH_PLATFORM CROSS_COMPILE=$CROSS_TOOLCHAIN menuconfig;;
		default) make ARCH=$ARCH_PLATFORM CROSS_COMPILE=$CROSS_TOOLCHAIN ;;
		openocd) sudo openocd -s /usr/share/openocd/scripts/ -f board/yyfish.cfg ;;
		telnet) telnet localhost 4444 ;;
		*)usage ;;
	esac
}


if [ $# -gt 0 ];then
	do_build $1;
else
	do_build default
fi


# openocd 命令
## 打开 openocd  
#sudo openocd -s /usr/local/share/openocd/scripts/board/ -f yyfish.cfg
## 打开 telnet 连接设备
#telnet localhost 4444
