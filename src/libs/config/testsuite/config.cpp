#include "storm/config/config.hpp"

#include <catch2/catch_all.hpp>

using namespace storm;

TEST_CASE("Case insensitive keys", "[config]")
{
    Data config;
    config["Key"] = 5;

    CHECK(config.contains("key"));
    CHECK(config.contains("KEY"));
    CHECK(!config.contains("Kay"));
    CHECK(config["key"] == 5);
    CHECK(config["KEY"] == 5);
}
