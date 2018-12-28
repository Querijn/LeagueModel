cmake_minimum_required(VERSION 3.8)
project(LeagueExtract)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

IF(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
	SET(CMAKE_CXX_FLAGS "-g -std=c++11 -fpermissive")
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL MSVC)
	SET(CMAKE_CXX_FLAGS "/EHsc") #So it doesn't constantly throw warnings
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL Intel)
	IF(WIN32)
	SET(CMAKE_CXX_FLAGS "/Qstd=c++11")
	ELSE()
	SET(CMAKE_CXX_FLAGS "-std=c++11")
	ENDIF()
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
	SET(CMAKE_CXX_FLAGS "-std=c++11")
ENDIF()

function(ADD_LEAGUE_EXTRACT_SOURCE folder inc src)
	source_group(${folder} FILES ${inc} ${src})
	set(LEAGUE_EXTRACT_SOURCES ${LEAGUE_EXTRACT_SOURCES} ${inc} ${src} PARENT_SCOPE)
endfunction(ADD_LEAGUE_EXTRACT_SOURCE)

#external projects
add_subdirectory("external/LeagueModel")
add_subdirectory("external/zlib")
add_subdirectory("external/zstd/build/cmake")

#external includes
include_directories("external/zstd/lib")
include_directories("external/zlib")
include_directories("${CMAKE_BINARY_DIR}/external/zlib")

set_target_properties(zlib PROPERTIES FOLDER Libraries/zlib)
set_target_properties(zlibstatic PROPERTIES FOLDER Libraries/zlib)
set_target_properties(uninstall PROPERTIES FOLDER Libraries/zlib)

set_target_properties("clean-all" PROPERTIES FOLDER Libraries/zstd)
set_target_properties(zstd PROPERTIES FOLDER Libraries/zstd)
set_target_properties(libzstd_shared PROPERTIES FOLDER Libraries/zstd)
set_target_properties(libzstd_static PROPERTIES FOLDER Libraries/zstd)

#local includes
include_directories("src")
include_directories("external/LeagueModel/include")

ADD_LEAGUE_EXTRACT_SOURCE(Application	""							"src/main.cpp")
ADD_LEAGUE_EXTRACT_SOURCE(WAD			"src/wad_file.hpp"			"src/wad_file.cpp")

add_executable(LeagueExtract ${LEAGUE_EXTRACT_SOURCES})
set_target_properties(LeagueExtract PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
target_link_libraries(LeagueExtract zlibstatic libzstd_static LeagueModel)