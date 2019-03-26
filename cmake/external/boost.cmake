
find_package(Boost
    QUIET
    COMPONENTS
        system
        filesystem
)
if(NOT Boost_FOUND)
    message(STATUS "[boost] Boost has not been found via `find_package`, fetching it from source...")
    include(FetchContent)

    FetchContent_Declare(boost
        GIT_REPOSITORY git@github.com:Orphis/boost-cmake.git
        GIT_TAG        v1.67.0
    )

    FetchContent_GetProperties(boost)
    if(NOT boost_POPULATED)
        FetchContent_Populate(boost)
    endif()

    add_subdirectory(${boost_SOURCE_DIR} ${boost_BINARY_DIR})
else(NOT Boost_FOUND)
    message(STATUS "[boost] Boost has been found via `find_package`.")
endif(NOT Boost_FOUND)
