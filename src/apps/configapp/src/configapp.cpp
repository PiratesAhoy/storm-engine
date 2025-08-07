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
#define DEFAULT_WINDOW_WIDTH 500
#define DEFAULT_WINDOW_HEIGHT 600

// Globals
LPDIRECT3D9 g_pD3D = NULL;
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
D3DPRESENT_PARAMETERS g_d3dpp = {};
SDL_Window *g_Window = NULL;
SDL_Renderer *g_Renderer = NULL;
SDL_SysWMinfo g_WmInfo;

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

    // Current values for editing
    std::string stringValue;
    int intValue = 0;
    float floatValue = 0.0f;
    bool boolValue = false;

    ConfigValue(const std::string &sect, const std::string &k, const std::string &display, const std::string &desc,
                ConfigValueType t, const std::string &def, const ConfigConstraints &constr = {})
        : section(sect)
        , key(k)
        , displayName(display)
        , description(desc)
        , type(t)
        , defaultValue(def)
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
            return boolValue ? "true" : "false";
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
std::vector<ConfigValue> g_ConfigDefinitions = {
    ConfigValue("", "full_screen", "Full Screen on/off", "Whether the game will be opened full-screen",
        ConfigValueType::Boolean, "0"),
    ConfigValue("", "display", "Display Index", "Index of display to use, 0 = default display",
        ConfigValueType::Integer, "0", {0, 5}),
    ConfigValue("interface", "screen_width", "Screen width.", "The width of the game window in pixels",
        ConfigValueType::Integer, "0", {200, 6000}),
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
        std::string value = g_IniFile.getValue(config.section, config.key, config.defaultValue);
        config.setFromString(value);
    }
    g_FileLoaded = true;
}

void SaveConfigToFile()
{
    for (const auto &config : g_ConfigDefinitions)
    {
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

// Replace RenderConfigValue with a version that draws into table columns
void RenderConfigValueTable(ConfigValue &config)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%s", config.displayName.c_str());
    if (!config.description.empty() && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", config.description.c_str());

    ImGui::TableSetColumnIndex(1);
    bool changed = false;
    ImGui::PushID(&config);

    switch (config.type)
    {
    case ConfigValueType::String: {
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
        if (ImGui::SliderInt("##slider", &config.intValue, (int)config.constraints.minValue, (int)config.constraints.maxValue))
            changed = true;
        break;
    }
    case ConfigValueType::Float: {
        if (ImGui::SliderFloat("##slider", &config.floatValue, config.constraints.minValue, config.constraints.maxValue, "%.1f"))
            changed = true;
        break;
    }
    case ConfigValueType::Boolean: {
        const char* boolItems[] = { "False", "True" };
        int current = config.boolValue ? 1 : 0;
        if (ImGui::Combo("##boolcombo", &current, boolItems, 2)) {
            config.boolValue = (current == 1);
            changed = true;
        }
        break;
    }
    }

    ImGui::TableSetColumnIndex(2);
    if (ImGui::SmallButton("Reset"))
    {
        config.setFromString(config.defaultValue);
        changed = true;
    }

    if (changed)
        g_HasUnsavedChanges = true;

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
            if (ImGui::BeginTable("config_table", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
            {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.4f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.4f);
                ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed, 60.0f); // 60 pixels for Reset
                ImGui::TableHeadersRow();

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
                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                {
                    if (!g_CurrentFileName.empty())
                    {
                        g_IniFile.close();
                        if (g_IniFile.open(g_CurrentFileName))
                        {
                            g_FileLoaded = true;
                            SaveConfigToFile();
                        }
                    }
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