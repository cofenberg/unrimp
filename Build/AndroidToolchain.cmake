# https://cmake.org/cmake/help/v3.7/manual/cmake-toolchains.7.html#cross-compiling-for-android
set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_SYSTEM_VERSION 23) # API level
set(CMAKE_ANDROID_API 23)
set(CMAKE_ANDROID_API_MIN 16)
set(CMAKE_ANDROID_NDK C:/ProgramData/Microsoft/AndroidNDK64/android-ndk-r13b) # TODO(co) Make this dynamic
set(CMAKE_ANDROID_ARCH arm64-v8a)
set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)
set(CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION clang)
set(CMAKE_ANDROID_STL_TYPE c++_static)
