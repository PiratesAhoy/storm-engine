#pragma once

#include <istring.hpp>

#include <nlohmann/json.hpp>

#include <map>

namespace storm
{

struct DataCompare
{
    constexpr bool operator()(const std::string_view &first, const std::string_view &second) const noexcept
    {
        return traits_cast<ichar_traits<char>>(first) < traits_cast<ichar_traits<char>>(second);
    }
};

template <typename U, typename V, typename... Args> using DataObject = std::map<U, V, DataCompare>;

using Data = nlohmann::basic_json<DataObject>;

} // namespace storm

class ATTRIBUTES;
void to_json(storm::Data &data, const ATTRIBUTES &attributes);