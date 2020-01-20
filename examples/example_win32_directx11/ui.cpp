
#include "imgui_stdafx.h"
#include <cstdlib>
#include <cstdio>
#include <string>
#include <array>

static std::string output;

void log(std::string msg)
{
    output += "\n";
    output += msg;
}

void render_ui()
{

    using namespace ImGui;
    ImGuiWindowFlags window_flags = 0 | ImGuiWindowFlags_MenuBar;

    Begin("Output Window", 0, window_flags);
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
            //if (ImGui::MenuItem("Save", "Ctrl+S"))   { /* Do stuff */ }
            if (ImGui::MenuItem("Close", "CTRL+S")) {
                exit(0);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    BulletText("This is the output");          // Display some text (you can use a format strings too)
    Text(output.c_str());          // Display some text (you can use a format strings too)
    ImGui::SetScrollHereY(1.0);
    End();

    window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
    Begin("RENDER UI",0, window_flags); // Create a window called "Hello, world!" and append into it.

    Text("This is some useful text before the menu.");          // Display some text (you can use a format strings too)

    auto io = ImGui::GetIO();

    io.WantCaptureKeyboard = true;

    if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
        exit(0);
    }
    else if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_Space))) {
        exit(0);
    }

    if (BeginMenuBar()) {
        if (BeginMenu("First Button")) {
            if ( Button("This is a button") ) {
                log("ASD\n");
            };
            //if (ImGui::IsItemHovered()){
            //    if (ImGui::IsMouseClicked(1)) {
            //        exit(0);
            //    }
            //}
            EndMenu();
        }
        if (BeginMenu("Second Button")){
            Text("aaa");
            EndMenu();
        };
        EndMenuBar();
    }

    static std::array<char, 255> input_str = {'t', 'e', 's','t'};
    ImGui::InputText("Testing input", input_str.data(), input_str.size());

    
    if (CollapsingHeader("This is some grouped text.")) {
        Text("grouped under header");          // Display some text (you can use a format strings too)
        Text("and again grouped under header");          // Display some text (you can use a format strings too)
    };

    // Display some text (you can use a format strings too)
    Text("This is some grouped text.");          // Display some text (you can use a format strings too)

    End();
}
