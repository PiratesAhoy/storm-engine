#include <windows.h>
#include "../rsrc/resource.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include <d3d9.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_sdl2.h>
#include <iostream>
#include <string>
#pragma comment(lib, "d3d9.lib")

// Globals
LPDIRECT3D9 g_pD3D = NULL;
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
D3DPRESENT_PARAMETERS g_d3dpp = {};
SDL_Window *g_Window = NULL;
SDL_Renderer *g_Renderer = NULL;
SDL_SysWMinfo g_WmInfo;


// TODO: Consider using something else? DirectX is a bit overkill for this,
//  but it's what we already use, so I'm not sure.

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
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

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

void LoadImGuiFontFromResource(ImGuiIO &io)
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
                                                  25.0f,         // font size in pixels
                                                  &fontConfig);

    if (!font)
    {
        OutputDebugStringA("Failed to load font into ImGui\n");
    }
    std::cout << "Succesfully loaded font resource." << std::endl;
}

int main(int, char **)
{
    // SDL and window init
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_ALLOW_HIGHDPI);
    g_Window =
        SDL_CreateWindow("Enhanced ImGui App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 400, window_flags);

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

    ImGui::StyleColorsDark();

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

    LoadImGuiFontFromResource(io);

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

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Create a fullscreen ImGui window
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("MainWindow", nullptr, window_flags);

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
    
    // Cleanup
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    SDL_DestroyWindow(g_Window);
    SDL_Quit();

    return 0;
}