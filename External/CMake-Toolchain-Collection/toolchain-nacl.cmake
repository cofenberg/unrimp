#
# Google Native Client toolchain file for CMake
#
# (c) Copyrights 2009-2011 Hartmut Seichter
# 
# Note: only works on OSX
#

set(NACL_PATH $ENV{NACL_ROOT} CACHE STRING "Native Client SDK Root Path")

set(NACL_TAG nacl)

set(NACL_HOST)

if (APPLE)
	set(NACL_HOST mac_x86)
endif()

# 
#
# 
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

#set(CMAKE_SYSTEM_PROCESSOR arm-eabi)

set(CMAKE_FIND_ROOT_PATH
	${NACL_PATH}/toolchain/${NACL_HOST}/nacl
	)


set(CMAKE_ASM_COMPILER ${NACL_PATH}/toolchain/${NACL_HOST}/bin/${NACL_TAG}-as)
set(CMAKE_C_COMPILER   ${NACL_PATH}/toolchain/${NACL_HOST}/bin/${NACL_TAG}-gcc-4.4.3)
set(CMAKE_CXX_COMPILER ${NACL_PATH}/toolchain/${NACL_HOST}/bin/${NACL_TAG}-c++)

set(CMAKE_C_FLAGS 
#	"--prefix=${NACL_PATH}/cross/armv7a --exec_prefix=${NACL_PATH}/cross/armv7a #--with-mpfr=${NACL_PATH}/staging/i686-linux/usr"
	CACHE STRING "OpenEmbedded - GCC/C flags" FORCE
)

set(CMAKE_CXX_FLAGS 
	""
	CACHE STRING "OpenEmbedded - GCC/C++ flags" FORCE
)


set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

