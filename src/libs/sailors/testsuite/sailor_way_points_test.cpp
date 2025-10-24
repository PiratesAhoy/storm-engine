#include "../src/sailors_way_points.h"

#include <catch2/catch_all.hpp>

namespace
{

SailorsPoints GenerateGrid(const size_t width, const size_t height)
{
    SailorsPoints points;
    points.points.point.reserve(width * height);
    points.links.link.reserve((width - 1) * height + width * (height - 1));
    for (size_t i = 0; i < width; ++i)
    {
        for (size_t j = 0; j < height; ++j)
        {
            points.points.point.emplace_back(Point{static_cast<float>(i), static_cast<float>(j), 0, PT_TYPE_NORMAL});
        }
    }
    points.points.count = points.points.point.size();
    for (size_t i = 0; i < width; ++i)
    {
        for (size_t j = 0; j < height - 1; ++j)
        {
            points.links.link.emplace_back(static_cast<int>(i + j * width), static_cast<int>(i + (j + 1) * width));
        }
    }
    for (size_t i = 0; i < width - 1; ++i)
    {
        for (size_t j = 0; j < height; ++j)
        {
            points.links.link.emplace_back(static_cast<int>(i + j * width), static_cast<int>(i + 1 + j * width));
        }
    }
    points.links.count = points.links.link.size();
    points.UpdateLinks();
    return points;
}

}

TEST_CASE("SailorsPointsFindPath", "[sailors]")
{
    Path path;
    SailorsPoints points = GenerateGrid(3, 3);
    points.findPath(path, 0, 7);
    CHECK(path.min == 3);
    CHECK(path.length == 4);
    CHECK(path.point[0] == 0);
    CHECK(path.point[1] == 1);
    CHECK(path.point[2] == 4);
    CHECK(path.point[3] == 7);
}
