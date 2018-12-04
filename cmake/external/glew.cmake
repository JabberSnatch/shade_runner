
find_package(GLEW
    QUIET
)
if(NOT GLEW_FOUND)
    message(STATUS "[GLEW] Glew has not been found via `find_package`, fetching it from source...")
    include(FetchContent)

    FetchContent_Declare(glew
        URL      https://github.com/nigels-com/glew/releases/download/glew-2.1.0/glew-2.1.0-win32.zip
        URL_HASH MD5=32a72e6b43367db8dbea6010cd095355
    )

    FetchContent_GetProperties(glew)
    if(NOT glew_POPULATED)
        FetchContent_Populate(glew)
    endif()

    get_filename_component(_INCLUDE_DIR       "${glew_SOURCE_DIR}/include" ABSOLUTE)
    get_filename_component(_IMPLIB            "${glew_SOURCE_DIR}/lib/Release/x64/glew32.lib" ABSOLUTE)
    get_filename_component(_IMPORTED_LOCATION "${glew_SOURCE_DIR}/bin/Release/x64/glew32.dll" ABSOLUTE)

    add_library(GLEW::GLEW SHARED IMPORTED)

    set_target_properties(GLEW::GLEW
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES        "${_INCLUDE_DIR}"
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_INCLUDE_DIR}"
            IMPORTED_IMPLIB                      "${_IMPLIB}"
            IMPORTED_LOCATION                    "${_IMPORTED_LOCATION}"
            IMPORTED_CONFIGURATIONS              RELEASE
    )
    unset(_INCLUDE_DIR)
    unset(_IMPLIB)
    unset(_IMPORTED_LOCATION)
else(NOT GLEW_FOUND)
    message(STATUS "[GLEW] Glew has been found via `find_package`")
endif(NOT GLEW_FOUND)
