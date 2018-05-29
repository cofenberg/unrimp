# https://cmake.org/cmake/help/v3.7/manual/cmake-toolchains.7.html#cross-compiling-for-android
set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_SYSTEM_VERSION 23) # API level
set(CMAKE_ANDROID_API 23)
set(CMAKE_ANDROID_API_MIN 16)
set(CMAKE_ANDROID_NDK C:/ProgramData/Microsoft/AndroidNDK64/android-ndk-r17) # TODO(co) Make this dynamic
set(CMAKE_ANDROID_ARCH arm64-v8a)
set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)
set(CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION clang)
set(CMAKE_ANDROID_STL_TYPE c++_shared)	# "Remove non-libc++ STLs" - https://android.googlesource.com/platform/ndk/+/master/docs/Roadmap.md - TODO(co) Is it considered to be safe to use "c++_shared" instead of "c++_static" to drastically reduce the binary size, or will the app then fail to run on several Android devices?
