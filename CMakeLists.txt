cmake_minimum_required(VERSION 3.8)
project(LeagueModel)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(UseAsLibrary "Use LeagueModel as library" OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(DirectX_D3DX9_LIBRARY OFF)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)

set(CMAKE_PATH ${CMAKE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
find_package(OpenGL REQUIRED)

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

function(ADD_LEAGUEMODEL_SOURCE folder inc src)
	source_group(${folder} FILES ${inc} ${src})
	set(LEAGUEMODEL_SOURCES ${LEAGUEMODEL_SOURCES} ${inc} ${src} PARENT_SCOPE)
endfunction(ADD_LEAGUEMODEL_SOURCE)

#local includes
include_directories("include")
include_directories("src")

ADD_LEAGUEMODEL_SOURCE(Application                     "src/application.hpp"                                      "src/application.cpp")
ADD_LEAGUEMODEL_SOURCE(Application\\Mesh               "src/application_mesh.hpp"                                 "src/application_mesh.cpp")

ADD_LEAGUEMODEL_SOURCE(EventHandler                    "src/event_handler.hpp"                                    "src/event_handler.cpp")
ADD_LEAGUEMODEL_SOURCE(EventHandler\\Event             "src/event_handler/event.hpp"                              "src/event_handler/event.cpp")
ADD_LEAGUEMODEL_SOURCE(EventHandler\\Events            "src/event_handler/events.hpp"                             "")
ADD_LEAGUEMODEL_SOURCE(EventHandler\\EventThread       "src/event_handler/event_thread.hpp"                       "src/event_handler/event_thread.cpp")

ADD_LEAGUEMODEL_SOURCE(File                            "include/league_model/file/file_system.hpp"                     "src/file_system.cpp")
ADD_LEAGUEMODEL_SOURCE(File\\Base                      "include/league_model/file/base_file.hpp"                  "src/file/base_file.cpp")
ADD_LEAGUEMODEL_SOURCE(File\\Emscripten                "include/league_model/file/emscripten_file.hpp"            "src/file/emscripten_file.cpp")
ADD_LEAGUEMODEL_SOURCE(File\\Native                    "include/league_model/file/native_file.hpp"                "src/file/native_file.cpp")

ADD_LEAGUEMODEL_SOURCE(Helper                          "src/helper.hpp"                                           "src/helper.cpp")

ADD_LEAGUEMODEL_SOURCE(League\\Animation               "include/league_model/animation.hpp"                       "src/league/animation.cpp")
ADD_LEAGUEMODEL_SOURCE(League\\Bin                     "include/league_model/bin.hpp"                             "src/league/bin.cpp")
ADD_LEAGUEMODEL_SOURCE(League\\Bin\\ValueStorage       "include/league_model/bin_valuestorage.hpp"                "src/league/bin_valuestorage.cpp")
ADD_LEAGUEMODEL_SOURCE(League\\Skeleton                "include/league_model/skeleton.hpp"                        "src/league/skeleton.cpp")
ADD_LEAGUEMODEL_SOURCE(League\\Skin                    "include/league_model/skin.hpp"                            "src/league/skin.cpp")

ADD_LEAGUEMODEL_SOURCE(Platform                        "include/league_model/platform.hpp"                        "src/platform.cpp")
ADD_LEAGUEMODEL_SOURCE(Platform\\BasePlatform          "include/league_model/platform/base_platform.hpp"          "src/platform/base_platform.cpp")
ADD_LEAGUEMODEL_SOURCE(Platform\\EmscriptenPlatform    "include/league_model/platform/emscripten_platform.hpp"    "src/platform/emscripten_platform.cpp")
ADD_LEAGUEMODEL_SOURCE(Platform\\WindowsPlatform       "include/league_model/platform/windows_platform.hpp"       "src/platform/windows_platform.cpp")

ADD_LEAGUEMODEL_SOURCE(Profiling                       "src/profiling.hpp"                                        "src/profiling.cpp")
ADD_LEAGUEMODEL_SOURCE(Profiling\\Memory               "src/profiling/memory.hpp"                                 "src/profiling/memory.cpp")

ADD_LEAGUEMODEL_SOURCE(Renderer                        "src/renderer.hpp"                                         "src/renderer.cpp")
ADD_LEAGUEMODEL_SOURCE(Renderer\\IndexBuffer           "src/renderer/index_buffer.hpp"                            "src/renderer/index_buffer.cpp")
ADD_LEAGUEMODEL_SOURCE(Renderer\\OpenGL                "src/renderer/opengl.hpp"                                  "src/renderer/opengl.cpp")
ADD_LEAGUEMODEL_SOURCE(Renderer\\Shader                "src/renderer/shader.hpp"                                  "src/renderer/shader.cpp")
ADD_LEAGUEMODEL_SOURCE(Renderer\\ShaderProgram         "src/renderer/shader_program.hpp"                          "src/renderer/shader_program.cpp")
ADD_LEAGUEMODEL_SOURCE(Renderer\\Texture               "src/renderer/texture.hpp"                                 "src/renderer/texture.cpp")
ADD_LEAGUEMODEL_SOURCE(Renderer\\Variable              "src/renderer/shader_variable.hpp"                         "src/renderer/shader_variable.cpp")
ADD_LEAGUEMODEL_SOURCE(Renderer\\VertexBuffer          "src/renderer/vertex_buffer.hpp"                           "src/renderer/vertex_buffer.cpp")

ADD_LEAGUEMODEL_SOURCE(WebInterface                    "src/web_interface.hpp"                                    "src/web_interface.cpp")

ADD_LEAGUEMODEL_SOURCE(Window                          "src/window.hpp"                                           "src/window.cpp")
ADD_LEAGUEMODEL_SOURCE(Window\\BaseWindow              "src/window/base_window.hpp"                               "src/window/base_window.cpp")
ADD_LEAGUEMODEL_SOURCE(Window\\EmscriptenWindow        "src/window/emscripten_window.hpp"                         "src/window/emscripten_window.cpp")
ADD_LEAGUEMODEL_SOURCE(Window\\WindowsWindow           "src/window/windows_window.hpp"                            "src/window/windows_window.cpp")


if (UseAsLibrary)
	add_executable(LeagueModel ${LEAGUEMODEL_SOURCES})
	set_target_properties(LeagueModel PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	
	if (WIN32)
		target_link_libraries(LeagueModel opengl32)

		add_custom_command(TARGET LeagueModel POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/libs" $<TARGET_FILE_DIR:LeagueModel>)
	endif (WIN32)
else (UseAsLibrary)
	add_library(LeagueModel ${LEAGUEMODEL_SOURCES})
endif (UseAsLibrary)