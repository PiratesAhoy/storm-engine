#pragma once

#include <storm/data.hpp>

#include <filesystem>
#include <map>

namespace storm {

enum ConfigFormat {
    Ini,
    Toml,
};

struct FindConfigResult
{
    ConfigFormat format;
    const std::filesystem::path path;
};

[[nodiscard]] std::optional<FindConfigResult> FindConfigFile(const std::filesystem::path &source_path);

[[nodiscard]] std::optional<Data> LoadConfig(const std::filesystem::path &file_path);

[[nodiscard]] bool SaveConfig(const std::filesystem::path &file_path, const Data &data);

namespace config {

std::optional<uint32_t> GetColor(const Data &node);

} // namespace config

} // namespace storm
