cmake_minimum_required(VERSION 3.20)

cmake_path(SET ANDROID_NDK_PATH ${CMAKE_ANDROID_NDK})

# build native_app_glue as a static lib
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=gnu++20 -frtti -fexceptions")
# Debug Information format, gdwarf2 supports local variables in debugger 
set(CMAKE_CXX_FLAGS_DEBUG "-g2 -gdwarf-2")

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

# Add glue
add_library(native_app_glue STATIC ${ANDROID_NDK_PATH}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC ${ANDROID_NDK_PATH}/sources/android/native_app_glue)

# Add native-activity
file(GLOB_RECURSE ALL_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/**)
file(GLOB_RECURSE SHARED_FILES CONFIGURE_DEPENDS ${shared_data_path}**)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source" FILES ${ALL_FILES})
source_group(TREE "${shared_data_path}" PREFIX "Shared" FILES ${SHARED_FILES})

add_library(native-activity SHARED ${ALL_FILES} ${SHARED_FILES})

target_include_directories(native-activity PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/src/ 
	${shared_data_path}
	${imgui_source_path}
	${lib_zstd_include_path}
	)
target_link_libraries(native-activity
    android
    native_app_glue
    EGL
    GLESv1_CM
    GLESv3
    m		#default math lib
    log
    nlohmann_json
    JSerializerLib
    asio
    imgui
	libzstd_shared)
