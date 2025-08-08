#include "../rsrc/resource.h"
#include "iniFile.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include <algorithm>
#include <d3d9.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_sdl2.h>
#include <iostream>
#include <map>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <windows.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "shlwapi.lib")

// Changeable settings
#define DEFAULT_WINDOW_WIDTH 400
#define DEFAULT_WINDOW_HEIGHT 600

// Globals
LPDIRECT3D9 g_pD3D = NULL;
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
D3DPRESENT_PARAMETERS g_d3dpp = {};
SDL_Window *g_Window = NULL;
SDL_Renderer *g_Renderer = NULL;
SDL_SysWMinfo g_WmInfo;

// TODO: Introduce Path type with validation
enum class ConfigValueType
{
    String,
    Integer,
    Float,
    Boolean
};

struct ConfigConstraints
{
    // For numeric types
    float minValue = 0.0f;
    float maxValue = 100.0f;

    // For string types
    size_t maxLength = 255;

    // For all types
    std::vector<std::string> allowedValues; // If not empty, restricts to these values
};

struct ConfigValue
{
    std::string section;
    std::string key;
    std::string displayName;
    std::string description;
    ConfigValueType type;
    std::string defaultValue;
    ConfigConstraints constraints;
    bool isAdvanced; // TODO: Hide advanced settings in UI;
                             //       should be toggleable with a checkbox

    // Current values for editing
    std::string stringValue;
    int intValue = 0;
    float floatValue = 0.0f;
    bool boolValue = false;

    ConfigValue(const std::string &sect, const std::string &k, const std::string &display, const std::string &desc,
                ConfigValueType t, const std::string &def, bool isAdvanced = false, const ConfigConstraints &constr = {})
        : section(sect)
        , key(k)
        , displayName(display)
        , description(desc)
        , type(t)
        , defaultValue(def)
        , isAdvanced(isAdvanced)
        , constraints(constr)
    {
        setFromString(def);
    }

    void setFromString(const std::string &value)
    {
        switch (type)
        {
        case ConfigValueType::String:
            stringValue = value;
            break;
        case ConfigValueType::Integer:
            intValue = std::stoi(value.empty() ? "0" : value);
            break;
        case ConfigValueType::Float:
            floatValue = std::stof(value.empty() ? "0.0" : value);
            break;
        case ConfigValueType::Boolean:
            boolValue = (value == "1");
            break;
        }
    }

    std::string toString() const
    {
        switch (type)
        {
        case ConfigValueType::String:
            return stringValue;
        case ConfigValueType::Integer:
            return std::to_string(intValue);
        case ConfigValueType::Float:
            return std::to_string(floatValue);
        case ConfigValueType::Boolean:
            return boolValue ? "1" : "0";
        }
        return "";
    }
};

// Configuration definitions - Add your preset values here
/* std::vector<ConfigValue> g_ConfigDefinitions = {
    // Graphics settings
    ConfigValue("graphics", "resolution_width", "Screen Width", "Horizontal resolution in pixels",
                ConfigValueType::Integer, "1920", {800, 3840}),
    ConfigValue("graphics", "resolution_height", "Screen Height", "Vertical resolution in pixels",
                ConfigValueType::Integer, "1080", {600, 2160}),
    ConfigValue("graphics", "fullscreen", "Fullscreen Mode", "Enable fullscreen display", ConfigValueType::Boolean,
                "false"),
    ConfigValue("graphics", "vsync", "V-Sync", "Enable vertical synchronization", ConfigValueType::Boolean, "true"),
    ConfigValue("graphics", "quality", "Graphics Quality", "Overall graphics quality preset", ConfigValueType::String,
                "High", {0, 0, 0, {"Low", "Medium", "High", "Ultra"}}),

    // Audio settings
    ConfigValue("audio", "master_volume", "Master Volume", "Overall volume level (0-100)", ConfigValueType::Float,
                "75.0", {0.0f, 100.0f}),
    ConfigValue("audio", "music_volume", "Music Volume", "Background music volume (0-100)", ConfigValueType::Float,
                "50.0", {0.0f, 100.0f}),
    ConfigValue("audio", "sfx_volume", "Sound Effects Volume", "Sound effects volume (0-100)", ConfigValueType::Float,
                "80.0", {0.0f, 100.0f}),
    ConfigValue("audio", "muted", "Mute Audio", "Disable all audio output", ConfigValueType::Boolean, "false"),

    // Game settings
    ConfigValue("game", "player_name", "Player Name", "Default player name", ConfigValueType::String, "Player",
                {0, 0, 32}),
    ConfigValue("game", "difficulty", "Difficulty Level", "Game difficulty setting", ConfigValueType::String, "Normal",
                {0, 0, 0, {"Easy", "Normal", "Hard", "Nightmare"}}),
    ConfigValue("game", "auto_save", "Auto Save", "Enable automatic saving", ConfigValueType::Boolean, "true"),
    ConfigValue("game", "save_interval", "Save Interval", "Auto-save interval in minutes", ConfigValueType::Integer,
                "5", {1, 60}),

    // Network settings
    ConfigValue("network", "server_port", "Server Port", "Default server port", ConfigValueType::Integer, "7777",
                {1024, 65535}),
    ConfigValue("network", "max_players", "Max Players", "Maximum number of players", ConfigValueType::Integer, "8",
                {1, 64}),
    ConfigValue("network", "timeout", "Connection Timeout", "Network timeout in seconds", ConfigValueType::Float,
                "30.0", {5.0f, 120.0f}),
};*/

// TODO: Do validation for loaded values. Could they be found in the file when loading? Are their values valid?
std::vector<ConfigValue> g_ConfigDefinitions = {
    /*
    DEFAULTS:

    Boolean:
    ConfigValue(changethis"SECTION", changethis"KEY", changethis"DisplayName", changethis"Description",
        ConfigValueType::Boolean, changethis"DEFAULTVALUE", changethis bool),
    
    Integer:
    ConfigValue(changethis"SECTION", changethis"KEY", changethis"DisplayName", changethis"Description",
        ConfigValueType::Integer, changethis"DEFAULTVALUE", changethis bool, changethis{}),
    
    Float:
    ConfigValue(changethis"SECTION", changethis"KEY", changethis"DisplayName", changethis"Description",
        ConfigValueType::Float, changethis"DEFAULTVALUE", changethis bool, changethis{}),

    String:
    ConfigValue(changethis"SECTION", changethis"KEY", changethis"DisplayName", changethis"Description",
        ConfigValueType::String, changethis"DEFAULTVALUE", changethis bool, changethis{}),
    */


    // These are usually loaded in `s_device.cpp` or `main.cpp`

    // --- SECTION: UNCATEGORISED ---

    ConfigValue("", "full_screen", "Full Screen On", "Whether the game will be opened full-screen",
        ConfigValueType::Boolean, "0"),
    ConfigValue("", "display", "Display Index", "Index of display to use, 0 = default display",
        ConfigValueType::Integer, "0", true, {0, 5}),
    ConfigValue("", "window_borders", "Window Borders On", "Whether to show window borders in windowed mode",
        ConfigValueType::Boolean, "1"),
    ConfigValue("", "screen_x", "Horizontal Resolution", "Horizontal resolution in pixels",
        ConfigValueType::Integer, "1", false, {500, 7680}),
    ConfigValue("", "screen_y", "Vertical Resolution", "Vertical resolution in pixels",
        ConfigValueType::Integer, "1", false, {250, 4320}),

    // I don't think "driver" is actually checked in the engine, but it is in the .ini

    ConfigValue("", "vsync", "Vsync Enabled", "Whether Vsync is enabled in the game",
        ConfigValueType::Boolean, "1"), 
    ConfigValue("", "msaa", "MSAA", "Multi-Sampling Anti-Aliasing",
        ConfigValueType::Integer, "8", false, {0, 16}),
    ConfigValue("", "lockable_back_buffer", "Lockable Back Buffer", "Whether the\nD3DPRESENTFLAG_LOCKABLE_BACKBUFFER\nflag will be passed to Direct3D",
        ConfigValueType::Boolean, "0", true),
    ConfigValue("", "screen_bpp", "Screen BPP", "Colour format for Direct3D\nD3DFMT_X8R8G8B8 = 32-bit, D3DFMT_R5G6B5 = 16-bit",
        ConfigValueType::String, "D3DFMT_X8R8G8B8", true, {0, 0, 0, {"D3DFMT_A8R8G8B8", "D3DFMT_X8R8G8B8", "D3DFMT_R5G6B5"}}),
    ConfigValue("", "texture_degradation", "Texture Degradation", "Texture quality degradation\n0 = High, 2 = Low",
        ConfigValueType::Integer, "0", false, {0,2}),

    // As of now (2025-aug-08), the post-processing read is commented out in the storm engine source code, and it is
    //  turned off.
    /*
    ConfigValue("", "postprocess", "Post-Processing Enabled", "Whether post-processing is enabled.",
        ConfigValueType::Boolean, "1", false),
    */

    // `controls` only supports "pcs_controls" from what I can see.
    /*
    ConfigValue("", "controls", "Controls", "Control-scheme",
        ConfigValueType::String, "pcs_controls", true, {0,0,0, {"pcs_controls"}}),
    */
    
    // TODO: Do some validation checks
    ConfigValue("", "program_directory", "Program Directory", "The relative path of the directory where\nthe game scripts are. This is\ntraditionally \"PROGRAM\\\"",
        ConfigValueType::String, "PROGRAM\\", true),
    ConfigValue("", "run", "Entry-Point Script", "The .c script file's name (with extension) inside\nthe Program Directory that is the entry script\nby the engine for launching the game.\nThis is traditionally `seadogs.c`.",
        ConfigValueType::String, "seadogs.c", true),
    ConfigValue("", "show_fps", "Show FPS", "Whether the game shows the frame-per-second value",
        ConfigValueType::Boolean, "0", false),
    ConfigValue("", "safe_render", "Safe Render On", "Whether safe mode for rendering is on",
        ConfigValueType::Boolean, "0", true),
    ConfigValue("", "texture_log", "Texture-Related Logging", "Whether to log texture-related operations",
        ConfigValueType::Boolean, "0", true),
    ConfigValue("", "geometry_log", "Geometry-Related Logging", "Whether to log geometry-related operations",
        ConfigValueType::Boolean, "0", true),

    // TODO: Look into the "offclass" parameter. What does it actually do?

    // I don't think the "mem_profile" parameter actually does anything in the engine.

    ConfigValue("", "startFontIniFile", "INI File for Fonts", "The path of the starting\nINI file that handles the fonts\n",
        ConfigValueType::String, "resource\\ini\\fonts_euro.ini", true),
    ConfigValue("", "font", "Default Interface Font", "The default font used by the game's interface",
        ConfigValueType::String, "interface_normal", true),

    // I don't think the "numoftips" parameter actually does anything in the engine.

    ConfigValue("", "tracefilesoff", "Trace Files OFF", "Whether script stack-traces are turned off.\nLook out: True here means that tracing\nis off, and False that it is on.",
        ConfigValueType::Boolean, "1", true),
    ConfigValue("", "run_in_background", "Run in Background", "Whether the game runs in the background\nwhen the game window is not in focus.\nIf off, the game will pause if it loses focus.",
        ConfigValueType::Boolean, "0", false),
    ConfigValue("", "sound_in_background", "Sound in Background On", "Whether the game has sound in the background\nwhen the game window is not in focus.\nIf off, the game will be mute if it loses focus.",
        ConfigValueType::Boolean, "0", false),
    ConfigValue("", "stash_folder", "Stash Folder", "Configure the \"Stash\" folder\nwhere the game will store generated files, defaults to\n\"<Documents>/My Games/Sea Dogs/\"",
        ConfigValueType::String, "stash", true),
    ConfigValue("", "logs_folder", "Logs Folder", "Configure the \"Logs\" folder where\nthe game will write log files, defaults\nto \"<Stash>/Logs/\"",
        ConfigValueType::String, ".", true),
    ConfigValue("", "title", "Window Title", "The title of the game window. You\ncan see this if you hover over the game's icon\non the taskbar, or on the top of your\nwindow if you play in windowed\nmode and the borders are enabled.",
        ConfigValueType::String, "Storm Engine", true),
    
    
    // --- SECTION: COMPATIBILITY ---
    ConfigValue("compatibility", "target_version", "Compatibility Target", "The engine supports multiple games, and each\nhave little differences that the engine\nhas to specifically accomodate for.\nThis option selects the compatibility mode.\nModes:\nsd - Sea Dogs\npotc - Pirates of the Caribbean\nct - Caribbean Tales\ncoas - City of Abandoned Ships\nteho - To Each His Own\nlatest - Beyond New Horizons",
        ConfigValueType::String, "latest", true, {0,0,0, {"latest", "sd", "potc", "ct", "coas", "teho", "latest"}}),


    ConfigValue("interface", "screen_width", "Screen width", "The width of the UI in pixels - DO NOT CHANGE",
        ConfigValueType::Integer, "0", true, {200, 6000}),
    ConfigValue("script", "debuginfo", "Debug info on/off", "Debug info for scripts.",
        ConfigValueType::Boolean, "0"),


};

IniFile g_IniFile;
std::string g_CurrentFileName = "enginetest.ini";
bool g_FileLoaded = false;
bool g_HasUnsavedChanges = false;

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        return false;

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    HRESULT hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                      &g_d3dpp, &g_pd3dDevice);
    if (FAILED(hr))
    {
        hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                  &g_d3dpp, &g_pd3dDevice);
        if (FAILED(hr))
            return false;
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
    return !FAILED(hr);
}

void LoadImGuiFontFromResource(ImGuiIO &io)
{
    HMODULE hModule = GetModuleHandle(NULL);
    HRSRC hRes = FindResource(NULL, L"IDR_FONT_ROBOTOREGULAR", RT_RCDATA);
    if (!hRes)
        return;

    HGLOBAL hResLoad = LoadResource(hModule, hRes);
    if (!hResLoad)
        return;

    void *pData = LockResource(hResLoad);
    DWORD dataSize = SizeofResource(hModule, hRes);
    if (!pData || dataSize == 0)
        return;

    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;

    ImFont *font = io.Fonts->AddFontFromMemoryTTF(pData, (int)dataSize, 16.0f, &fontConfig);
    if (font)
    {
        std::cout << "Successfully loaded font resource." << std::endl;
    }
}

void LoadConfigFromFile()
{
    for (auto &config : g_ConfigDefinitions)
    {
        bool parameterValidlyDefined = g_IniFile.hasKey(config.section, config.key);
        if (!parameterValidlyDefined)
        {
            // TODO: Throw warning to users?
            std::cout << "Config parameter not found: " << config.section << " -> " << config.key
                      << ". Using default value: " << config.defaultValue << std::endl;
            config.setFromString(config.defaultValue);
            continue;
        }

        std::string value = g_IniFile.getValue(config.section, config.key, config.defaultValue);

        std::cout << "Loading config: " << config.section << " -> " << config.key << " = " << value << std::endl;

        config.setFromString(value);
    }
    g_FileLoaded = true;
}

void SaveConfigToFile()
{
    for (const auto &config : g_ConfigDefinitions)
    {
        std::cout << "Saving config: " << config.section << " -> " << config.key << " = " << config.toString()
            << std::endl;

        g_IniFile.setValue(config.section, config.key, config.toString());
    }
    g_IniFile.save();
    g_HasUnsavedChanges = false;
}


void TryLoadIniFile()
{
    if (!g_CurrentFileName.empty())
    {
        g_IniFile.close();
        if (g_IniFile.open(g_CurrentFileName))
        {
            g_FileLoaded = true;
            LoadConfigFromFile();
            g_HasUnsavedChanges = false;
        }
    }
}

void RenderConfigValueTable(ConfigValue &config)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", config.displayName.c_str());
    if (!config.description.empty() && ImGui::IsItemHovered()){
        ImVec2 sp = ImGui::GetCursorScreenPos();
        ImGui::PushTextWrapPos();
        // TODO: Show the original key to the user? Maybe while if they hold Shift?
        ImGui::SetTooltip("%s", config.description.c_str());
        ImGui::PopTextWrapPos();
    }


    ImGui::TableSetColumnIndex(1);
    bool changed = false;
    ImGui::PushID(&config);

    switch (config.type)
    {
    case ConfigValueType::String: {
        ImGui::SetNextItemWidth(-FLT_MIN); // Fill column
        if (!config.constraints.allowedValues.empty())
        {
            auto it = std::find(config.constraints.allowedValues.begin(), config.constraints.allowedValues.end(), config.stringValue);
            int currentItem = (it != config.constraints.allowedValues.end()) ? (int)(it - config.constraints.allowedValues.begin()) : 0;
            if (ImGui::Combo("##combo", &currentItem,
                [](void* data, int idx, const char** out_text) {
                    auto* values = static_cast<std::vector<std::string>*>(data);
                    if (idx >= 0 && idx < (int)values->size()) {
                        *out_text = (*values)[idx].c_str();
                        return true;
                    }
                    return false;
                },
                &config.constraints.allowedValues, (int)config.constraints.allowedValues.size()))
            {
                config.stringValue = config.constraints.allowedValues[currentItem];
                changed = true;
            }
        }
        else
        {
            char buffer[256];
            strncpy_s(buffer, config.stringValue.c_str(), sizeof(buffer) - 1);
            if (ImGui::InputText("##input", buffer, sizeof(buffer)))
            {
                config.stringValue = buffer;
                if (config.stringValue.length() > config.constraints.maxLength)
                    config.stringValue = config.stringValue.substr(0, config.constraints.maxLength);
                changed = true;
            }
        }
        break;
    }
    case ConfigValueType::Integer: {

        // NOTE: Yes, the slider already has a text input, but a regular
        //  user might not know that they have to CTRL+Click to edit it.
        if (ImGui::SliderInt("##slider", &config.intValue, (int)config.constraints.minValue,
                             (int)config.constraints.maxValue, "", ImGuiSliderFlags_NoInput))
        {
            changed = true;
        }

        ImGui::SameLine();

        if (ImGui::InputInt("##input", &config.intValue, 0, 0))
        {
            changed = true;
        
        }
        break;
    }
    case ConfigValueType::Float: {

        if (ImGui::SliderFloat("##slider", &config.floatValue, config.constraints.minValue,
                             config.constraints.maxValue, "", ImGuiSliderFlags_NoInput))
        {
            changed = true;
        }

        ImGui::SameLine();

        if (ImGui::InputFloat("##input", &config.floatValue, 0, 0, "%.3f"))
            changed = true;

        break;
    }
    case ConfigValueType::Boolean: {
        ImGui::SetNextItemWidth(-FLT_MIN); // Fill column
        const char* boolItems[] = { "False", "True" };
        int current = config.boolValue ? 1 : 0;

        if (ImGui::Combo("##boolcombo", &current, boolItems, 2)) {
            config.boolValue = (current == 1);
            changed = true;
        }
        break;
    }
    }

    /*
    ImGui::TableSetColumnIndex(2);
     if (ImGui::SmallButton("Reset"))
    {
        config.setFromString(config.defaultValue);
        changed = true;
    }

    if (changed)
        g_HasUnsavedChanges = true;
    */
    ImGui::PopID();
}

// In RenderConfigEditor, use a table for each section:
void RenderConfigEditor()
{
    ImGui::Text("File: %s", g_CurrentFileName.empty() ? "No file loaded" : g_CurrentFileName.c_str());

    if (ImGui::Button("Load File"))
        TryLoadIniFile();

    ImGui::SameLine();
    if (ImGui::Button("Save File") && g_FileLoaded)
        SaveConfigToFile();

    if (g_HasUnsavedChanges)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "*");
    }

    ImGui::Separator();

    if (!g_FileLoaded)
    {
        ImGui::Text("Loading INI file...");
        TryLoadIniFile();
        return;
    }

    ImGui::BeginChild("ConfigEditorChild", ImVec2(0, 0), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    // Group settings by section
    std::map<std::string, std::vector<ConfigValue *>> sections;
    for (auto &config : g_ConfigDefinitions)
        sections[config.section].push_back(&config);

    for (auto &[sectionName, configs] : sections)
    {
        std::string headerName = sectionName.empty() ? "[UNCATEGORISED]" : sectionName;
        if (ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent();
            if (ImGui::BeginTable("config_table", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
            {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.2f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.2f);
                //ImGui::TableSetupColumn("Manual Control", ImGuiTableColumnFlags_WidthStretch, 0.1f);
                //ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed, 60.0f); // 60 pixels for Reset
                //ImGui::TableHeadersRow();

                for (auto *config : configs)
                    RenderConfigValueTable(*config);

                ImGui::EndTable();
            }
            ImGui::Unindent();
        }
    }

    ImGui::EndChild();
}

int main(int, char **)
{
    // SDL and window init
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
    g_Window = SDL_CreateWindow("INI Configuration Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT,
                                window_flags);

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

    io.IniFilename = NULL; // disable the ini file
    io.LogFilename = NULL;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    LoadImGuiFontFromResource(io);

    if (!ImGui_ImplSDL2_InitForD3D(g_Window) || !ImGui_ImplDX9_Init(g_pd3dDevice))
    {
        std::cerr << "Failed to initialise ImGui backends" << std::endl;
        CleanupDeviceD3D();
        SDL_DestroyWindow(g_Window);
        SDL_Quit();
        return -1;
    }

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
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                int new_width = event.window.data1;
                int new_height = event.window.data2;
                g_d3dpp.BackBufferWidth = new_width;
                g_d3dpp.BackBufferHeight = new_height;
                ImGui_ImplDX9_InvalidateDeviceObjects();
                if (ResetDevice())
                {
                    ImGui_ImplDX9_CreateDeviceObjects();
                }
            }
        }

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // --- Main fixed window ---
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("MainWindow", nullptr, window_flags);

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Load", "Ctrl+O"))
                {
                    TryLoadIniFile();
                }
                if (ImGui::MenuItem("Save", "Ctrl+S", false, g_FileLoaded))
                {
                    SaveConfigToFile();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                {
                    done = true;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("About"))
                {
                    // TODO: Add an about page, maybe?
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        RenderConfigEditor();

        ImGui::End();
        // --- End main window ---

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

    // Cleanup
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    SDL_DestroyWindow(g_Window);
    SDL_Quit();

    return 0;
}