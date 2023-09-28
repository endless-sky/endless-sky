/* Debug.cpp
Copyright (c) 2023 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Debug.h"

#include "Logger.h"

#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <SDL2/SDL.h>

using namespace std;

namespace {
    bool state = false;
#if NDEBUG
    bool debugMode = false;
#else
    bool debugMode = true;
#endif
}



bool Debug::Init(SDL_Window *window, SDL_GLContext glContext)
{
    if (!IMGUI_CHECKVERSION()) {
        Logger::LogError("Failed to verify ImGui.");
        return false;
    }

    if(!ImGui::CreateContext()) {
        Logger::LogError("Failed to Create ImGui Context.");
        return false;
    }
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if(!ImGui_ImplSDL2_InitForOpenGL(window, glContext)) {
        Logger::LogError("Failed to Initialize Imgui SDL2 for OpenGL.");
        return false;
    }
    if(!ImGui_ImplOpenGL3_Init()) {
        Logger::LogError("Failed to Initialize Imgui OpenGL.");
        return false;
    }

    return true;
}



bool Debug::Process(SDL_Event *event)
{
    ImGui_ImplSDL2_ProcessEvent(event);
    const auto &io = ImGui::GetIO();
    return (io.WantCaptureKeyboard || io.WantCaptureMouse);
}



void Debug::StartCapture() {
    if(!state && debugMode) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        static float UIScale;
        if(ImGui::SliderFloat("UI Scale", &UIScale, 1., 10.)) {
            ImGui::SetWindowFontScale(UIScale);
        }
        state = !state;
    }
}



void Debug::Render() {
    if(state && debugMode) {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        state = !state;
    }
}



void Debug::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}



void Debug::SetDebugMode(bool mode) {
    debugMode = mode;
}



bool Debug::GetDebugMode() {
    return debugMode;
}
