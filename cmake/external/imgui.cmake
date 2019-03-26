
find_package(imgui
    QUIET
)
if(NOT imgui_FOUND)
    message(STATUS "[imgui] imgui has not been found via `find_package`, fetching it from source...")
    include(FetchContent)

    FetchContent_Declare(
        imgui
        GIT_REPOSITORY "git@github.com:ocornut/imgui.git"
        GIT_TAG        v1.66

    )

    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
      FetchContent_Populate(imgui)
    endif()

    add_library(_imgui STATIC)
    add_library(imgui::imgui ALIAS _imgui)

    target_sources(_imgui
        PRIVATE
            "${imgui_SOURCE_DIR}/imgui.cpp"
            "${imgui_SOURCE_DIR}/imgui_demo.cpp"
            "${imgui_SOURCE_DIR}/imgui_draw.cpp"
            "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    )

    target_include_directories(_imgui SYSTEM
        PUBLIC
            "${imgui_SOURCE_DIR}"
    )
else(NOT imgui_FOUND)
    message(STATUS "[imgui] imgui has been found via `find_package`")
endif(NOT imgui_FOUND)
