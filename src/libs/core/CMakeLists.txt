STORM_SETUP(
    TARGET_NAME core
    TYPE library
    DEPENDENCIES diagnostics editor math shared_headers steam_api fast_float ${SDL2_LIBRARIES} window tomlplusplus nlohmann_json
)
