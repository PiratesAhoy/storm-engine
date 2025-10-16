STORM_SETUP(
    TARGET_NAME core
    TYPE library
    DEPENDENCIES diagnostics editor math shared_headers sound steam_api FastFloat::fast_float ${SDL2_LIBRARIES} window tomlplusplus::tomlplusplus nlohmann_json::nlohmann_json
)
