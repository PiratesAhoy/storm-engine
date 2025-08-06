add_executable(configapp
    configapp.cpp
    ../rsrc/resource.rc
)

if (NOT WIN32)
    set(SYSTEM_DEPS "${NATIVE_D3D9_LIBS}")
endif()

set(IMGUI_BINGINDS_DIR "${CMAKE_BINARY_DIR}/imgui")

# TODO: imgui_backend is defined in `libs\editor\CMakeLists.txt`, but
#  we shouldn't assume it will always load before this file. Move that
#  definition to a higher CMakeLists file.

#add_library(imgui_backend STATIC
#    "${IMGUI_BINGINDS_DIR}/imgui_impl_sdl2.cpp"
#    "${IMGUI_BINGINDS_DIR}/imgui_stdlib.cpp"
#)

if (WIN32)
    target_sources(imgui_backend PRIVATE "${IMGUI_BINGINDS_DIR}/imgui_impl_dx9.cpp")
endif()

target_include_directories(imgui_backend PUBLIC ${IMGUI_BINGINDS_DIR})
target_link_libraries(imgui_backend PUBLIC imgui ${SDL2_LIBRARIES} ${SYSTEM_DEPS})

target_link_libraries(configapp PRIVATE imgui_backend tomlplusplus)

#set_target_properties(configapp PROPERTIES
#    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
#    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}
#    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}
#)