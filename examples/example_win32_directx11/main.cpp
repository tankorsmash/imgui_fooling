// dear imgui - standalone example application for DirectX 11
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.

#include "imgui_stdafx.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <memory>
#include "ui.h"
#include "stb_image.h"
#include <fstream>
#include <sstream>


//josh
struct ColorData
{
    int width;
    int height;
    unsigned char* red;
    unsigned char* green;
    unsigned char* blue;

    bool has_26 = false;
};

// Data
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

HRESULT CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return E_FAIL;

    CreateRenderTarget();

    return S_OK;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void prebuilt_imgui_demo(bool& show_demo_window, bool& show_another_window, ImVec4 clear_color)
{
    static float f     = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");          // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

    if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

unsigned char* create_bitmap(const int width, const int height, unsigned char* red, unsigned char* green, unsigned char* blue)
{
    const int w = width, h = width;
    int x, y;
    int r, g, b;

    auto get_index = [width](int x, int y) {
        return x + width * y;
    };
    //int total_size = w*h;

    unsigned char *img = NULL;
    int filesize = 54 + 3 * w*h;  //w is your image width, h is image height, both int

    img = (unsigned char *)malloc(3 * w*h);
    memset(img, 0, 3 * w*h);

    for (int i = 0; i < w; i++)
    {
        for (int j = 0; j < h; j++)
        {
            x = i; y = (h - 1) - j;
            r = red[get_index(j,i)];
            g = green[get_index(j,i)];
            b = blue[get_index(j,i)];
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            img[(x + y*w) * 3 + 2] = (unsigned char)(r);
            img[(x + y*w) * 3 + 1] = (unsigned char)(g);
            img[(x + y*w) * 3 + 0] = (unsigned char)(b);
        }
    }

    unsigned char bmpfileheader[14] = {'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0};
    unsigned char bmpinfoheader[40] = {40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0};
    unsigned char bmppad[3] = {0, 0, 0};

    bmpfileheader[2] = static_cast<unsigned char>(filesize);
    bmpfileheader[3] = static_cast<unsigned char>(filesize >> 8);
    bmpfileheader[4] = static_cast<unsigned char>(filesize >> 16);
    bmpfileheader[5] = static_cast<unsigned char>(filesize >> 24);

    bmpinfoheader[4] = static_cast<unsigned char>(w);
    bmpinfoheader[5] = static_cast<unsigned char>(w >> 8);
    bmpinfoheader[6] = static_cast<unsigned char>(w >> 16);
    bmpinfoheader[7] = static_cast<unsigned char>(w >> 24);
    bmpinfoheader[8] = static_cast<unsigned char>(h);
    bmpinfoheader[9] = static_cast<unsigned char>(h >> 8);
    bmpinfoheader[10] = static_cast<unsigned char>(h >> 16);
    bmpinfoheader[11] = static_cast<unsigned char>(h >> 24);

    unsigned char* out_img = new unsigned char[200000]; //random size

    int offset = 0;
    for (int i = 0; i < 14; i++) {
        out_img[i+offset] = bmpfileheader[i];
    }
    offset = 14;
    for (int i = 0; i < 40; i++) {
        out_img[i+offset] = bmpinfoheader[i];
    }

    //theirs
    for (int i = 0; i < w; i++)
    {
        for (int j = 0; j < h; j++)
        {
            x = i; y = (h - 1) - j;
            r = red[get_index(j,i)];
            g = green[get_index(j,i)];
            b = blue[get_index(j,i)];
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            img[(x + y*w) * 3 + 2] = (unsigned char)(r);
            img[(x + y*w) * 3 + 1] = (unsigned char)(g);
            img[(x + y*w) * 3 + 0] = (unsigned char)(b);

        }
    }
    FILE* f;
    fopen_s(&f, "their_temp.bmp", "wb");
    fwrite(bmpfileheader, 1, 14, f);
    fwrite(bmpinfoheader, 1, 40, f);
    for (int i = 0; i < h; i++)
    {
        fwrite(img + (w*(h - i - 1) * 3), 3, w, f);
        fwrite(bmppad, 1, (4 - (w * 3) % 4) % 4, f);
    }
    long fpos = ftell(f);
    fclose(f);

    auto in_stream = std::ifstream("their_temp.bmp");
    size_t chars_read;
    if (!(in_stream.read((char*)out_img, fpos))) // read up to the size of the buffer
    {
        if (!in_stream.eof()) {
            assert(false); // something went wrong while reading. Find out what and handle.
        }
    }
    chars_read = in_stream.gcount(); // get amount of characters really read.

    return out_img;

}

struct TextureData
{
    ID3D11ShaderResourceView* texture_id;
    int image_width;
    int image_height;
};
TextureData* create_texture_from_memory(unsigned char red[], unsigned char green[], unsigned char blue[], const int width, const int height);

TextureData* create_texture_from_memory(const ColorData& color_data)
{
    return create_texture_from_memory(color_data.red, color_data.green, color_data.blue, color_data.width, color_data.height);
}

TextureData* create_texture_from_memory(unsigned char red[], unsigned char green[], unsigned char blue[], const int width, const int height)
{
    const int length = 200000;
    auto result = new TextureData{{}, 0, 0};
    unsigned char* buffer2 = create_bitmap(width, height, red, green, blue);
    bool ret = load_texture_from_memory(buffer2, length, &result->texture_id, &result->image_width, &result->image_height);
    if (!ret) {
        const char* reason = stbi_failure_reason();

        wchar_t wide_reason[256];
        size_t retval;
        mbsrtowcs_s(&retval, wide_reason, &reason, strlen(reason) + 1, nullptr);
        wcscat_s(wide_reason, L"\n\0");
        OutputDebugStringW(wide_reason);
    }

    //update_data(&result);

    return result;
};

ColorData create_voronoi_color_data(int color)
{
    ColorData color_data;
    color_data.width = 256;
    color_data.height = 256;
    color_data.red = new unsigned char[color_data.height*color_data.width];
    color_data.green = new unsigned char[color_data.height*color_data.width];
    color_data.blue = new unsigned char[color_data.height*color_data.width];
    for (int h = 0; h < color_data.width; h++) {
        for (int w = 0; w < color_data.width; w++) {
            color_data.red  [w + color_data.width *  h] = 100;
            color_data.green[w + color_data.width *  h] = 42;
            color_data.blue [w + color_data.width *  h] = 200;
        }
    }

    bool& has_26 = color_data.has_26;
    for (int i = 0; i < color_data.width; i++) {
        int height = 2;
        int color = rand() % 255;
        if (color == 26) has_26 = true;
        //color_data.red[i * color_data.width + height] = color;
        color_data.green[i * color_data.width + height] = color;
        //color_data.blue [i * color_data.width + height] = color+2;
    }
    std::wstringstream ss;
    ss << "has the color: " << has_26 << std::endl;
    OutputDebugStringW(ss.str().c_str());

    return color_data;
}

int main(int, char**)
{
    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, _T("Dear ImGui DirectX11 Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (CreateDeviceD3D(hwnd) < 0)
    {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);


    //josh edit
    //bool ret = LoadTextureFromFile("../../MyImage01.jpg", &my_texture, &my_image_width, &my_image_height);
    //bool ret = LoadTextureFromFile("panel.png", &my_texture, &my_image_width, &my_image_height);

    ColorData color_data;
    color_data.width = 256;
    color_data.height = 256;
    const int h = color_data.height, w = color_data.width;
    color_data.red = new unsigned char [h*w];
    color_data.green = new unsigned char [h*w];
    color_data.blue = new unsigned char [h*w];
    for (int h = 0; h < color_data.width; h++) {
        for (int w = 0; w < color_data.width; w++) {
            color_data.red[w + color_data.width *  h] = 100;
            color_data.green[w + color_data.width *  h] = 42;
            color_data.blue[w + color_data.width *  h] = 200;
        }
    }
    for (int i = 0; i < w; i++) {
        color_data.red[i * color_data.width + 2] = 255;
        color_data.green[i * color_data.width + 2] = 255;
        color_data.blue[i * color_data.width + 2] = 255;
    }
    //auto new_texture_data = create_texture_from_memory(red, green, blue, width, height);
    auto new_texture_data = create_texture_from_memory(color_data);

    //for (int i = 0; i < w; i++) {
    //    red[i * width + 2] = 0;
    //    red[i * width + 2] = 0;
    //    red[i * width + 2] = 0;
    //}
    //auto new_texture_data2 = create_texture_from_memory(red, green, blue, width, height);
    //memcpy_s(new_texture_data2.texture_id, 100, black_data, 50);



    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            render_ui();
            //josh edit
            ImGui::Begin("DirectX11 Texture Test");
            static TextureData* TEXTURE_DATA = new_texture_data;
            static int color = 0;
            if (ImGui::Button("Regenerate")) {
                //TEXTURE_DATA = create_texture_from_memory(red, green, blue, width, height);
                std::wstringstream ss;
                ss << "Color: " << color << std::endl;
                OutputDebugStringW(ss.str().c_str()); //26 crashed once, 25 did too
                ColorData color_data = create_voronoi_color_data(26);
                color++;
                TEXTURE_DATA = create_texture_from_memory(color_data);
            }
            ImGui::Text("pointer = %p", TEXTURE_DATA->texture_id);
            ImGui::Text("size = %d x %d", TEXTURE_DATA->image_width, TEXTURE_DATA->image_height);
            ImGui::Image((void*)TEXTURE_DATA->texture_id, ImVec2((float)TEXTURE_DATA->image_width, (float)TEXTURE_DATA->image_height));
            //ImGui::Image((void*)new_texture_data2->texture_id, ImVec2((float)new_texture_data2->image_width, (float)new_texture_data2->image_height));
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}
