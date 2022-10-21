cmake_minimum_required(VERSION 3.20)

cmake_path(SET ANDROID_NDK_PATH ${CMAKE_ANDROID_NDK})

# build native_app_glue as a static lib
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=gnu++20 -frtti")
# Debug Information format, gdwarf2 supports local variables in debugger 
set(CMAKE_CXX_FLAGS_DEBUG "-g2 -gdwarf-2")

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

# Add glue
add_library(native_app_glue STATIC ${ANDROID_NDK_PATH}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PRIVATE ${ANDROID_NDK_PATH}/sources/android/native_app_glue)

# Add native-activity
add_library(native-activity SHARED ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_include_directories(native-activity PRIVATE ${ANDROID_NDK_PATH}/sources/android/native_app_glue)
target_link_libraries(native-activity
    android
	native_app_glue
    EGL
    GLESv1_CM
    log
    nlohmann_json
    JSerializerLib)
