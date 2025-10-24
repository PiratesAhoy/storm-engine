#pragma once

#include "matrix.h"
#include "dx9render.h"
#include "math_inlines.h"

#include <string>
#include <vector>

const int MAX_POINTS = 100;

enum PointType
{
    PT_TYPE_NORMAL,
    PT_TYPE_CANNON_L,
    PT_TYPE_CANNON_R,
    PT_TYPE_CANNON_F,
    PT_TYPE_CANNON_B,

    PT_TYPE_MAST_1,
    PT_TYPE_MAST_2,
    PT_TYPE_MAST_3,
    PT_TYPE_MAST_4,
    PT_TYPE_MAST_5,
    PT_TYPE_NOT_TARGETPOINT
};

#define COLOR_SHADOW D3DCOLOR_ARGB(150, 0, 0, 0);
#define COLOR_SHADOW_SELECTED D3DCOLOR_ARGB(200, 150, 150, 150);
#define COLOR_POINT D3DCOLOR_ARGB(255, 200, 200, 0);
#define COLOR_GRAY D3DCOLOR_ARGB(200, 200, 200, 200);
#define COLOR_SELECTED 0xFFFFFFFF;

//-----------------------------------------------------------------------------------------------
struct Path
{
    uint8_t length{};            // path length (number of elements)
    float min{-1};                 // Path value
    uint8_t point[MAX_POINTS]{}; // Walking sequence
    int currentPointPosition{-1};  // Current position on the way
};

//-----------------------------------------------------------------------------------------------
struct Link
{
    int first{};
    int next{};
};

//-----------------------------------------------------------------------------------------------

class Links
{
public:
    std::vector<Link> link;
    int selected{-1};
    int count{};

    void Add();
    void Delete(int Index);
};

//-----------------------------------------------------------------------------------------------

struct Point
{
    float x{};
    float y{8};
    float z{};
    PointType pointType{PT_TYPE_NORMAL};

    bool buisy{};
    bool disabled{};
    bool cannonReloaded{true};

    int climbPosition{}; // If there is more than one person on the mast

    bool IsMast() const;
    bool IsCannon() const;
    bool IsNormal() const;
};

//-----------------------------------------------------------------------------------------------

struct Points
{
    std::vector<Point> point;

    int count{};
    int selected{-1};

    void Add();
    void Delete(int Index);
};

//-----------------------------------------------------------------------------------------------

class SailorsPoints
{
  private:
    bool PointsPassed[MAX_POINTS]{};        // tmp mark of passed points (for finding a path)
    float matrix[MAX_POINTS][MAX_POINTS]{}; // Matrix for fast path finding


  public:
    Points points;
    Links links;

    void Draw(VDX9RENDER *rs, bool pointmode);
    void Draw_(VDX9RENDER *rs, bool pointmode);
    void DrawLinks(VDX9RENDER *rs);

    Path findPath(Path &path, int from, int to); // Calculate the path
    Path getPath(int src, int dst, int l); // Finding a way

    void UpdateLinks(); // Refresh pathfinder matrix

    int WriteToFile(std::string fileName);
    int ReadFromFile(std::string fileName);
};

//-------------------------------------------------------------------------------------

inline float Dest(const CVECTOR &_v1, const CVECTOR &_v2)
{
    return sqrt((_v2.x - _v1.x) * (_v2.x - _v1.x) + (_v2.y - _v1.y) * (_v2.y - _v1.y) +
                (_v2.z - _v1.z) * (_v2.z - _v1.z));
};

inline bool Dest(const CVECTOR &_v1, const CVECTOR &_v2, float d)
{
    return (fabs(_v2.x - _v1.x) < d && fabs(_v2.y - _v1.y) < d && fabs(_v2.z - _v1.z) < d);
};

inline float Vector2Angle(const CVECTOR &_v)
{
    auto result = atan2(_v.x, _v.z);

    while (result >= PI * 2)
        result -= PI * 2;
    while (result < 0)
        result += PI * 2;

    return result;
}
