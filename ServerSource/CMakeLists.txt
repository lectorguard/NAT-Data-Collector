cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=c++20 -frtti -fexceptions")


file(GLOB_RECURSE ALL_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/**)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source" FILES ${ALL_FILES})
add_executable(server-app ${ALL_FILES})
target_include_directories(server-app PRIVATE	"/" )
target_link_libraries(server-app LINK_PUBLIC  nlohmann_json JSerializerLib asio mongoc_static)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT server-app)

