#include "storm/editor/engine_editor.hpp"

#include "core.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>

#include <array>

#ifdef _WIN32
#include <Windows.h>
#include <imgui_impl_dx9.h>
#endif WIN32

namespace storm::editor
{

struct EditorToolConfig
{
    std::string title;
    EditorToolCallback callback;
    bool active = false;
};

class EngineEditor::Impl final {
public:
    static std::vector<EditorToolConfig> tools_;

    std::array<bool, 5> debugFlags_;

    ImGuiContext *imgui_;

    bool isFocused_ = true;
};

std::vector<EditorToolConfig> EngineEditor::Impl::tools_;

EngineEditor::EngineEditor(SDL_Window *window, IDirect3DDevice9 *device)
    : impl_(std::make_unique<Impl>())
{
    IMGUI_CHECKVERSION();
    impl_->imgui_ = ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplSDL2_InitForD3D(window);
#ifdef _WIN32
    ImGui_ImplDX9_Init(device);
#endif WIN32
}

EngineEditor::~EngineEditor()
{
    ImGui::DestroyContext(impl_->imgui_);
}

void EngineEditor::StartFrame()
{
    const auto &io = ImGui::GetIO();
    impl_->isFocused_ = io.WantCaptureKeyboard;
    core.GetWindow()->ShowCursor(impl_->isFocused_);

#ifdef _WIN32
    ImGui_ImplDX9_NewFrame();
#endif WIN32
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    if (ImGui::Begin("Developer Tools", NULL, ImGuiWindowFlags_MenuBar) )
    {
        if (ImGui::BeginMenuBar() )
        {
            if (ImGui::BeginMenu("Tools") )
            {
                for (auto &config : Impl::tools_)
                {
                    ImGui::MenuItem(config.title.c_str(), NULL, &config.active);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Debug") )
            {
                ImGui::MenuItem("Wireframe mode", NULL, &impl_->debugFlags_[static_cast<size_t>(DebugFlag::RenderWireframe)]);
                ImGui::MenuItem("Sound debug", NULL, &impl_->debugFlags_[static_cast<size_t>(DebugFlag::SoundDebug)]);
                ImGui::MenuItem("Location debug", NULL, &impl_->debugFlags_[static_cast<size_t>(DebugFlag::LocationDebug)]);
                ImGui::MenuItem("Extended location debug", NULL, &impl_->debugFlags_[static_cast<size_t>(DebugFlag::ExtendedLocationDebug)]);
                ImGui::MenuItem("Ship cannon debug", NULL, &impl_->debugFlags_[static_cast<size_t>(DebugFlag::CannonDebug)]);
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::End();
    }

    for (auto &config : Impl::tools_)
    {
        if (config.active)
        {
            config.callback(config.active);
        }
    }
}

void EngineEditor::EndFrame()
{
    ImGui::Render();
#ifdef _WIN32
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
#endif WIN32
}

void EngineEditor::ProcessEvent(SDL_Event &event)
{
    ImGui_ImplSDL2_ProcessEvent(&event);
}

bool EngineEditor::IsFocused() const
{
    return impl_->isFocused_;
}

bool EngineEditor::IsDebugFlagEnabled(DebugFlag flag) const
{
    return impl_->debugFlags_[static_cast<size_t>(flag)];
}

void EngineEditor::RegisterEditorTool(const std::string_view &title, EditorToolCallback callback)
{
    Impl::tools_.push_back({.title = std::string(title), .callback = callback});
}

} // namespace storm::editor
