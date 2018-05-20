cmake_minimum_required(VERSION 3.0)


set(SOURCE_DIR sources/src)
set(INCLUDE_DIR sources/inc)
set(LIB_DIR lib/)

# ==================================================================

find_package(OpenGL)

set( LIB_INCLUDE_DIR
	 ${LIB_DIR}/glew/include/
	 )

find_library(GLEW_LIB glew32 PATHS ${LIB_DIR}/glew/lib/Release/x64/)

# ==================================================================

set(WIN32_BOOTSTRAP_DIR ${SOURCE_DIR}/win32_bootstrap)
set( WIN32_BOOTSTRAP_SOURCES
	${WIN32_BOOTSTRAP_DIR}/main.cc
	)

# ==================================================================

include_directories(${INCLUDE_DIR} ${LIB_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")

add_definitions()

# ==================================================================

project(win32_bootstrap)

add_executable(win32_bootstrap WIN32 ${WIN32_BOOTSTRAP_SOURCES})
target_link_libraries( win32_bootstrap
					   PRIVATE
					   "${GLEW_LIB}"
					   ${OPENGL_gl_LIBRARY}
					   )
add_custom_command( TARGET win32_bootstrap POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different
							"${PROJECT_SOURCE_DIR}/${LIB_DIR}/glew/bin/Release/x64/glew32.dll"
							$<TARGET_FILE_DIR:win32_bootstrap>
							)

