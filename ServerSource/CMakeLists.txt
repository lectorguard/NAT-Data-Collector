cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=c++20 -frtti -fexceptions")

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/ThirdParty/mongo-c-driver-1.23.1/cmake-build/src/libbson/bson/)
	message(FATAL_ERROR "Mongo-c-driver library does not exist, please execute shell script to generate files \n"
						"Path to .sh file : <root dir>/ThirdParty/*.sh \n"
						"Please check the documentation for other platforms (e.g. windows) https://mongoc.org/libmongoc/current/installing.html")
endif()

find_package (bson-1.0 1.7 REQUIRED PATHS "${CMAKE_SOURCE_DIR}/ThirdParty/mongo-c-driver-1.23.1/cmake-build/src/libbson/bson/")
find_package (mongoc-1.0 1.7 REQUIRED PATHS "${CMAKE_SOURCE_DIR}/ThirdParty/mongo-c-driver-1.23.1/cmake-build/src/libmongoc/mongoc/")

file(GLOB_RECURSE ALL_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/**)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source" FILES ${ALL_FILES})
add_executable(server-app ${ALL_FILES})
target_include_directories(server-app PRIVATE "/")
target_link_libraries(server-app LINK_PUBLIC  nlohmann_json JSerializerLib asio mongo::mongoc_shared)

