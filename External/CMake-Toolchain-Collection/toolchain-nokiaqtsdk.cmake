#
# Qt SDK for Maemo/Harmattan toolchain file for CMake
#
# (c) Copyrights 2009-2011 Hartmut Seichter
# 
# Note: only tested on OSX
#


#
# updated for NokiaQtSDK 03/Nov/2010
#

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm-none-linux-gnueabi)

set(N900 ON CACHE BOOL "Detected a N900 toolchain" FORCE)
 
# fiddle around with the Nokia SDK
set(NOKIAQTSDK_ROOT $ENV{HOME}/QtSDK)
set(MAEMO_TOOLCHAIN
	${NOKIAQTSDK_ROOT}/Maemo/4.6.2/toolchains/arm-2007q3-51sb6-gdb71-arm-none-linux-gnueabi_darwin
	)
set(MAEMO_SYS_ROOT
	${NOKIAQTSDK_ROOT}/Maemo/4.6.2/sysroots/fremantle-arm-sysroot-20.2010.36-2-slim) 

set(CMAKE_C_FLAGS "--sysroot=${MAEMO_SYS_ROOT} -I${MAEMO_SYS_ROOT}/usr/include" CACHE STRING "Maemo GCC flags" FORCE)
set(CMAKE_CXX_FLAGS "--sysroot=${MAEMO_SYS_ROOT} -I${MAEMO_SYS_ROOT}/usr/include" CACHE STRING "Maemo G++ flags" FORCE)

set(CMAKE_C_COMPILER   "${MAEMO_TOOLCHAIN}/bin/${CMAKE_SYSTEM_PROCESSOR}-gcc") 
set(CMAKE_CXX_COMPILER "${MAEMO_TOOLCHAIN}/bin/${CMAKE_SYSTEM_PROCESSOR}-g++") 

set(CMAKE_FIND_ROOT_PATH 
	${MAEMO_SYS_ROOT}
	${MAEMO_SYS_ROOT}/usr
	)
 
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
