cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(shaderunner)

option(WARN_VERBOSE "Enable highest warning level and disable warnings as errors" OFF)
option(GL_DEBUG_CONTEXT "Create OpenGL debug context" OFF)

set(SOURCE_DIR sources/src)
set(LIB_DIR ${CMAKE_CURRENT_LIST_DIR}/../lib)

# ==============================================================================
# Boost should be built using the "tagged" naming convention
set(BOOST_ROOT
    "${LIB_DIR}/boost"
    CACHE
    PATH
    "Boost root installation"
)
set(Boost_ARCHITECTURE "-x64")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_DEBUG_LIBS ON)
set(Boost_USE_RELEASE_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
unset(Boost_USE_STATIC_RUNTIME)
set(Boost_NO_BOOST_CMAKE ON)
unset(Boost_USE_STATIC_RUNTIME CACHE)
find_package(Boost REQUIRED MODULE
    COMPONENTS
        filesystem
)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED PATHS ${LIB_DIR})
find_package(imgui CONFIG REQUIRED PATHS ${LIB_DIR})
find_package(bass CONFIG QUIET PATHS ${LIB_DIR})
# ==============================================================================

set(UTILITY_DIR ${SOURCE_DIR}/utility)
set( UTILITY_SOURCES
	 ${UTILITY_DIR}/file.cc
	 ${UTILITY_DIR}/clock.cc
	 )

set(OGLBASE_DIR ${SOURCE_DIR}/oglbase)
set( OGLBASE_SOURCES
	 ${OGLBASE_DIR}/error.cc
	 ${OGLBASE_DIR}/framebuffer.cc
	 ${OGLBASE_DIR}/handle.cc
	 ${OGLBASE_DIR}/shader.cc
	 )

set(UIBASE_DIR ${SOURCE_DIR}/uibase)
set( UIBASE_SOURCES
	 ${UIBASE_DIR}/gizmo_layer.cc
	 ${UIBASE_DIR}/imguicontext.cc
	 )

set(SHADERUNNER_DIR ${SOURCE_DIR}/shaderunner)
set( SHADERUNNER_SOURCES
	 ${SHADERUNNER_DIR}/shaderunner.cc
	 ${SHADERUNNER_DIR}/shader_cache.cc
	 )

set(APPBASE_DIR ${SOURCE_DIR}/appbase)
set( APPBASE_SOURCES
     ${APPBASE_DIR}/layer_mediator.cc
	 ${APPBASE_DIR}/imgui_layer.cc
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


	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	  add_compile_options(-std:c++latest)
	  add_compile_definitions(_SCL_SECURE_NO_WARNINGS)

	  if (WARN_VERBOSE)
		add_compile_options(-Wall)
	  else()
		add_compile_options(-W4 -WX)
	  endif(WARN_VERBOSE)
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR
		   "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	  add_compile_options(-std=c++17)

	  if (WARN_VERBOSE)
		add_compile_options(-Weverything)
	  else()
		add_compile_options(-Werror)
	  endif(WARN_VERBOSE)
	  else()
	  message("${CMAKE_CXX_COMPILER_ID}")
	endif()

function(_COMMON_PROJECT_OPTIONS TARGET_NAME)
	target_include_directories(${TARGET_NAME}
		PRIVATE
			sources/inc
	)
	target_link_libraries(${TARGET_NAME}
		PRIVATE
			Boost::filesystem
	)

	if(${GL_DEBUG_CONTEXT})
	  add_compile_definitions(SR_GL_DEBUG_CONTEXT)
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


add_library(utl SHARED ${UTILITY_SOURCES})
_common_project_options(utl)
add_library(ogb SHARED ${OGLBASE_SOURCES})
target_link_libraries(ogb PUBLIC OpenGL::GL GLEW::GLEW)
_common_project_options(ogb)

add_library(sr MODULE ${SHADERUNNER_SOURCES})
target_link_libraries(sr PRIVATE utl ogb)
_common_project_options(sr)

# ______________________________________________________________________________

add_library(appbase STATIC ${APPBASE_SOURCES})
target_link_libraries(appbase
	PUBLIC
		oglbase
	PRIVATE
		shaderunner
		uibase
)
_common_project_options(appbase)

# ______________________________________________________________________________

if (WIN32)
add_executable(win32_bootstrap WIN32 ${WIN32_BOOTSTRAP_SOURCES})
target_link_libraries(win32_bootstrap
	PRIVATE
		appbase
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
		"${CMAKE_CURRENT_BINARY_DIR}/install"
)

install(FILES
	"${GLEW_DSO_PATH}"
	DESTINATION
		"${CMAKE_CURRENT_BINARY_DIR}/install"
)
endif()

# ______________________________________________________________________________

find_package(X11 REQUIRED)

add_executable(xlib_bootstrap ${XLIB_BOOTSTRAP_SOURCES})
target_link_libraries(xlib_bootstrap
    PRIVATE
		X11
		X11::Xfixes
		appbase
		bass::bass
)
_common_project_options(xlib_bootstrap)

install(TARGETS xlib_bootstrap DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/install")

get_target_property(BASS_DSO_PATH
	bass::bass
	IMPORTED_LOCATION
)

add_custom_command(
	TARGET xlib_bootstrap
	POST_BUILD
	COMMAND
		${CMAKE_COMMAND} -E copy_if_different
			"${BASS_DSO_PATH}"
			$<TARGET_FILE_DIR:xlib_bootstrap>
)

install(FILES "${BASS_DSO_PATH}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/install")

# ==============================================================================
set(CPACK_PACKAGE_NAME "Shade Runner")
set(CPACK_PACKAGE_VERSION "0.0.2")
include(CPack)

