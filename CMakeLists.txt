cmake_minimum_required(VERSION 3.15)
project(LeagueModel LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
function(ADD_SRC var folder inc src)
	source_group(${folder} FILES ${inc} ${src})
	set(${var} ${${var}} ${inc} ${src} CACHE INTERNAL "")
endfunction(ADD_SRC)

# Sokol
add_subdirectory("ext/sokol")
set_target_properties(Sokol PROPERTIES FOLDER Libraries)
target_compile_definitions(Sokol PUBLIC SOKOL_IMGUI)

# LeagueLib
add_subdirectory("ext/leaguelib")
set_target_properties(LeagueLib PROPERTIES FOLDER Libraries)

set(LEAMOD_SRC "" CACHE INTERNAL "LeagueModel Source Files")
ADD_SRC(LEAMOD_SRC	"\\"					""									"src/main.cpp")

ADD_SRC(LEAMOD_SRC	"Skin"					"src/skin.hpp"						"src/skin.cpp")
ADD_SRC(LEAMOD_SRC	"Skeleton"				"src/skeleton.hpp"					"src/skeleton.cpp")
ADD_SRC(LEAMOD_SRC	"Animation"				"src/animation.hpp"					"src/animation.cpp")
ADD_SRC(LEAMOD_SRC	"Animation"				"src/animation_graph.hpp"			"src/animation_graph.cpp")
ADD_SRC(LEAMOD_SRC	"Animation"				"src/animation_flags.hpp"			"")
ADD_SRC(LEAMOD_SRC	"Character"				"src/character.hpp"					"src/character.cpp")
ADD_SRC(LEAMOD_SRC	"Character\\Animation"	"src/character_animation.hpp"		"src/character_animation.cpp")
ADD_SRC(LEAMOD_SRC	"Image"					"src/managed_image.hpp"				"src/managed_image.cpp")
ADD_SRC(LEAMOD_SRC	"Input"					"src/simple_input.hpp"				"src/simple_input.cpp")
ADD_SRC(LEAMOD_SRC	"UI"					"src/UI.hpp"						"src/UI.cpp")
ADD_SRC(LEAMOD_SRC	"Shaders"				"src/shaders/animated_mesh.hpp"		"src/shaders/animated_mesh.glsl")

add_executable(LeagueModel ${LEAMOD_SRC})
target_include_directories(LeagueModel PRIVATE "ext/imgui/imgui")
target_include_directories(LeagueModel PRIVATE "ext/imgui")
target_include_directories(LeagueModel PRIVATE "ext/leaguelib/ext")
target_include_directories(LeagueModel PRIVATE "ext/leaguelib/ext/glm")
target_include_directories(LeagueModel PRIVATE "ext")
target_include_directories(LeagueModel PRIVATE "src")

if (WIN32)
	target_compile_definitions(LeagueModel PUBLIC WIN32_LEAN_AND_MEAN)
	target_compile_definitions(LeagueModel PUBLIC NOMINMAX)
	target_link_libraries(LeagueModel opengl32)

	target_compile_definitions(LeagueModel PUBLIC SOKOL_GLCORE33)
else (EMSCRIPTEN)
	target_compile_definitions(LeagueModel PUBLIC SOKOL_GLES2)
endif ()

target_link_libraries(LeagueModel Sokol LeagueLib)
set_target_properties(LeagueModel PROPERTIES
					VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
					CXX_STANDARD 20
					CXX_EXTENSIONS OFF
					)
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT LeagueModel)

if (EMSCRIPTEN)
	set(LEAMOD_FLAGS "" CACHE INTERNAL "LeagueModel Linker flags")
	if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
		set(${LEAMOD_FLAGS} ${LEAMOD_FLAGS} "-g4-s SAFE_HEAP=1 -s WARN_UNALIGNED=1 -fsanitize=undefined")
	else ()
		set(${LEAMOD_FLAGS} ${LEAMOD_FLAGS} "-O2 --closure 1")
	endif ()
	set_target_properties(LEAMOD PROPERTIES LINK_FLAGS ${LEAMOD_FLAGS} "-s FULL_ES2=1 -s DISABLE_EXCEPTION_CATCHING=2 -s MINIMAL_RUNTIME -lwebgl.js -s ALLOW_MEMORY_GROWTH=1 -std=c++20")
endif ()

# add_custom_target(LeagueModelShaderGen ${LEAMOD_TEMPLATES} COMMAND cmd /c "node ${CMAKE_CURRENT_SOURCE_DIR}/src/build_shader.js" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
# add_dependencies(LeagueModel LeagueModelShaderGen)