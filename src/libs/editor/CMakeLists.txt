if (NOT WIN32)
    set(SYSTEM_DEPS "${NATIVE_D3D9_LIBS}")
endif()

STORM_SETUP(
        TARGET_NAME editor
        TYPE storm_module
        DEPENDENCIES core
        TEST_DEPENDENCIES catch2
)

### ImGui
if (ImGui_FOUND)
    set(IMGUI_BINGINDS_DIR "${CMAKE_BINARY_DIR}/imgui")

    add_library(imgui_backend STATIC
            "${IMGUI_BINGINDS_DIR}/imgui_impl_sdl2.cpp"
            "${IMGUI_BINGINDS_DIR}/imgui_stdlib.cpp"
    )

    if (WIN32)
        target_sources(imgui_backend PRIVATE "${IMGUI_BINGINDS_DIR}/imgui_impl_dx9.cpp")
    endif()

    target_include_directories(imgui_backend PUBLIC ${IMGUI_BINGINDS_DIR})
    target_link_libraries(imgui_backend PUBLIC imgui::imgui ${SDL2_LIBRARIES} ${SYSTEM_DEPS})

    target_link_libraries(editor PUBLIC imgui_backend)
endif()
###
