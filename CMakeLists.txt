cmake_minimum_required(VERSION 3.20)

project(AndroidNativeActivity)

set(APP_VERSION "v0.0.1") # Set the version for server and client, checked for mismatch 
set(SOCKET_TIMEOUT_MS 5000) # Set the timeout for sockets in ms
set(MAX_MSG_LENGTH_DECIMALS 9)

# Must be set in main CMakeLists.txt for Visual Studio to recognize
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# JSerializer library
set(JSERIALIZER_ONLY_LIBRARY ON CACHE BOOL "no tests" FORCE)
add_subdirectory(ThirdParty/JSerializer)

#zlib
set(ZSTD_BUILD_STATIC OFF)
set(ZSTD_BUILD_SHARED ON)
set(BUILD_TESTING OFF)
set(ZSTD_BUILD_PROGRAMS OFF)
set(ZSTD_BUILD_TESTS OFF)
set(ZSTD_BUILD_CONTRIB OFF)
set(ZSTD_LEGACY_SUPPORT ON)
set(lib_zstd_include_path ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/zstd/lib)
add_subdirectory(ThirdParty/zstd/build/cmake)


#asio library
set(lib_asio_name asio)
set(lib_asio_include_path ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/asio-1.24.0/include/)
set(lib_asio_source_path ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/asio-1.24.0/include/)

#imgui library
set(imgui_name imgui)
set(imgui_source_path ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui)
set(imgui_android_source 		
		${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui/imgui.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui/imgui_demo.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui/imgui_draw.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui/imgui_tables.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui/imgui_widgets.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui/misc/cpp/imgui_stdlib.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui/backends/imgui_impl_android.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui/backends/imgui_impl_opengl3.cpp)


#shared data path
set(shared_data_path ${CMAKE_CURRENT_SOURCE_DIR}/Shared/)

include(FetchContent)

# Package Name : Name of the package/library
# Download PATH : URL to the package/library
# Destination PATH : Path to the source code of the package/library
function(TRY_DOWNLOAD_ZIP PACKAGE_NAME DOWNLOAD_PATH DESTINATION_PATH)
	if(EXISTS ${DESTINATION_PATH})
		message(STATUS "Package with name ${PACKAGE_NAME} and path ${DESTINATION_PATH} already exists, no download executed")
	else()
		message(STATUS "Download Package ${PACKAGE_NAME} ...")
		FetchContent_Declare(
						${PACKAGE_NAME} 
						URL ${DOWNLOAD_PATH} 
						SOURCE_DIR ${DESTINATION_PATH}
						DOWNLOAD_EXTRACT_TIMESTAMP false
						)
		FetchContent_MakeAvailable(${PACKAGE_NAME})
		message(STATUS "Done.")
	endif()
endfunction()

# Library name : A library with this name will be added, please add parenthesis around the name 
# Library path : The path to the downloaded library, without a "/" at the end
function(CREATE_HEADER_LIBRARY LIBRARY_NAME LIBRARY_PATH)
	get_property(use_folder_is_set GLOBAL PROPERTY USE_FOLDERS SET)
	if (NOT use_folder_is_set)
		message(WARNING "Please activate folder view : set_property(GLOBAL PROPERTY USE_FOLDERS ON)") 
	endif()
	
	message(STATUS "Find all files in library path : ${LIBRARY_PATH}")
	# Find all source files in library path
	file(GLOB_RECURSE LIBRARY_FILES CONFIGURE_DEPENDS ${LIBRARY_PATH}/**)
	message(STATUS "Found files for library ${LIBRARY_NAME} with path ${LIBRARY_PATH}")
	#message(STATUS "${LIBRARY_FILES}")
	add_library(${LIBRARY_NAME} INTERFACE)
	# Creating a dummy library to force the visual studio project creation
	add_library(${LIBRARY_NAME}_Preview STATIC ${LIBRARY_FILES})
	set_target_properties(${LIBRARY_NAME}_Preview PROPERTIES LINKER_LANGUAGE CXX)
	
	source_group(TREE "${LIBRARY_PATH}/" PREFIX "Include" FILES ${LIBRARY_FILES})
	message(STATUS "Created successfully library with name: ${LIBRARY_NAME}")
	
	unset(LIBRARY_FILES)
endfunction()

create_header_library(${lib_asio_name} ${lib_asio_source_path})
target_include_directories(${lib_asio_name} INTERFACE ${lib_asio_include_path})
#target_compile_definitions(${lib_asio_name} INTERFACE ASIO_NO_EXCEPTIONS=1)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	# add imgui lib
	add_library(${imgui_name} STATIC ${imgui_android_source})
	target_compile_definitions(${imgui_name} PRIVATE IMGUI_IMPL_OPENGL_ES3)
	target_include_directories(${imgui_name} PRIVATE ${imgui_source_path})
	file(GLOB_RECURSE IMGUI_FILES CONFIGURE_DEPENDS ${imgui_source_path}/**)
	source_group(TREE "${imgui_source_path}/" PREFIX "include" FILES ${IMGUI_FILES})
	message(STATUS "Created successfully library with name: ${imgui_name}")

	add_subdirectory(AndroidSource) 
	INCLUDE(android.config)
	message(STATUS "IP address from android config ${SERVER_IP}")
	target_compile_definitions(native-activity PRIVATE 
										SOCKET_TIMEOUT_MS=${SOCKET_TIMEOUT_MS}
										MAX_MSG_LENGTH_DECIMALS=${MAX_MSG_LENGTH_DECIMALS}
										SERVER_IP="${SERVER_IP}"
										SERVER_NAT_UDP_PORT_1=${SERVER_NAT_UDP_PORT_1}
										SERVER_NAT_UDP_PORT_2=${SERVER_NAT_UDP_PORT_2}
										SERVER_TRANSACTION_TCP_PORT=${SERVER_TRANSACTION_TCP_PORT}
										NAT_IDENT_REQUEST_FREQUNCY_MS=${NAT_IDENT_REQUEST_FREQUNCY_MS}
										NAT_IDENT_AMOUNT_SAMPLES_USED=${NAT_IDENT_AMOUNT_SAMPLES_USED}
										NAT_IDENT_MAX_PROG_SYM_DELTA=${NAT_IDENT_MAX_PROG_SYM_DELTA}
										NAT_COLLECT_REQUEST_DELAY_MS=${NAT_COLLECT_REQUEST_DELAY_MS} 
										NAT_COLLECT_PORTS_PER_SAMPLE=${NAT_COLLECT_PORTS_PER_SAMPLE}
										NAT_COLLECT_EXTRA_TIME_MS=${NAT_COLLECT_EXTRA_TIME_MS}
										NAT_COLLECT_SAMPLE_DELAY_MS=${NAT_COLLECT_SAMPLE_DELAY_MS}
										NAT_COLLECT_UPLOAD_DELAY_MS=${NAT_COLLECT_UPLOAD_DELAY_MS}
										NAT_TRAVERSE_ANALYZE_PORTS=${NAT_TRAVERSE_ANALYZE_PORTS}
										NAT_TRAVERSE_ATTEMPTS=${NAT_TRAVERSE_ATTEMPTS}
										NAT_TRAVERSE_DEADLINE_MS=${NAT_TRAVERSE_DEADLINE_MS}
										MONGO_DB_NAME="${MONGO_DB_NAME}"
										MONGO_NAT_TRAVERSAL_COLL_NAME="${MONGO_NAT_TRAVERSAL_COLL_NAME}"
										MONGO_NAT_SAMPLES_COLL_NAME="${MONGO_NAT_SAMPLES_COLL_NAME}"
										MONGO_NAT_USERS_COLL_NAME="${MONGO_NAT_USERS_COLL_NAME}"
										MONGO_INFORMATION_COLL_NAME="${MONGO_INFORMATION_COLL_NAME}"
										MONGO_VERSION_COLL_NAME="${MONGO_VERSION_COLL_NAME}"
										RANDOM_SYM_NAT_REQUIRED=${RANDOM_SYM_NAT_REQUIRED}
										TRAVERSAL_FEATURE_ENABLED=${TRAVERSAL_FEATURE_ENABLED}
										APP_VERSION="${APP_VERSION}"
									    MAX_LOG_LINES=${MAX_LOG_LINES}) 
else()
	set(ENABLE_AUTOMATIC_INIT_AND_CLEANUP OFF CACHE BOOL "Disable Clean Up" FORCE)
	set(ENABLE_EXAMPLES OFF CACHE BOOL "no examples" FORCE)
	set(ENABLE_TESTS OFF CACHE BOOL "no tests" FORCE)
	add_subdirectory(ThirdParty/mongo-c-driver)
	add_subdirectory(ServerSource)
	# create config
	configure_file(server_template_config.json ServerSource/server_config.json)
	target_compile_definitions(server-app PRIVATE 
										CONFIG_PATH="server_config.json"
										APP_VERSION="${APP_VERSION}"
										SOCKET_TIMEOUT_MS=${SOCKET_TIMEOUT_MS}
										MAX_MSG_LENGTH_DECIMALS=${MAX_MSG_LENGTH_DECIMALS})
endif()



