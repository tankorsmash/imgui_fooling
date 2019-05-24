
#include "imgui_stdafx.h"
#include <cstdlib>
#include <cstdio>

void render_ui()
{

    using namespace ImGui;
    ImGuiWindowFlags window_flags = 0 | ImGuiWindowFlags_MenuBar;
    window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
    Begin("RENDER UI",0, window_flags); // Create a window called "Hello, world!" and append into it.

    Text("This is some useful text before the menu.");          // Display some text (you can use a format strings too)

    if (BeginMenuBar()) {
        if (BeginMenu("First")) {
            Text("ASD");
            if ( Button("This is a button") ) {
                printf("ASD\n");
            };
            if (ImGui::IsItemHovered()){
                if (ImGui::IsMouseClicked(1)) {
                    exit(0);
                }
            }
            EndMenu();
        }
        if (BeginMenu("Second")){
            Text("aaa");
            EndMenu();
        };
        EndMenuBar();
    }
    
    if (CollapsingHeader("This is some grouped text.")) {
        Text("NESTED");          // Display some text (you can use a format strings too)
        Text("NESTED");          // Display some text (you can use a format strings too)
        Text("NESTED");          // Display some text (you can use a format strings too)
        Text("NESTED");          // Display some text (you can use a format strings too)
        
    };          // Display some text (you can use a format strings too)
    Text("This is some grouped text.");          // Display some text (you can use a format strings too)
    Text("This is some grouped text.");          // Display some text (you can use a format strings too)
    Text("This is some grouped text.");          // Display some text (you can use a format strings too)

    End();
}
