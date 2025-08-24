#include "attributes.h"

#include <catch2/catch_all.hpp>

namespace
{

// TODO: Either move STRING_CODEC outside of Engine or make a re-usable codec for testing
class TestStringCodec : public VSTRING_CODEC
{
public:
    uint32_t GetNum() override
    {
        return map_.size();
    }

    uint32_t Convert(const char *pString) override
    {
        std::string str(pString);
        const uint32_t hash = std::hash<std::string>{}(str);
        map_.emplace(hash, str);
        return hash;
    }

    uint32_t Convert(const char *pString, int32_t iLen) override
    {
        std::string str(pString, iLen);
        const uint32_t hash = std::hash<std::string>{}(str);
        map_.emplace(hash, str);
        return hash;
    }

    const char *Convert(uint32_t code) override
    {
        return map_[code].c_str();
    }

    void VariableChanged() override
    {
    }

private:
    std::unordered_map<uint32_t, std::string> map_;
};

} // namespace

TEST_CASE("Match attribute path by pattern", "[config]")
{
    TestStringCodec string_codec{};
    ATTRIBUTES attribute(string_codec);
    ATTRIBUTES *third = attribute.CreateSubAClass(&attribute, "first.second.third");
    ATTRIBUTES *fourth = third->CreateAttribute("fourth", "value");

    CHECK(MatchAttributePath("first.second.third.fourth", *fourth) );
    CHECK_FALSE(MatchAttributePath("first.second.wrong.fourth", *fourth) );
    CHECK_FALSE(MatchAttributePath("third.fourth", *fourth) );
    CHECK_FALSE(MatchAttributePath("*.fourth", *fourth) );
    CHECK_FALSE(MatchAttributePath("third", *fourth) );
    CHECK(MatchAttributePath("first.*.third.fourth", *fourth) );
}
