#
# iOS toolchain file for CMake
#
# (c) Copyrights 2010-2012 Hartmut Seichter
# 
# Note: this version can only be used for Xcode 4.3 (or Makefiles) and iOS 5 SDK 
#
# March 24, 2012 - Updated for Xcode 4.3.2 and iOS SDK 5.1 
#

# force CMake to assume crosscompiling
set(CMAKE_CROSSCOMPILING 1)

# assume we are somewhat on a Darwin system
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR arm )

# set the platform flags manually
set(APPLE 1)
set(IOS 1)

# hard set values
set(IOS_SDK_VERSION "5.1")
set(IOS_TARGET "iPhoneOS")
set(IOS_ARCH "armv7")

find_program(CMAKE_XCODE_SELECT xcode-select)
if(CMAKE_XCODE_SELECT)
	execute_process(COMMAND ${CMAKE_XCODE_SELECT} "-print-path" OUTPUT_VARIABLE OSX_DEVELOPER_ROOT OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

# some internal values
set(IOS_DEVELOPER_ROOT "${OSX_DEVELOPER_ROOT}/Platforms/${IOS_TARGET}.platform/Developer")
set(IOS_SDK_ROOT "${IOS_DEVELOPER_ROOT}/SDKs/${IOS_TARGET}${IOS_SDK_VERSION}.sdk")

# for Xcode 4.x we need to set the sysroot to the internal string
if(XCODE)
	set(CMAKE_OSX_SYSROOT "iphoneos${IOS_SDK_VERSION}" CACHE STRING "SDK version" FORCE)
else()
	set(CMAKE_OSX_SYSROOT "IOS_SDK_ROOT" CACHE STRING "SDK version" FORCE)
endif()

# 
set(CMAKE_OSX_ARCHITECTURES "${IOS_ARCH}" CACHE STRING "SDK Architecture" FORCE)

# default to searching for frameworks first
set (CMAKE_FIND_FRAMEWORK FIRST)

# set up the default search directories for frameworks
set (CMAKE_SYSTEM_FRAMEWORK_PATH
	${IOS_SDK_ROOT}/System/Library/Frameworks
	${IOS_SDK_ROOT}/Developer/Library/Frameworks
#	${IOS_SDK_ROOT}/System/Library/PrivateFrameworks
	)

# root path settings
set(CMAKE_FIND_ROOT_PATH 
	${IOS_DEVELOPER_ROOT}
	${IOS_SDK_ROOT}
	)

# set appropriate flags
set(CMAKE_C_FLAGS "--sysroot=${IOS_SDK_ROOT} -miphoneos-version-min=${IOS_SDK_VERSION} -arch ${IOS_ARCH} -v" CACHE STRING "C flags" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "C++ flags" FORCE)

	

#message(${CMAKE_FIND_ROOT_PATH})

# specify compiler
set(CMAKE_C_COMPILER   "${IOS_DEVELOPER_ROOT}/usr/bin/clang"    CACHE PATH "C compiler" FORCE)
set(CMAKE_CXX_COMPILER "${IOS_DEVELOPER_ROOT}/usr/bin/clang++"  CACHE PATH "C++ compiler" FORCE)

# for Xcode we need to skip the compiler introspection
if(XCODE)
	set(CMAKE_CXX_COMPILER_WORKS TRUE)
	set(CMAKE_C_COMPILER_WORKS TRUE)
endif()


# search paths (for makefiles the first one might be switched to "NEVER")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
