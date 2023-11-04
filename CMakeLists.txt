cmake_minimum_required(VERSION 3.20)

project(AndroidNativeActivity)

# Must be set in main CMakeLists.txt for Visual Studio to recognize
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# JSerializer library
set(JSERIALIZER_ONLY_LIBRARY ON CACHE BOOL "no tests" FORCE)
add_subdirectory(ThirdParty/JSerializer)

#asio library
set(lib_asio_name asio)
set(lib_asio_include_path ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/asio-1.24.0/include/)
set(lib_asio_source_path ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/asio-1.24.0/include/)

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
	add_subdirectory(AndroidSource)
	target_compile_definitions(native-activity PRIVATE BUFFER_SIZE=16384)
	target_compile_definitions(native-activity PRIVATE SOCKET_TIMEOUT_MS=5000) 
else()
	set(ENABLE_AUTOMATIC_INIT_AND_CLEANUP OFF CACHE BOOL "Disable Clean Up" FORCE)
	set(ENABLE_EXAMPLES OFF CACHE BOOL "no examples" FORCE)
	set(ENABLE_TESTS OFF CACHE BOOL "no tests" FORCE)
	add_subdirectory(ThirdParty/mongo-c-driver)
	add_subdirectory(ServerSource)
	target_compile_definitions(server-app PRIVATE BUFFER_SIZE=16384)
	target_compile_definitions(server-app PRIVATE SOCKET_TIMEOUT_MS=5000) 
endif()



