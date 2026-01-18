#include "walk_graph.hpp"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <array>
#include <vector>

namespace
{
bool Contains(const std::vector<long> &values, long target)
{
    return std::find(values.begin(), values.end(), target) != values.end();
}
} // namespace

TEST_CASE("WalkGraph pair queries", "[legacy_sailors]")
{
    WalkGraph graph;

    SECTION("TestPair checks both directions")
    {
        graph.AddPair(1, 2, 10, 11);

        CHECK(graph.TestPair(1, 2));
        CHECK(graph.TestPair(2, 1));
        CHECK_FALSE(graph.TestPair(1, 3));
    }

    SECTION("Clear removes pairs")
    {
        graph.AddPair(1, 2, 10, 11);
        graph.Clear();

        CHECK_FALSE(graph.TestPair(1, 2));
    }
}

TEST_CASE("WalkGraph random queries return valid candidates", "[legacy_sailors]")
{
    WalkGraph graph;

    SECTION("FindRandomWithType returns -1 when empty")
    {
        CHECK(graph.FindRandomWithType(5) == -1);
    }

    SECTION("FindRandomWithType returns only matching endpoints")
    {
        graph.AddPair(1, 2, 5, 6);
        graph.AddPair(3, 4, 7, 5);

        const std::vector<long> expected{1, 4};
        const auto value = graph.FindRandomWithType(5);
        CHECK(Contains(expected, value));
    }

    SECTION("FindRandomLinkedWithType filters by type and source")
    {
        graph.AddPair(1, 2, 10, 20);
        graph.AddPair(2, 3, 30, 40);
        graph.AddPair(4, 4, 20, 20);

        CHECK(graph.FindRandomLinkedWithType(1, 20) == 2);
        CHECK(graph.FindRandomLinkedWithType(2, 10) == 1);
        CHECK(graph.FindRandomLinkedWithType(1, 10) == -1);
    }

    SECTION("FindRandomLinkedWithoutType filters by non-matching type")
    {
        graph.AddPair(1, 2, 10, 20);
        graph.AddPair(2, 3, 30, 40);
        graph.AddPair(4, 4, 10, 20);

        CHECK(graph.FindRandomLinkedWithoutType(1, 20) == -1);
        CHECK(graph.FindRandomLinkedWithoutType(1, 10) == 2);
        CHECK(graph.FindRandomLinkedWithoutType(2, 10) == 3);
    }

    SECTION("FindRandomLinkedAnyType ignores type and excludes self-pairs")
    {
        graph.AddPair(1, 2, 10, 20);
        graph.AddPair(2, 3, 30, 40);
        graph.AddPair(4, 4, 50, 60);

        const std::vector<long> expected{1, 3};
        const auto value = graph.FindRandomLinkedAnyType(2);
        CHECK(Contains(expected, value));
    }
}
