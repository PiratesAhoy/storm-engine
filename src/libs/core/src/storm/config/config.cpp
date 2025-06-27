#include "storm/config/config.hpp"
#include "file_service.h"

#include <toml++/toml.h>

namespace storm
{

namespace
{

[[nodiscard]] ConfigFormat GetPreferredConfigFormat(const std::filesystem::path &source_path)
{
    if (source_path.has_extension())
    {
        const std::string extension = source_path.extension().string();
        if (extension == ".toml")
        {
            return ConfigFormat::Toml;
        }
        else if (extension == ".ini")
        {
            return ConfigFormat::Ini;
        }
    }

    return ConfigFormat::Toml;
}

std::string_view GetConfigFileExtension(ConfigFormat format)
{
    switch (format)
    {
    case Toml:
        return ".toml";
    case Ini:
        return ".ini";
    default:
        return "";
    }
}

[[nodiscard]] std::filesystem::path GetConfigFilePath(const std::filesystem::path &source_path, ConfigFormat format)
{
    const std::filesystem::path stem = source_path.stem();
    const std::filesystem::path directory = source_path.parent_path();

    return directory / (stem.string() + std::string(GetConfigFileExtension(format)));
}

Data::object_t ConvertTable(const toml::table &toml);
Data::array_t ConvertArray(const toml::array &array);

Data::value_type ConvertValue(const toml::node &toml)
{
    if (toml.is_string()) {
        return **toml.as_string();
    }
    else if (toml.is_boolean()) {
        return **toml.as_boolean();
    }
    else if (toml.is_integer()) {
        return **toml.as_integer();
    }
    else if (toml.is_floating_point()) {
        return **toml.as_floating_point();
    }
    else if (toml.is_array()) {
        return ConvertArray(*toml.as_array());
    }
    else if (toml.is_table()) {
        return ConvertTable(*toml.as_table());
    }
    return {};
}

Data::array_t ConvertArray(const toml::array &array)
{
    Data::array_t result;
    for (const auto &value : array) {
        result.push_back(ConvertValue(value));
    }
    return result;
}

Data::object_t ConvertTable(const toml::table &toml)
{
    Data::object_t result;
    for (const auto &entry : toml) {
        const auto &key = std::string(entry.first);
        const auto &value = entry.second;
        result[key] = ConvertValue(value);
    }
    return result;
}

Data ToConfig(const toml::table &toml)
{
    return ConvertValue(toml);
}

} // namespace

std::optional<FindConfigResult> FindConfigFile(const std::filesystem::path &source_path)
{
    ConfigFormat preferred_format = GetPreferredConfigFormat(source_path);

    const std::filesystem::path first_test_path = GetConfigFilePath(source_path, preferred_format);
    if (exists(first_test_path))
    {
        return FindConfigResult{
            preferred_format,
            first_test_path,
        };
    }

    for (const auto format : {ConfigFormat::Toml, ConfigFormat::Ini})
    {
        if (format == preferred_format)
        {
            continue;
        }
        const std::filesystem::path test_path = GetConfigFilePath(source_path, format);
        if (exists(test_path))
        {
            return FindConfigResult{
                format,
                test_path,
            };
        }
    }

    return {};
}

std::optional<Data> LoadConfig(const std::filesystem::path &file_path)
{
    auto found_config = FindConfigFile(file_path);
    if (!found_config)
    {
        return {};
    }

    const auto &[format, path] = *found_config;

    switch (format)
    {
    case Toml: {
        return ToConfig(toml::parse_file(path.string()));
    }
    case Ini: {
        const std::string path_str = path.string();
        const auto ini = fio->OpenIniFile(path_str.c_str());
        if (ini == nullptr)
            return {};
        return ini->ToData();
    }
    default:
        return {};
    }
}

namespace config {

std::optional<uint32_t> GetColor(const storm::Data &node)
{
    if (node.is_number_integer()) {
        return node.get<uint32_t>();
    }
    else if (node.is_string()) {
        const auto value = node.get<std::string>();
        if (value.starts_with('#') ) {
            const auto code = std::string_view(value.begin() + 1, value.end());
            if (value.length() == 7) {
                uint32_t result;
                const char *first = code.data();
                const char *last = first + code.length();
                const auto parse_result = std::from_chars(first, last, result, 16);
                if (parse_result.ptr == last) {
                    return result | 0xff000000;
                }
            }
            else if (value.length() == 9) {
                uint32_t result;
                const char *first = code.data();
                const char *last = first + code.length();
                const auto parse_result = std::from_chars(first, last, result, 16);
                if (parse_result.ptr == last) {
                    return result;
                }
            }
        }
    }
    return {};
}

} // namespace config

} // namespace storm
