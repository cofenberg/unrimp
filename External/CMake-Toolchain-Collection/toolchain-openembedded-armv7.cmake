#
# OpenEmbedded toolchain file for CMake
#
# (c) Copyrights 2009-2011 Hartmut Seichter
# 
# Note: this version only targets ARMv7 devices (BeagleBoard) on OE
#


set(OPENEMBEDDED_PATH $ENV{OE_ROOT} CACHE STRING "OpenEmbedded Root Path")

set(V4L2_TARGET_ROOT "${OPENEMBEDDED_PATH}/staging/armv7a-angstrom-linux-gnueabi/usr")

# 
#
# 
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_SYSTEM_PROCESSOR arm-eabi)

set(CMAKE_ASM_COMPILER ${OPENEMBEDDED_PATH}/cross/armv7a/bin/arm-angstrom-linux-gnueabi-as)
set(CMAKE_C_COMPILER ${OPENEMBEDDED_PATH}/cross/armv7a/bin/arm-angstrom-linux-gnueabi-gcc)
set(CMAKE_CXX_COMPILER ${OPENEMBEDDED_PATH}/cross/armv7a/bin/arm-angstrom-linux-gnueabi-g++)

set(CMAKE_C_FLAGS 
#	"--prefix=${OPENEMBEDDED_PATH}/cross/armv7a --exec_prefix=${OPENEMBEDDED_PATH}/cross/armv7a --with-mpfr=${OPENEMBEDDED_PATH}/staging/i686-linux/usr"
	CACHE STRING "OpenEmbedded - GCC/C flags" FORCE
)

set(CMAKE_CXX_FLAGS 
	""
	CACHE STRING "OpenEmbedded - GCC/C++ flags" FORCE
)

set(CMAKE_FIND_ROOT_PATH
	${OPENEMBEDDED_PATH}
	${OPENEMBEDDED_PATH}/cross
	${OPENEMBEDDED_PATH}/cross/armv7a
	)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

