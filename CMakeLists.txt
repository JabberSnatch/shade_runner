cmake_minimum_required(VERSION 3.0)

option(WARN_VERBOSE "Enable highest warning level and disable warnings as errors" OFF)


set(SOURCE_DIR sources/src)
set(INCLUDE_DIR sources/inc)
set(LIB_DIR lib)

# ==============================================================================

set( LIB_INCLUDE_DIR
	 ${LIB_DIR}/glew/include/
	 ${LIB_DIR}/boost/
	 )

function(_FIND_GL_LIBRARIES OUT)
	find_package(OpenGL REQUIRED)
	find_library(GLEW_LIB glew32 PATHS ${LIB_DIR}/glew/lib/Release/x64/)
	set(${OUT} "${GLEW_LIB}" ${OPENGL_gl_LIBRARY} PARENT_SCOPE)
endfunction()

# ==============================================================================

set(SHADERUNNER_DIR ${SOURCE_DIR}/shaderunner)
set( SHADERUNNER_SOURCES
	 ${SHADERUNNER_DIR}/shaderunner.cc
	 )

set(WIN32_BOOTSTRAP_DIR ${SOURCE_DIR}/win32_bootstrap)
set( WIN32_BOOTSTRAP_SOURCES
	 ${WIN32_BOOTSTRAP_DIR}/main.cc
	 )

# ==============================================================================

include_directories(${INCLUDE_DIR} ${LIB_INCLUDE_DIR})

function(_COMMON_COMPILE_OPTIONS)
	add_compile_options(-std:c++latest)

	if (WARN_VERBOSE)
	add_compile_options(-Wall)
	else()
	add_compile_options(-W4 -WX)
	endif(WARN_VERBOSE)
endfunction()

add_definitions()

# ==============================================================================

project(shaderunner)

_common_compile_options()
_find_gl_libraries(GL_LIBRARIES)
add_library(shaderunner STATIC ${SHADERUNNER_SOURCES})
target_link_libraries( shaderunner
					   PRIVATE
					   "${GL_LIBRARIES}"
					   )

# ______________________________________________________________________________

project(win32_bootstrap)

_common_compile_options()
_find_gl_libraries(GL_LIBRARIES)
add_executable(win32_bootstrap WIN32 ${WIN32_BOOTSTRAP_SOURCES})
target_link_libraries( win32_bootstrap
					   PRIVATE
					   "${GL_LIBRARIES}"
					   shaderunner
					   )
add_custom_command( TARGET win32_bootstrap POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different
							"${PROJECT_SOURCE_DIR}/${LIB_DIR}/glew/bin/Release/x64/glew32.dll"
							$<TARGET_FILE_DIR:win32_bootstrap>
							)

