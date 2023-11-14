cmake_minimum_required(VERSION 3.20)
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=c++20 -frtti -fexceptions -Wno-ignored-attributes")



file(GLOB_RECURSE ALL_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/**)
file(GLOB_RECURSE SHARED_FILES CONFIGURE_DEPENDS ${shared_data_path}**)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source" FILES ${ALL_FILES})
source_group(TREE "${shared_data_path}" PREFIX "Shared" FILES ${SHARED_FILES})
add_executable(server-app ${ALL_FILES} ${SHARED_FILES})
target_include_directories(server-app PRIVATE	"src/" ${shared_data_path})
target_link_libraries(server-app LINK_PUBLIC  nlohmann_json JSerializerLib asio mongoc_shared)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT server-app)

target_compile_features(server-app PUBLIC cxx_std_20)
# create config
configure_file(server_config_template.json server_config.json)
target_compile_definitions(server-app PRIVATE CONFIG_PATH="server_config.json")

