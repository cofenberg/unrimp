#
# Android "Standalone Toolchain" toolchain file for CMake
#
# (c) Copyrights 2011 Hartmut Seichter
# 
# Note: this version only targets NDK r5 upwards using the standalone
#

# need to know where the NDK resides
set(ANDROID_TOOLCHAIN_ROOT "$ENV{NDK_TOOLCHAIN_ROOT}" CACHE PATH "Android Standalone Toolchain location")

#set(ANDROID_NDK_TOOLCHAIN_DEBUG ON)

# basic setup
set(CMAKE_CROSSCOMPILING 1)
set(CMAKE_SYSTEM_NAME Linux)

# for convenience
set(ANDROID 1)

# set supported architecture
set(ANDROID_NDK_ARCH_SUPPORTED "arm;armv7;x86")
set(ANDROID_NDK_ARCH "arm" CACHE STRING "Android NDK CPU architecture (${ANDROID_NDK_ARCH_SUPPORTED})")
set_property(CACHE ANDROID_NDK_ARCH PROPERTY STRINGS ${ANDROID_NDK_ARCH_SUPPORTED})

# armeabi / armeabi-v7a / x86
set(ANDROID_NDK_ABI)
set(ANDROID_NDK_ABI_EXT)
set(ANDROID_NDK_GCC_PREFIX)

set(ANDROID_NDK_ARCH_CFLAGS)
set(ANDROID_NDK_ARCH_LDFLAGS)

if("${ANDROID_NDK_ARCH}" STREQUAL "arm" )
	set(ANDROID_NDK_ABI "armeabi")
	set(ANDROID_NDK_ABI_EXT "arm-linux-androideabi")
	set(ANDROID_NDK_GCC_PREFIX "arm-linux-androideabi")
	set(ANDROID_NDK_ARCH_CFLAGS "-D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__ -Wno-psabi -march=armv5te -mtune=xscale -msoft-float") 
	#  -mthumb
endif()	
if("${ANDROID_NDK_ARCH}" STREQUAL "armv7" )
	set(ANDROID_NDK_ABI "armeabi-v7a")
	set(ANDROID_NDK_ABI_EXT "arm-linux-androideabi")
	set(ANDROID_NDK_GCC_PREFIX "arm-linux-androideabi")
	set(ANDROID_NDK_ARCH_CFLAGS "-march=armv7-a -mfloat-abi=softfp")
	set(ANDROID_NDK_ARCH_LDFLAGS "-Wl,--fix-cortex-a8")
endif()
if("${ANDROID_NDK_ARCH}" STREQUAL "x86" )
	set(ANDROID_NDK_ABI "x86")
	set(ANDROID_NDK_ABI_EXT "x86")
	set(ANDROID_NDK_GCC_PREFIX "i686-android-linux")
endif()

if(ANDROID_NDK_TOOLCHAIN_DEBUG)
	message(STATUS "ANDROID_NDK_ABI - ${ANDROID_NDK_ABI}")
	message(STATUS "ANDROID_NDK_ABI_EXT - ${ANDROID_NDK_ABI_EXT}")
	message(STATUS "ANDROID_NDK_ARCH_CFLAGS - ${ANDROID_NDK_ARCH_CFLAGS}")
endif()

# global C flags
set(ANDROID_NDK_GLOBAL_CFLAGS "-DANDROID -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64 -ffunction-sections -funwind-tables")
set(ANDROID_NDK_GLOBAL_CXXFLAGS "-fno-exceptions -fno-rtti")

# set the Android Platform
set(ANDROID_API_SUPPORTED "android-9;android-14")
set(ANDROID_API "android-9" CACHE STRING "Android SDK API (${ANDROID_API_SUPPORTED})")
set_property(CACHE ANDROID_API PROPERTY STRINGS ${ANDROID_API_SUPPORTED})

# set sysroot - in Android this in function of Android API and architecture
set(ANDROID_NDK_SYSROOT "${ANDROID_TOOLCHAIN_ROOT}/sysroot/usr" CACHE PATH "NDK sysroot" FORCE)

# set version
set(ANDROID_NDK_GCC_VERSION "4.4.3")

# global linker flags
#  -Wl,-z,noexecstack -Wl,--gc-sections -Wl,-z,nocopyreloc
set(ANDROID_NDK_GLOBAL_LDFLAGS "-Wl,--no-undefined -Wl,-z,noexecstack -Wl,--fix-cortex-a8 ")


# linker flags (here only one thing missing here is lstdc++ - in Android the actual STL implementation is user dependent!)
# gnustl_static or the like need to be set in the actual cmake file!
set(CMAKE_SHARED_LINKER_FLAGS "-lc -lm -ldl -lgcc" CACHE STRING "Linker flags" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}" CACHE STRING "Linker flags" FORCE)

# some overrides (see docs/STANDALONE-TOOLCHAIN.html) 
set(CMAKE_C_FLAGS " ${ANDROID_NDK_GLOBAL_CFLAGS} --sysroot=${ANDROID_NDK_SYSROOT} -I${ANDROID_TOOLCHAIN_ROOT}/sysroot/usr/include -DANDROID ${ANDROID_NDK_ARCH_CFLAGS} ${ANDROID_NDK_ARCH_LDFLAGS} ${ANDROID_NDK_GLOBAL_LDFLAGS}" CACHE STRING "C flags" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${ANDROID_NDK_GLOBAL_CXXFLAGS}" CACHE STRING "C++ flags" FORCE)

# specify compiler
set(CMAKE_C_COMPILER   "${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_NDK_GCC_PREFIX}-gcc" CACHE PATH "C compiler" FORCE)
set(CMAKE_CXX_COMPILER "${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_NDK_GCC_PREFIX}-g++" CACHE PATH "C++ compiler" FORCE)

if(ANDROID_NDK_TOOLCHAIN_DEBUG)
	message(STATUS "c compiler: ${CMAKE_C_COMPILER}")
	message(STATUS "c++ compiler: ${CMAKE_CXX_COMPILER}")
	message(STATUS "sysroot: ${ANDROID_NDK_SYSROOT}")
	message(STATUS "companion: ${ANDROID_NDK_GCC_COMPANIONLIBRARY}")
	
endif()

# root path
set(CMAKE_FIND_ROOT_PATH ${ANDROID_NDK_SYSROOT})

# search paths
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
