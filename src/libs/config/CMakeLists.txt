STORM_SETUP(
        TARGET_NAME config
        TYPE storm_module
        DEPENDENCIES core
        TEST_DEPENDENCIES Catch2::Catch2WithMain
)
