cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(shaderunner)

option(WARN_VERBOSE "Enable highest warning level and disable warnings as errors" OFF)

set(SOURCE_DIR sources/src)
set(LIB_DIR lib)

# ==============================================================================
include(cmake/external/boost.cmake)
# ==============================================================================
find_package(OpenGL REQUIRED)
# ==============================================================================
include(cmake/external/glew.cmake)
# ==============================================================================
include(cmake/external/imgui.cmake)
# ==============================================================================

set(UTILITY_DIR ${SOURCE_DIR}/utility)
set( UTILITY_SOURCES
	 ${UTILITY_DIR}/file.cc
	 ${UTILITY_DIR}/clock.cc
	 )

set(OGLBASE_DIR ${SOURCE_DIR}/oglbase)
set( OGLBASE_SOURCES
	 ${OGLBASE_DIR}/error.cc
	 ${OGLBASE_DIR}/handle.cc
	 ${OGLBASE_DIR}/shader.cc
	 )

set(UIBASE_DIR ${SOURCE_DIR}/uibase)
set( UIBASE_SOURCES
	 ${UIBASE_DIR}/imguicontext.cc
	 )

set(SHADERUNNER_DIR ${SOURCE_DIR}/shaderunner)
set( SHADERUNNER_SOURCES
	 ${SHADERUNNER_DIR}/shaderunner.cc
	 ${SHADERUNNER_DIR}/shader_cache.cc
	 )

set(WIN32_BOOTSTRAP_DIR ${SOURCE_DIR}/win32_bootstrap)
set( WIN32_BOOTSTRAP_SOURCES
	 ${WIN32_BOOTSTRAP_DIR}/main.cc
	 )

set(XLIB_BOOTSTRAP_DIR ${SOURCE_DIR}/xlib_bootstrap)
set( XLIB_BOOTSTRAP_SOURCES
     ${XLIB_BOOTSTRAP_DIR}/main.cc
	 )

# ==============================================================================

function(_COMMON_PROJECT_OPTIONS TARGET_NAME)
	target_include_directories(${TARGET_NAME}
		PRIVATE
			sources/inc
	)
	target_link_libraries(${TARGET_NAME}
		PRIVATE
			Boost::boost
			Boost::filesystem
	)

	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	  add_compile_options(-std:c++latest)

	  if (WARN_VERBOSE)
		add_compile_options(-Wall)
	  else()
		add_compile_options(-W4 -WX)
	  endif(WARN_VERBOSE)
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	  add_compile_options(-std:c++17)

	  if (WARN_VERBOSE)
		add_compile_options(-Weverything)
	  else()
		add_compile_options(-Werror)
	  endif(WARN_VERBOSE)
	endif()

endfunction()

# ==============================================================================

add_library(utility STATIC ${UTILITY_SOURCES})
_common_project_options(utility)

# ______________________________________________________________________________

add_library(oglbase STATIC ${OGLBASE_SOURCES})
target_link_libraries(oglbase
	PUBLIC
		OpenGL::GL
		GLEW::GLEW
)
_common_project_options(oglbase)

# ______________________________________________________________________________

add_library(uibase STATIC ${UIBASE_SOURCES})
target_link_libraries(uibase
	PUBLIC
		imgui::imgui
	PRIVATE
		oglbase
)
_common_project_options(uibase)

# ______________________________________________________________________________

add_library(shaderunner STATIC ${SHADERUNNER_SOURCES})
target_link_libraries(shaderunner
	PRIVATE
		utility
		oglbase
)
_common_project_options(shaderunner)

# ______________________________________________________________________________

add_executable(win32_bootstrap WIN32 ${WIN32_BOOTSTRAP_SOURCES})
target_link_libraries(win32_bootstrap
	PRIVATE
		oglbase
		uibase
		shaderunner
)
_common_project_options(win32_bootstrap)

# NOTE(andrea.machizaud) Fetch Glew dynamic library path directly from CMake imported target
get_target_property(GLEW_DSO_PATH
	GLEW::GLEW
	IMPORTED_LOCATION
)

message(STATUS "[install/GLEW] DSO path: ${GLEW_DSO_PATH}")

add_custom_command(
	TARGET win32_bootstrap
	POST_BUILD
	COMMAND
		${CMAKE_COMMAND} -E copy_if_different
			"${GLEW_DSO_PATH}"
			$<TARGET_FILE_DIR:win32_bootstrap>
)

install(TARGETS win32_bootstrap
	DESTINATION
		"${CMAKE_CURRENT_BINARY_DIR}"
)

install(FILES
	"${GLEW_DSO_PATH}"
	DESTINATION
		"${CMAKE_CURRENT_BINARY_DIR}"
)

# ______________________________________________________________________________

add_executable(xlib_bootstrap ${XLIB_BOOTSTRAP_SOURCES})
target_link_libraries(xlib_bootstrap
    PRIVATE
		X11
		shaderunner
)
_common_project_options(xlib_bootstrap)

# ==============================================================================
set(CPACK_PACKAGE_NAME "Shade Runner")
set(CPACK_PACKAGE_VERSION "0.0.1")
include(CPack)

