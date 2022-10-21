cmake_minimum_required(VERSION 3.20)

project(AndroidNativeActivity)

set(JSERIALIZER_ONLY_LIBRARY true)
add_subdirectory(ThirdParty/JSerializer)

add_subdirectory(AndroidSource)