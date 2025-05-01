#pragma once

#include <span>
#include <string_view>
#include <vector>

namespace storm
{

class AudioResource final
{
public:
    [[nodiscard]] int32_t GetSampleRate() const
    {
        return sampleRate_;
    }

    [[nodiscard]] std::span<const int16_t> GetSampleData() const
    {
        return samples_;
    }

// private:
    int32_t sampleRate_{};
    std::vector<int16_t> samples_;
};

class WavLoader final
{
public:
    AudioResource LoadFromFile(const std::string_view &name);

private:

};

}