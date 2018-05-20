cmake_minimum_required(VERSION 3.0)

set(SOURCE_DIR sources/src)
set(INCLUDE_DIR sources/inc)

set(WIN32_BOOTSTRAP_DIR ${SOURCE_DIR}/win32_bootstrap)
set( WIN32_BOOTSTRAP_SOURCES
	${WIN32_BOOTSTRAP_DIR}/main.cc
	)


project(win32_bootstrap)


include_directories(${INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")

add_definitions()

add_executable(win32_bootstrap WIN32 ${WIN32_BOOTSTRAP_SOURCES})
