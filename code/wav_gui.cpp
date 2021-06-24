#include "wav.h"
#include "wav_gui.h"
#include <ctime>
#include <string>
#include "font_inter_embed.cpp"

global_variable ImVec4 ButtonColor = RGB_TO_IMVEC4F(144, 186, 173);
global_variable ImVec4 ButtonHoverColor = RGB_TO_IMVEC4F(119, 168, 154);
global_variable ImVec4 ButtonActiveColor = RGB_TO_IMVEC4F(119, 168, 154);

global_variable ImVec2 DefaultMainPanelSize = ImVec2(400.0f, 275.0f);
global_variable bool DebugOpen = true;

void InitUI(ui_state *State)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Register font
    // NOTE(liam): Inter-regular.ttf is embedded in the source code so we don't have to worry
    // about shipping fonts with the binaries
    ImFontConfig FontConfig = ImFontConfig();
    FontConfig.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF((void *)Inter_data, Inter_size, 16.0f * State->DpiScale, &FontConfig);

    // Setup Dear ImGui base style and then override
    ImGui::StyleColorsClassic();
    
    ImVec4 *colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
    colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ButtonColor;
    colors[ImGuiCol_ButtonHovered] = ButtonHoverColor;
    colors[ImGuiCol_ButtonActive] = ButtonActiveColor;
    colors[ImGuiCol_TitleBgActive] = ButtonColor;
    colors[ImGuiCol_TitleBg] = ButtonColor;
    colors[ImGuiCol_TitleBgCollapsed] = ButtonColor;
    colors[ImGuiCol_HeaderHovered] = ButtonColor;
    ImGui::GetStyle().ScaleAllSizes(State->DpiScale);
}





inline void UpdateMainPanelSize(ui_state *State)
{
    ImVec2 ResultSize = ImVec2(DefaultMainPanelSize.x * State->DpiScale, DefaultMainPanelSize.y * State->DpiScale);
    if ((State->ExpandFlags & EXPAND_CONVERT) == EXPAND_CONVERT)
    {
        ResultSize.y += (50.f * State->DpiScale);
    }

    if ((State->ExpandFlags & EXPAND_CONVERT_SAVE) == EXPAND_CONVERT_SAVE)
    {
        ResultSize.y += (50.f * State->DpiScale);
    }

    if ((State->ExpandFlags & EXPAND_TRIM) == EXPAND_TRIM)
    {
        
        ResultSize.y += (50.f * State->DpiScale);
    }
    

    if ((State->ExpandFlags & EXPAND_TRIM_INPUTS) == EXPAND_TRIM_INPUTS)
    {
        ResultSize.x += (50.f * State->DpiScale);
        ResultSize.y += (50.f * State->DpiScale);
    }
    
    if ((State->ExpandFlags & EXPAND_CHECK) == EXPAND_CHECK)
    {
        ResultSize.y += (25.f * State->DpiScale);
    }
    
    if ((State->ExpandFlags & EXPAND_CHECK_2) == EXPAND_CHECK_2)
    {
        ResultSize.y += (25.f * State->DpiScale);
    }
    
    if ((State->ExpandFlags & EXPAND_OUTRO) == EXPAND_OUTRO)
    {
        ResultSize.y += (60.f * State->DpiScale);
    }

    State->MainPanelSize = ResultSize;
}

ui_state InitUiState()
{
    ui_state State = {};
    State.ShowFlags = SHOW_NONE;
    State.ExpandFlags = EXPAND_NONE;
    State.IntroAsOutro = false;
    State.AddOutro = false;
    State.ClearColor = ImVec4(0.96862745098f, 0.96078431373f, 0.97254901961f, 1.0f);
    State.ShowSavedStart = 0;
    State.ButtonSize = ImVec2(80.0f, 25.0f);
    State.MainPanelSize = DefaultMainPanelSize;
    State.TrimStartBuf[0] = 0;
    State.TrimEndBuf[0] = 0;
    State.DpiScale = 1.f;
    return State;
}

void ResetUiState(ui_state *State)
{
    State->AddOutro = false;
    State->IntroAsOutro = false;
    
    State->TrimStartBuf[0] = 0;
    State->TrimEndBuf[0] = 0;
    
    State->IntroFilePath = 0;
    State->PodcastFilePath = 0;
    State->OutroFilePath = 0;
    State->SaveFilePath = 0;
    State->TrimFilePath = 0;
    State->TrimSavePath = 0;

}

// This is the Central UI Loop of the application
// As this function is not in the platform layer, it's nice and simple to add new functionality to application
// cross plat.
void RenderUi(ui_state *State)
{
    PlatformImguiStartFrame();
    UpdateMainPanelSize(State);

    if ((State->ShowFlags & SHOW_DEMO) == SHOW_DEMO)
        ImGui::ShowDemoWindow((bool *)1);

    {
        
        ImGui::SetNextWindowSize(State->MainPanelSize);
        ImGui::Begin(
            "Intro Music",
            (bool *)1,
            ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse); // Create a virtual ImGui window

        if (ImGui::TreeNode("Convert Video File to WAV"))
        {
            State->ExpandFlags |= EXPAND_CONVERT;
            ImGui::Text("Select Video (MKV) file to convert");
            if (ImGui::Button("Browse##Video", State->ButtonSize))
            {
                file_filter Filter = {L"*.mkv", L"MKV files"};
                State->VideoFilePath = PlatformShowFileOpenDialog(State, &Filter);
            }
            
            if (State->VideoFilePath)
            {
                State->ExpandFlags |= EXPAND_CONVERT_SAVE;
                ImGui::SameLine(); ImGui::Text("%s", State->VideoFilePath);
                if (ImGui::Button("Save##VidOut", State->ButtonSize))
                {
                    State->VideoSavePath = PlatformShowFileSaveDialog(State, NULL, NULL);
                    if (State->VideoSavePath)
                    {
                        if (PlatformConvertVideo(State->VideoFilePath, State->VideoSavePath))
                        {
                            PlatformOpenFileManager(State->VideoSavePath, State);
                            State->VideoSavePath = 0;
                            State->VideoFilePath = 0;
                            State->ExpandFlags &= ~EXPAND_CONVERT_SAVE;
                        }
                    }
                }
            }

            ImGui::TreePop();
        } 
        else 
        {
            State->ExpandFlags &= ~EXPAND_CONVERT;
        }


        
        if (ImGui::TreeNode("Trim Start/End"))
        {
            State->ExpandFlags |= EXPAND_TRIM;
           
            ImGui::Text("Select WAV File to Trim");
            if (ImGui::Button("Browse##Trim", State->ButtonSize))
            {
                State->TrimFilePath = PlatformShowFileOpenDialog(State, NULL);
            }
            
            if ((State->ShowFlags & SHOW_TRIM_SAVED_MESSAGE) == SHOW_TRIM_SAVED_MESSAGE)
            {
                ImGui::SameLine();
                ImGui::Text("Saved!");
                int64 now = time(NULL);
                if (now - State->ShowTrimSavedStart >= 3)
                {
                    State->ShowFlags &= ~SHOW_TRIM_SAVED_MESSAGE;
                    State->ShowTrimSavedStart = 0;
                }
            }

            if (State->TrimFilePath)
            {
                State->ExpandFlags |= EXPAND_TRIM_INPUTS;
                ImGui::SameLine();
                ImGui::Text("%s", State->TrimFilePath);
                ImGui::InputText("Start At (seconds)", State->TrimStartBuf, 64, ImGuiInputTextFlags_CharsDecimal);
                ImGui::InputText("End At (seconds)", State->TrimEndBuf, 64, ImGuiInputTextFlags_CharsDecimal);
                if (State->TrimStartBuf[0] || State->TrimEndBuf[0])
                {
                    if (ImGui::Button("Save##Trim", State->ButtonSize))
                    {
                        State->TrimSavePath = PlatformShowFileSaveDialog(State, NULL, NULL);
                    }
            
                    if (State->TrimSavePath)
                    {
                        float TrimStart = (float)atof(&State->TrimStartBuf[0]);
                        float TrimEnd = (float)atof(&State->TrimEndBuf[0]);

                        loaded_sound TrimSound = LoadWAV(State->TrimFilePath);
                        if (!Trim(&TrimSound, TrimStart, TrimEnd, State->TrimSavePath))
                        {
                            Assert("Trim failed");
                        }

                        PlatformFree(TrimSound.Samples);
                        State->ShowFlags |= SHOW_TRIM_SAVED_MESSAGE;
                        State->ExpandFlags &= ~EXPAND_TRIM_INPUTS;
                        State->ShowTrimSavedStart = time(NULL);
                        State->TrimFilePath = 0;
                        State->TrimSavePath = 0;
                        State->TrimStartBuf[0] = 0;
                        State->TrimEndBuf[0] = 0;
                    }
                }
            }
            ImGui::TreePop();
        }
        else
        {
            State->TrimFilePath = 0;
            State->TrimSavePath = 0;
            State->TrimStartBuf[0] = 0;
            State->TrimEndBuf[0] = 0;
            State->ExpandFlags &= ~EXPAND_TRIM;
        }

        ImGui::Text("Select Intro WAV file to prepend");
        if (ImGui::Button("Browse##Intro", State->ButtonSize))
        {
            State->IntroFilePath = PlatformShowFileOpenDialog(State, NULL);
        }
        
        if (State->IntroFilePath)
        {
            State->ExpandFlags |= EXPAND_CHECK;
            ImGui::SameLine();
            ImGui::Text("%s", State->IntroFilePath);
            
            ImGui::Checkbox("Use this file for Outro", &State->IntroAsOutro);
            
            if (State->IntroAsOutro)
            {
                State->OutroFilePath = State->IntroFilePath;
            }
        }

        ImGui::NewLine();
        ImGui::Text("Select Main WAV file");
        if (ImGui::Button("Browse##Podcast", State->ButtonSize))
        {
            State->PodcastFilePath = PlatformShowFileOpenDialog(State, NULL);
        }
        
        if (State->PodcastFilePath)
        {
            ImGui::SameLine();
            ImGui::Text("%s", State->PodcastFilePath);
        }

        if (!State->IntroAsOutro)
        {
            State->ExpandFlags |= EXPAND_CHECK_2;
            ImGui::Checkbox("Add an Outro", &State->AddOutro);
            
            if (State->AddOutro) 
            {
                State->ExpandFlags |= EXPAND_OUTRO;
                ImGui::NewLine();
                ImGui::Text("Select Outro WAV file to append");
                if (ImGui::Button("Browse##Outro", State->ButtonSize))
                {
                    State->OutroFilePath = PlatformShowFileOpenDialog(State, NULL);
                }
                
                if (State->OutroFilePath)
                {
                    ImGui::SameLine();
                    ImGui::Text("%s", State->OutroFilePath);
                }
            }
            else
            {
                State->ExpandFlags &= ~EXPAND_OUTRO;
            }
        }

        ImGui::NewLine();
        ImGui::Text("Combine and save to new WAV file");
        if (ImGui::Button("Save", State->ButtonSize))
        {
            State->SaveFilePath = PlatformShowFileSaveDialog(State, NULL, NULL);
            if (
                State->IntroFilePath &&
                State->PodcastFilePath &&
                State->SaveFilePath)
            {
                if (Process(State->IntroFilePath, State->PodcastFilePath, State->OutroFilePath, State->SaveFilePath))
                {
                    PlatformOpenFileManager(State->SaveFilePath, State);
                    ResetUiState(State);
                    State->ShowFlags |= SHOW_SAVED_MESSAGE;
                    State->ShowSavedStart = time(NULL);
                }
            }
        }
        
        if ((State->ShowFlags & SHOW_SAVED_MESSAGE) == SHOW_SAVED_MESSAGE)
        {
            ImGui::SameLine();
            ImGui::Text("Saved!");
            int64 now = time(NULL);
            if (now - State->ShowSavedStart >= 3)
            {
                State->ShowFlags &= ~SHOW_SAVED_MESSAGE;
                State->ShowSavedStart = 0;
            }
        }
        ImGui::End();
        ImVec2 AboutPos = ImVec2(60, State->MainPanelSize.y + (60 + 50 * State->DpiScale));
        ImGui::SetNextWindowPos(AboutPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400 * State->DpiScale, 57 * State->DpiScale), ImGuiCond_FirstUseEver);
        ImGui::Begin("About");
        ImGui::Text("Version: %s", VERSION);
        ImGui::End();

#if WAV_INTERNAL
        if (DebugOpen)
        {
            const ImGuiViewport *main_viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(ImVec2(State->DpiScale * (main_viewport->WorkPos.x + 650), State->DpiScale * (main_viewport->WorkPos.y + 20)), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(550 * State->DpiScale, 680 * State->DpiScale), ImGuiCond_FirstUseEver);
            ImGui::Begin("DEBUG", &DebugOpen);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }
#endif

        ImGui::Render();
        PlatformImguiRender(State);
    }
}

void ShutdownUi()
{
    PlatformImguiShutdown();
    ImGui::DestroyContext();
}
