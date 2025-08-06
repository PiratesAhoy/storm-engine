#include "../rsrc/resource.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include <d3d9.h>
#include <filesystem>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_sdl2.h>
#include <iostream>
#include <string>
#include <toml++/toml.h>
#include <unordered_map>
#include <variant>
#include <windows.h>
#pragma comment(lib, "d3d9.lib")

// Globals
LPDIRECT3D9 g_pD3D = NULL;
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
D3DPRESENT_PARAMETERS g_d3dpp = {};
SDL_Window *g_Window = NULL;
SDL_Renderer *g_Renderer = NULL;
SDL_SysWMinfo g_WmInfo;

// Configuration system
enum class ConfigType
{
    STRING,
    INTEGER,
    FLOAT,
    BOOLEAN
};

struct ConfigRestrictions
{
    // For numeric types (int/float)
    std::optional<double> min_value;
    std::optional<double> max_value;

    // For string types
    std::optional<size_t> max_length;
    std::optional<std::vector<std::string>> allowed_values; // Enum-like restriction

    // For all types
    bool read_only = false;

    ConfigRestrictions() = default;

    static ConfigRestrictions Range(double min, double max)
    {
        ConfigRestrictions r;
        r.min_value = min;
        r.max_value = max;
        return r;
    }

    static ConfigRestrictions Min(double min)
    {
        ConfigRestrictions r;
        r.min_value = min;
        return r;
    }

    static ConfigRestrictions Max(double max)
    {
        ConfigRestrictions r;
        r.max_value = max;
        return r;
    }

    static ConfigRestrictions StringLength(size_t max_len)
    {
        ConfigRestrictions r;
        r.max_length = max_len;
        return r;
    }

    static ConfigRestrictions Enum(const std::vector<std::string> &values)
    {
        ConfigRestrictions r;
        r.allowed_values = values;
        return r;
    }

    static ConfigRestrictions ReadOnly()
    {
        ConfigRestrictions r;
        r.read_only = true;
        return r;
    }
};

struct ConfigValue
{
    std::string name;
    std::string description;
    ConfigType type;
    std::variant<std::string, int64_t, double, bool> default_value;
    std::variant<std::string, int64_t, double, bool> current_value;
    ConfigRestrictions restrictions;

    ConfigValue(const std::string &n, const std::string &desc, ConfigType t,
                const std::variant<std::string, int64_t, double, bool> &def_val,
                const ConfigRestrictions &restr = ConfigRestrictions{})
        : name(n)
        , description(desc)
        , type(t)
        , default_value(def_val)
        , current_value(def_val)
        , restrictions(restr)
    {
    }

    // Validation methods
    bool IsValid(const std::variant<std::string, int64_t, double, bool> &value) const
    {
        switch (type)
        {
        case ConfigType::INTEGER: {
            int64_t int_val = std::get<int64_t>(value);
            if (restrictions.min_value && int_val < *restrictions.min_value)
                return false;
            if (restrictions.max_value && int_val > *restrictions.max_value)
                return false;
            break;
        }
        case ConfigType::FLOAT: {
            double float_val = std::get<double>(value);
            if (restrictions.min_value && float_val < *restrictions.min_value)
                return false;
            if (restrictions.max_value && float_val > *restrictions.max_value)
                return false;
            break;
        }
        case ConfigType::STRING: {
            const std::string &str_val = std::get<std::string>(value);
            if (restrictions.max_length && str_val.length() > *restrictions.max_length)
                return false;
            if (restrictions.allowed_values)
            {
                auto &allowed = *restrictions.allowed_values;
                if (std::find(allowed.begin(), allowed.end(), str_val) == allowed.end())
                {
                    return false;
                }
            }
            break;
        }
        case ConfigType::BOOLEAN:
            // Booleans don't need validation
            break;
        }
        return true;
    }

    std::variant<std::string, int64_t, double, bool> ClampValue(
        const std::variant<std::string, int64_t, double, bool> &value) const
    {
        switch (type)
        {
        case ConfigType::INTEGER: {
            int64_t int_val = std::get<int64_t>(value);
            if (restrictions.min_value)
                int_val = std::max(int_val, (int64_t)*restrictions.min_value);
            if (restrictions.max_value)
                int_val = std::min(int_val, (int64_t)*restrictions.max_value);
            return int_val;
        }
        case ConfigType::FLOAT: {
            double float_val = std::get<double>(value);
            if (restrictions.min_value)
                float_val = std::max(float_val, *restrictions.min_value);
            if (restrictions.max_value)
                float_val = std::min(float_val, *restrictions.max_value);
            return float_val;
        }
        case ConfigType::STRING: {
            std::string str_val = std::get<std::string>(value);
            if (restrictions.max_length && str_val.length() > *restrictions.max_length)
            {
                str_val = str_val.substr(0, *restrictions.max_length);
            }
            if (restrictions.allowed_values)
            {
                auto &allowed = *restrictions.allowed_values;
                if (std::find(allowed.begin(), allowed.end(), str_val) == allowed.end())
                {
                    // Return first allowed value if current is invalid
                    if (!allowed.empty())
                        return allowed[0];
                }
            }
            return str_val;
        }
        case ConfigType::BOOLEAN:
            return value; // No clamping needed for booleans
        }
        return value;
    }
};

class ConfigManager
{
  private:
    std::unordered_map<std::string, ConfigValue> config_values;
    std::string config_file_path;
    bool config_loaded;

  public:
    ConfigManager()
        : config_file_path("config.toml")
        , config_loaded(false)
    {
        // Define configuration schema with default values and restrictions
        AddConfigValue("app.title", "Application Title", ConfigType::STRING, std::string("Enhanced ImGui App"),
                       ConfigRestrictions::StringLength(100));
        AddConfigValue("app.version", "Application Version", ConfigType::STRING, std::string("1.0.0"),
                       ConfigRestrictions::ReadOnly());
        AddConfigValue("app.debug_mode", "Enable Debug Mode", ConfigType::BOOLEAN, false);

        AddConfigValue("window.width", "Window Width", ConfigType::INTEGER, int64_t(800),
                       ConfigRestrictions::Range(400, 3840));
        AddConfigValue("window.height", "Window Height", ConfigType::INTEGER, int64_t(600),
                       ConfigRestrictions::Range(300, 2160));
        AddConfigValue("window.fullscreen", "Start Fullscreen", ConfigType::BOOLEAN, false);

        AddConfigValue("graphics.vsync", "Enable VSync", ConfigType::BOOLEAN, true);
        AddConfigValue("graphics.fps_limit", "FPS Limit", ConfigType::INTEGER, int64_t(60),
                       ConfigRestrictions::Range(30, 300));
        AddConfigValue("graphics.render_scale", "Render Scale", ConfigType::FLOAT, 1.0,
                       ConfigRestrictions::Range(0.1, 4.0));

        AddConfigValue("ui.font_size", "UI Font Size", ConfigType::FLOAT, 25.0, ConfigRestrictions::Range(8.0, 72.0));
        AddConfigValue("ui.theme", "UI Theme", ConfigType::STRING, std::string("dark"),
                       ConfigRestrictions::Enum({"dark", "light", "classic"}));
        AddConfigValue("ui.show_fps", "Show FPS Counter", ConfigType::BOOLEAN, true);

        // Additional examples with various restrictions
        AddConfigValue("audio.master_volume", "Master Volume", ConfigType::FLOAT, 0.8,
                       ConfigRestrictions::Range(0.0, 1.0));
        AddConfigValue("network.max_connections", "Max Network Connections", ConfigType::INTEGER, int64_t(10),
                       ConfigRestrictions::Range(1, 100));
        AddConfigValue("user.username", "Username", ConfigType::STRING, std::string("Player"),
                       ConfigRestrictions::StringLength(20));
    }

    void AddConfigValue(const std::string &key, const std::string &description, ConfigType type,
                        const std::variant<std::string, int64_t, double, bool> &default_val,
                        const ConfigRestrictions &restrictions = ConfigRestrictions{})
    {
        config_values.emplace(key, ConfigValue(key, description, type, default_val, restrictions));
    }

    bool LoadConfig()
    {
        try
        {
            if (!std::filesystem::exists(config_file_path))
            {
                std::cout << "Config file doesn't exist, using defaults and creating: " << config_file_path
                          << std::endl;
                SaveConfig(); // Create config file with defaults
                config_loaded = true;
                return true;
            }

            auto config = toml::parse_file(config_file_path);

            for (auto &[key, config_val] : config_values)
            {
                try
                {
                    auto node = config.at_path(key);
                    if (node)
                    {
                        switch (config_val.type)
                        {
                        case ConfigType::STRING:
                            if (auto str_val = node.value<std::string>())
                            {
                                auto clamped = config_val.ClampValue(*str_val);
                                config_val.current_value = clamped;
                            }
                            break;
                        case ConfigType::INTEGER:
                            if (auto int_val = node.value<int64_t>())
                            {
                                auto clamped = config_val.ClampValue(*int_val);
                                config_val.current_value = clamped;
                            }
                            break;
                        case ConfigType::FLOAT:
                            if (auto float_val = node.value<double>())
                            {
                                auto clamped = config_val.ClampValue(*float_val);
                                config_val.current_value = clamped;
                            }
                            break;
                        case ConfigType::BOOLEAN:
                            if (auto bool_val = node.value<bool>())
                            {
                                config_val.current_value = *bool_val;
                            }
                            break;
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    std::cout << "Warning: Failed to load config value '" << key << "': " << e.what() << std::endl;
                }
            }

            config_loaded = true;
            std::cout << "Successfully loaded config from: " << config_file_path << std::endl;
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error loading config: " << e.what() << std::endl;
            config_loaded = false;
            return false;
        }
    }

    bool SaveConfig()
    {
        try
        {
            toml::table config_table;

            for (const auto &[key, config_val] : config_values)
            {
                std::vector<std::string> path_parts;
                std::string current_part;

                // Split the key by '.' to create nested tables
                for (char c : key)
                {
                    if (c == '.')
                    {
                        if (!current_part.empty())
                        {
                            path_parts.push_back(current_part);
                            current_part.clear();
                        }
                    }
                    else
                    {
                        current_part += c;
                    }
                }
                if (!current_part.empty())
                {
                    path_parts.push_back(current_part);
                }

                // Navigate/create the nested structure
                toml::table *current_table = &config_table;
                for (size_t i = 0; i < path_parts.size() - 1; ++i)
                {
                    auto it = current_table->find(path_parts[i]);
                    if (it == current_table->end() || !it->second.is_table())
                    {
                        current_table->insert_or_assign(path_parts[i], toml::table{});
                    }
                    current_table = current_table->get(path_parts[i])->as_table();
                }

                // Insert the value
                const std::string &final_key = path_parts.back();
                std::visit([&](const auto &value) { current_table->insert_or_assign(final_key, value); },
                           config_val.current_value);
            }

            std::ofstream config_file(config_file_path);
            config_file << config_table << std::endl;

            std::cout << "Config saved to: " << config_file_path << std::endl;
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error saving config: " << e.what() << std::endl;
            return false;
        }
    }

    template <typename T> T GetValue(const std::string &key) const
    {
        auto it = config_values.find(key);
        if (it != config_values.end())
        {
            return std::get<T>(it->second.current_value);
        }
        throw std::runtime_error("Config key not found: " + key);
    }

    template <typename T> void SetValue(const std::string &key, const T &value)
    {
        auto it = config_values.find(key);
        if (it != config_values.end())
        {
            if (!it->second.restrictions.read_only)
            {
                auto clamped_value = it->second.ClampValue(value);
                it->second.current_value = clamped_value;
            }
        }
    }

    const std::unordered_map<std::string, ConfigValue> &GetAllValues() const
    {
        return config_values;
    }

    bool IsConfigLoaded() const
    {
        return config_loaded;
    }
};

// Global config manager
ConfigManager g_Config;

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
    {
        std::cerr << "Failed to create Direct3D9 object" << std::endl;
        return false;
    }

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // Use config for VSync setting
    try
    {
        bool vsync = g_Config.GetValue<bool>("graphics.vsync");
        g_d3dpp.PresentationInterval = vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
    }
    catch (...)
    {
        g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }

    HRESULT hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                      &g_d3dpp, &g_pd3dDevice);

    // Fallback to software vertex processing if hardware fails
    if (FAILED(hr))
    {
        hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                  &g_d3dpp, &g_pd3dDevice);
        if (FAILED(hr))
        {
            std::cerr << "Failed to create D3D device" << std::endl;
            return false;
        }
    }

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = NULL;
    }
    if (g_pD3D)
    {
        g_pD3D->Release();
        g_pD3D = NULL;
    }
}

bool ResetDevice()
{
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (FAILED(hr))
    {
        std::cerr << "Failed to reset D3D device" << std::endl;
        return false;
    }
    return true;
}

void LoadImGuiFontFromResource(ImGuiIO &io, float font_size)
{
    // 1. Get the EXE module handle
    HMODULE hModule = GetModuleHandle(NULL);

    // 2. Find the font resource
    HRSRC hRes = FindResource(NULL, L"IDR_FONT_ROBOTOREGULAR", RT_RCDATA);
    if (!hRes)
    {
        DWORD err = GetLastError();
        char buf[256];
        sprintf(buf, "FindResource failed (error %d)\n", err);
        OutputDebugStringA(buf);
        std::cout << buf << std::endl;
        return;
    }

    // 3. Load and lock it
    HGLOBAL hResLoad = LoadResource(hModule, hRes);
    if (!hResLoad)
    {
        OutputDebugStringA("Failed to load font resource\n");
        return;
    }

    void *pData = LockResource(hResLoad);
    DWORD dataSize = SizeofResource(hModule, hRes);
    if (!pData || dataSize == 0)
    {
        OutputDebugStringA("Invalid font resource data\n");
        return;
    }

    // 4. Add it to ImGui
    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false; // We manage the memory, not ImGui

    ImFont *font = io.Fonts->AddFontFromMemoryTTF(pData,         // pointer to TTF data
                                                  (int)dataSize, // size of the TTF data
                                                  font_size,     // font size from config
                                                  &fontConfig);

    if (!font)
    {
        OutputDebugStringA("Failed to load font into ImGui\n");
    }
    std::cout << "Successfully loaded font resource with size: " << font_size << std::endl;
}

void RenderConfigUI()
{
    if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Config Status: %s", g_Config.IsConfigLoaded() ? "Loaded" : "Not Loaded");

        if (ImGui::Button("Reload Config"))
        {
            g_Config.LoadConfig();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Config"))
        {
            g_Config.SaveConfig();
        }

        ImGui::Separator();

        // Group configurations by category
        std::unordered_map<std::string, std::vector<std::string>> categories;
        for (const auto &[key, config_val] : g_Config.GetAllValues())
        {
            size_t dot_pos = key.find('.');
            std::string category = (dot_pos != std::string::npos) ? key.substr(0, dot_pos) : "General";
            categories[category].push_back(key);
        }

        for (const auto &[category, keys] : categories)
        {
            if (ImGui::TreeNode(category.c_str()))
            {
                for (const std::string &key : keys)
                {
                    const auto &config_val = g_Config.GetAllValues().at(key);

                    // Extract the display name (part after the last dot)
                    std::string display_name = key;
                    size_t last_dot = key.find_last_of('.');
                    if (last_dot != std::string::npos)
                    {
                        display_name = key.substr(last_dot + 1);
                    }

                    ImGui::PushID(key.c_str());

                    // Check if read-only
                    if (config_val.restrictions.read_only)
                    {
                        ImGui::BeginDisabled();
                    }

                    switch (config_val.type)
                    {
                    case ConfigType::STRING: {
                        std::string current_str = std::get<std::string>(config_val.current_value);

                        // Check if this is an enum-style restriction
                        if (config_val.restrictions.allowed_values)
                        {
                            auto &allowed_values = *config_val.restrictions.allowed_values;
                            int current_item = 0;

                            // Find current selection
                            for (size_t i = 0; i < allowed_values.size(); ++i)
                            {
                                if (allowed_values[i] == current_str)
                                {
                                    current_item = static_cast<int>(i);
                                    break;
                                }
                            }

                            // Create combo box
                            if (ImGui::BeginCombo(display_name.c_str(), current_str.c_str()))
                            {
                                for (size_t i = 0; i < allowed_values.size(); ++i)
                                {
                                    bool is_selected = (current_item == static_cast<int>(i));
                                    if (ImGui::Selectable(allowed_values[i].c_str(), is_selected))
                                    {
                                        g_Config.SetValue(key, allowed_values[i]);
                                    }
                                    if (is_selected)
                                    {
                                        ImGui::SetItemDefaultFocus();
                                    }
                                }
                                ImGui::EndCombo();
                            }
                        }
                        else
                        {
                            // Regular text input
                            char buffer[256];
                            size_t max_len = config_val.restrictions.max_length
                                                 ? std::min(*config_val.restrictions.max_length, sizeof(buffer) - 1)
                                                 : sizeof(buffer) - 1;

                            strncpy(buffer, current_str.c_str(), max_len);
                            buffer[max_len] = '\0';

                            if (ImGui::InputText(display_name.c_str(), buffer, max_len + 1))
                            {
                                g_Config.SetValue(key, std::string(buffer));
                            }
                        }
                        break;
                    }
                    case ConfigType::INTEGER: {
                        int current_int = static_cast<int>(std::get<int64_t>(config_val.current_value));

                        if (config_val.restrictions.min_value || config_val.restrictions.max_value)
                        {
                            int min_val = config_val.restrictions.min_value
                                              ? static_cast<int>(*config_val.restrictions.min_value)
                                              : INT_MIN;
                            int max_val = config_val.restrictions.max_value
                                              ? static_cast<int>(*config_val.restrictions.max_value)
                                              : INT_MAX;

                            if (ImGui::SliderInt(display_name.c_str(), &current_int, min_val, max_val))
                            {
                                g_Config.SetValue(key, static_cast<int64_t>(current_int));
                            }
                        }
                        else
                        {
                            if (ImGui::InputInt(display_name.c_str(), &current_int))
                            {
                                g_Config.SetValue(key, static_cast<int64_t>(current_int));
                            }
                        }
                        break;
                    }
                    case ConfigType::FLOAT: {
                        float current_float = static_cast<float>(std::get<double>(config_val.current_value));

                        if (config_val.restrictions.min_value || config_val.restrictions.max_value)
                        {
                            float min_val = config_val.restrictions.min_value
                                                ? static_cast<float>(*config_val.restrictions.min_value)
                                                : -FLT_MAX;
                            float max_val = config_val.restrictions.max_value
                                                ? static_cast<float>(*config_val.restrictions.max_value)
                                                : FLT_MAX;

                            if (ImGui::SliderFloat(display_name.c_str(), &current_float, min_val, max_val, "%.3f"))
                            {
                                g_Config.SetValue(key, static_cast<double>(current_float));
                            }
                        }
                        else
                        {
                            if (ImGui::InputFloat(display_name.c_str(), &current_float))
                            {
                                g_Config.SetValue(key, static_cast<double>(current_float));
                            }
                        }
                        break;
                    }
                    case ConfigType::BOOLEAN: {
                        bool current_bool = std::get<bool>(config_val.current_value);
                        if (ImGui::Checkbox(display_name.c_str(), &current_bool))
                        {
                            g_Config.SetValue(key, current_bool);
                        }
                        break;
                    }
                    }

                    if (config_val.restrictions.read_only)
                    {
                        ImGui::EndDisabled();
                    }

                    // Show description and restrictions as tooltip
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", config_val.description.c_str());

                        // Show restrictions info
                        if (config_val.restrictions.read_only)
                        {
                            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Read-only");
                        }
                        if (config_val.restrictions.min_value || config_val.restrictions.max_value)
                        {
                            if (config_val.restrictions.min_value && config_val.restrictions.max_value)
                            {
                                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Range: %.3f - %.3f",
                                                   *config_val.restrictions.min_value,
                                                   *config_val.restrictions.max_value);
                            }
                            else if (config_val.restrictions.min_value)
                            {
                                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Min: %.3f",
                                                   *config_val.restrictions.min_value);
                            }
                            else if (config_val.restrictions.max_value)
                            {
                                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Max: %.3f",
                                                   *config_val.restrictions.max_value);
                            }
                        }
                        if (config_val.restrictions.max_length)
                        {
                            ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Max length: %zu",
                                               *config_val.restrictions.max_length);
                        }
                        if (config_val.restrictions.allowed_values)
                        {
                            ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Allowed values:");
                            for (const auto &val : *config_val.restrictions.allowed_values)
                            {
                                ImGui::Text("  - %s", val.c_str());
                            }
                        }
                        ImGui::EndTooltip();
                    }

                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
        }
    }
}

int main(int, char **)
{
    // Load configuration first
    g_Config.LoadConfig();

    // SDL and window init
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Get window dimensions from config
    int window_width = static_cast<int>(g_Config.GetValue<int64_t>("window.width"));
    int window_height = static_cast<int>(g_Config.GetValue<int64_t>("window.height"));
    std::string window_title = g_Config.GetValue<std::string>("app.title");

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_ALLOW_HIGHDPI);
    if (g_Config.GetValue<bool>("window.fullscreen"))
    {
        window_flags = (SDL_WindowFlags)(window_flags | SDL_WINDOW_FULLSCREEN);
    }

    g_Window = SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width,
                                window_height, window_flags);

    if (!g_Window)
    {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(g_Window, &wmInfo))
    {
        std::cerr << "Failed to get window info: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(g_Window);
        SDL_Quit();
        return -1;
    }

    HWND hwnd = wmInfo.info.win.window;

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        SDL_DestroyWindow(g_Window);
        SDL_Quit();
        return 1;
    }

    // Dear ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard navigation

    // Set theme based on config
    std::string theme = g_Config.GetValue<std::string>("ui.theme");
    if (theme == "dark")
    {
        ImGui::StyleColorsDark();
    }
    else if (theme == "light")
    {
        ImGui::StyleColorsLight();
    }
    else
    {
        ImGui::StyleColorsClassic();
    }

    if (!ImGui_ImplSDL2_InitForD3D(g_Window))
    {
        std::cerr << "Failed to initialize ImGui SDL2 backend" << std::endl;
        CleanupDeviceD3D();
        SDL_DestroyWindow(g_Window);
        SDL_Quit();
        return -1;
    }

    if (!ImGui_ImplDX9_Init(g_pd3dDevice))
    {
        std::cerr << "Failed to initialize ImGui DX9 backend" << std::endl;
        ImGui_ImplSDL2_Shutdown();
        CleanupDeviceD3D();
        SDL_DestroyWindow(g_Window);
        SDL_Quit();
        return -1;
    }

    // Application state
    std::string label_text = "Hello!";
    int click_count = 0;

    // Load font with size from config
    float font_size = static_cast<float>(g_Config.GetValue<double>("ui.font_size"));
    LoadImGuiFontFromResource(io, font_size);

    // FPS tracking
    float fps = 0.0f;
    Uint32 frame_count = 0;
    Uint32 last_time = SDL_GetTicks();

    // Main loop
    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(g_Window))
                done = true;
        }

        // Calculate FPS
        frame_count++;
        Uint32 current_time = SDL_GetTicks();
        if (current_time - last_time >= 1000)
        {
            fps = frame_count / ((current_time - last_time) / 1000.0f);
            frame_count = 0;
            last_time = current_time;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Create a fullscreen ImGui window
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags_imgui = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                              ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                              ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("MainWindow", nullptr, window_flags_imgui);

        // App info section
        ImGui::Text("Application: %s", g_Config.GetValue<std::string>("app.title").c_str());
        ImGui::Text("Version: %s", g_Config.GetValue<std::string>("app.version").c_str());

        if (g_Config.GetValue<bool>("ui.show_fps"))
        {
            ImGui::SameLine();
            ImGui::Text("| FPS: %.1f", fps);
        }

        ImGui::Separator();

        // Original demo section
        ImGui::Text("%s", label_text.c_str());
        ImGui::Text("Button clicked %d times", click_count);

        if (ImGui::Button("Click Me"))
        {
            click_count++;
            label_text = "You clicked the button " + std::to_string(click_count) + " time(s)!";
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset"))
        {
            click_count = 0;
            label_text = "Hello!";
        }

        ImGui::Separator();

        // Configuration UI
        RenderConfigUI();

        ImGui::Separator();

        if (ImGui::Button("Quit"))
        {
            done = true;
        }

        ImGui::End();

        // Rendering
        ImGui::EndFrame();

        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(45, 45, 48, 255), 1.0f, 0);

        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }

        // Handle device lost
        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
        if (result == D3DERR_DEVICELOST)
        {
            if (g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            {
                ImGui_ImplDX9_InvalidateDeviceObjects();
                if (ResetDevice())
                {
                    ImGui_ImplDX9_CreateDeviceObjects();
                }
            }
        }
    }

    // Save config before exit
    g_Config.SaveConfig();

    // Cleanup
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    SDL_DestroyWindow(g_Window);
    SDL_Quit();

    return 0;
}