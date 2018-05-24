#
# Clang compiler on OSX toolchain
#
# (c) Copyrights 2009-2011 Hartmut Seichter
# 
# Note: using LLVM as a "cross compiler"
#

include(CMakeForceCompiler)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)

set(LLVM_PATH $ENV{LLVM_ROOT} CACHE STRING "Native Client SDK Root Path")


set(LLVM_HOST)
set(LLVM_TAG LLVM)

# use LLVM from MacPorts
set(LLVM_PATH /opt/local)

# 
# Basic settings
# 
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_FIND_ROOT_PATH
	${LLVM_PATH}/include
	${LLVM_PATH}/usr/include
	)


set(CMAKE_C_COMPILER   ${LLVM_PATH}/bin/clang)
set(CMAKE_CXX_COMPILER ${LLVM_PATH}/bin/clang++)
set(CMAKE_ASM_COMPILER ${LLVM_PATH}/bin/${LLVM_TAG}-as)

set(TOOLCHAIN_LLVM_DEBUG 1)

if(TOOLCHAIN_LLVM_DEBUG)
	message(STATUS "${CMAKE_C_COMPILER}")
endif()

set(CMAKE_C_FLAGS 
	CACHE STRING "LLVM - GCC/C flags" FORCE
)

set(CMAKE_CXX_FLAGS 
	""
	CACHE STRING "LLVM - GCC/C++ flags" FORCE
)

set(CMAKE_SHARED_LINKER_FLAGS 
	""
	CACHE STRING "LLVM - GCC/C++ flags" FORCE
)


set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

