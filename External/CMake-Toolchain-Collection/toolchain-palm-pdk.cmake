#
# Palm PDK toolchain file for CMake
#
# (c) Copyrights 2010-2011 Hartmut Seichter
# 
# Note: only tested on OSX
#

#
# updated for Palm PDK 1.4.5
#

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm-none-linux-gnueabi)

set(PALM TRUE)

set(PDK_TARGET "arm-gcc")

option(PDK_EMULATOR "Use Emulator or device" OFF)
if(PDK_EMULATOR)
	# fiddle around with the Palm PDK
	set(PDK_ROOT /opt/PalmPDK)	
	set(PDK_SYS_ROOT ${PDK_ROOT}/sysroot)
else()
	set(PDK_ROOT /opt/PalmPDK/${PDK_TARGET})
	set(PDK_SYS_ROOT ${PDK_ROOT}/sysroot) 
endif()

if(PDK_TOOLCHAIN_DEBUG)
	message(STATUS "PDK Root: ${PDK_ROOT}")
endif()
 
set(CMAKE_C_FLAGS "--sysroot=${PDK_SYS_ROOT}" CACHE STRING "Palm PDK GCC flags" FORCE)
set(CMAKE_CXX_FLAGS "--sysroot=${PDK_SYS_ROOT}" CACHE STRING "Palm PDK flags" FORCE)

set(CMAKE_C_COMPILER   "${PDK_ROOT}/bin/${CMAKE_SYSTEM_PROCESSOR}-gcc") 
set(CMAKE_CXX_COMPILER "${PDK_ROOT}/bin/${CMAKE_SYSTEM_PROCESSOR}-g++") 

set(CMAKE_FIND_ROOT_PATH 
	${PDK_SYS_ROOT}
	${PDK_SYS_ROOT}/usr
	)
 
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
