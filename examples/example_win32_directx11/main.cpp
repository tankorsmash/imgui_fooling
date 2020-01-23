// dear imgui - standalone example application for DirectX 11
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.

#include "imgui_stdafx.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>

#include "stb_image.h"
#include "bitmap_image.hpp"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <memory>
#include <fstream>
#include <sstream>
#include <array>
#include <chrono>

#include "ui.h"
#include <thread>
#include "FastNoise.h"
#include "imgui_internal.h"
#include "delaunator.hpp"
#include <set>


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

struct PointData
{
    std::array<int, 2> pos;
    BYTE color;
};

// Data
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

    static FastNoise* noise;

template <typename T>
const T& clamp( const T& v, const T& lo, const T& hi )
{
    assert( !(hi < lo) );
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

void my_print(const std::wstring& msg)
{
    std::wstringstream ss;
    ss << msg << std::endl;
    OutputDebugStringW(ss.str().c_str());
}

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
TextureData* create_texture_from_rgb_array(unsigned char red[], unsigned char green[], unsigned char blue[], const int width, const int height);

TextureData* create_texture_from_rgb_array(const ColorData& color_data)
{
    return create_texture_from_rgb_array(color_data.red, color_data.green, color_data.blue, color_data.width, color_data.height);
}

TextureData* create_texture_data_from_image(bitmap_image* image)
{
    auto texture_data = new TextureData{{}, 0, 0};
    unsigned char* image_buffer = image->data(); //only pixel data, pretty sure

    int width = image->width();
    int height = image->height();

    //create a 4bpp vector from the 3bpp source
    std::vector<unsigned char> vector_buffer{};
    auto buf_len = image->pixel_count() * (image->bytes_per_pixel()); //3bpp 
    auto new_buf_len = image->pixel_count() * (image->bytes_per_pixel() + 1); //3bpp + 1 for alpha
    vector_buffer.reserve(new_buf_len);
    for (int i = 0; i < buf_len; i++) { //assuming 200k is size of buffer
        if (i % 3 == 0) {
            vector_buffer.push_back(image_buffer[i+2]);
            vector_buffer.push_back(image_buffer[i+1]);
            vector_buffer.push_back(image_buffer[i]);
            vector_buffer.push_back(255); //alpha
        }
    }

    create_dx11_texture(&texture_data->texture_id, &texture_data->image_width, &texture_data->image_height, width, height, vector_buffer.data());

    return texture_data;
}

TextureData* create_texture_from_rgb_array(unsigned char red[], unsigned char green[], unsigned char blue[], const int width, const int height)
{
    auto result = new TextureData{{}, 0, 0};
    //unsigned char* buffer = create_bitmap(width, height, red, green, blue); //has an issue with pixel with 26 colors i think

    auto image = new bitmap_image(width, height);
    image->clear();
    image->set_all_channels(0, 255, 0);
    image->import_rgb(red, green, blue);
    image->save_image("bitmap_image.bmp");
    unsigned char* image_buffer = image->data(); //only pixel data, pretty sure

    //create a 4bpp vector from the 3bpp source
    std::vector<unsigned char> vector_buffer{};
    auto buf_len = image->pixel_count() * (image->bytes_per_pixel()); //3bpp 
    auto new_buf_len = image->pixel_count() * (image->bytes_per_pixel() + 1); //3bpp + 1 for alpha
    vector_buffer.reserve(new_buf_len);
    for (int i = 0; i < buf_len; i++) { //assuming 200k is size of buffer
        if (i % 3 == 0) {
            vector_buffer.push_back(image_buffer[i+2]);
            vector_buffer.push_back(image_buffer[i+1]);
            vector_buffer.push_back(image_buffer[i]);
            vector_buffer.push_back(255); //alpha
        }
    }

    create_dx11_texture(&result->texture_id, &result->image_width, &result->image_height, width, height, vector_buffer.data());

    //update_data(result);

    return result;
};

void set_pixel_color(ColorData& color_data, const std::vector<PointData*>& points, int h, int w)
{
    int min_dist = 9999;

    PointData * const* closest_point = &points.front(); //always have one, no matter what
    for (auto& point_data : points) {
        const std::array<int, 2>& pt = point_data->pos;
        int x_dist  = std::pow(w - pt[0], 2);
        int y_dist  = std::pow(h - pt[1], 2);
        double root = sqrt(x_dist + y_dist);

        if (root < min_dist) {
            closest_point = &point_data;
        }
        min_dist    = std::min(min_dist, (int)root);
    }

    //convert to a 0-255 color
    min_dist = ((float)min_dist) / (color_data.width) * 255;

    auto close_color = (*closest_point)->color;
    int addr               = w + color_data.width *  h;
    color_data.red  [addr] = min_dist;
    color_data.green[addr] = min_dist;
    color_data.blue [addr] = close_color;
}

void set_row_colors(ColorData& color_data, const std::vector<PointData*>& points, int h)
{
    for (int w = 0; w < color_data.width; w++) {

        //std::thread* px_thread = new std::thread(&set_pixel_color, color_data, points, h, w);
        //row_threads.push_back(px_thread);

        set_pixel_color(color_data, points, h, w);
    }

    //my_print(L"done row "+std::to_wstring(h));
}

const BYTE* get_noise_colors(int width_height, int rng_seed)
{
    BYTE* result = new BYTE[width_height*width_height*2];

    int i = 0;

    FN_DECIMAL low_val = 99999;
    FN_DECIMAL hi_val = -999999;
    for (int h = 0; h < width_height; h++) {
        for (int w = 0; w < width_height; w++) {
            int addr = w + width_height*  h;

            noise->SetCellularReturnType(FastNoise::Distance);
            FN_DECIMAL distance = noise->GetNoise(w, h);
            noise->SetCellularReturnType(FastNoise::CellValue);
            FN_DECIMAL raw_value = noise->GetNoise(w, h);

            //color
            auto raw_color = (distance * 255.0f);
            raw_color = clamp(raw_color, 0.0f, 255.0f);
            BYTE color = (BYTE)raw_color;

            //value
            FN_DECIMAL value = clamp((int)((raw_value+1)/2*255), 0, 255);
            low_val = std::min(low_val, value);
            hi_val = std::max(hi_val, value);

            result[i] = color;
            i++;
            result[i] = (BYTE)value;
            i++;

        }
    }

    std::wstringstream ss;
    ss << "lowest: " << low_val << " and highest: " << hi_val << std::endl;
    my_print(ss.str());

    return result;
}

ColorData create_voronoi_color_data(int width_height, int rng_seed)
{
    ColorData color_data;
    color_data.width = width_height;
    color_data.height = width_height;
    color_data.red   = new unsigned char[color_data.height*color_data.width];
    color_data.green = new unsigned char[color_data.height*color_data.width];
    color_data.blue  = new unsigned char[color_data.height*color_data.width];

    auto start = std::chrono::high_resolution_clock::now();

    const BYTE* noise_data = get_noise_colors(width_height, rng_seed);

    //for (int h = 0; h < color_data.height; h++) {
    //    for (int w = 0; w < color_data.width; w++) {
    //        int addr = (w + color_data.width * h) * 2;

    //        auto color = noise_data[addr];
    //        auto value = noise_data[addr + 1];
    //        color_data.red[addr/2] = color;
    //        color_data.green[addr/2] = color;
    //        color_data.blue[addr/2] = value;
    //        //my_print(std::to_wstring(value));
    //    }
    //}

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = end - start; //seconds, I think


    std::wstringstream ss;
    ss << "elapsed: " << elapsed.count() << std::endl;

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
    int width_height = 512;
    color_data.width = width_height;
    color_data.height = width_height;
    color_data.red   = new unsigned char [color_data.height*color_data.width];
    color_data.green = new unsigned char [color_data.height*color_data.width];
    color_data.blue  = new unsigned char [color_data.height*color_data.width];


    int num_points = 20;
    int* raw_points = new int[num_points * 2];
    std::vector<double>  points{};

    using coord_t = std::pair<unsigned int, unsigned int>;
    std::vector<coord_t>  edge_points{};
    for (int i = 0; i < num_points; i++) {
        int x = rand() % color_data.width;
        int y = rand() % color_data.height;
        points.push_back((double)x);
        points.push_back((double)y);
        edge_points.push_back(std::make_pair(x, y));
    }

    delaunator::Delaunator delaunator(points);
    auto image = bitmap_image(width_height, width_height);
    image.clear();
    image.set_all_channels(240, 240, 240);
    image_drawer drawer(image);
    for(std::size_t i = 0; i < delaunator.triangles.size(); i+=3) {
        drawer.triangle(
            delaunator.coords[2 * delaunator.triangles[i]],        //tx0
            delaunator.coords[2 * delaunator.triangles[i] + 1],    //ty0
            delaunator.coords[2 * delaunator.triangles[i + 1]],    //tx1
            delaunator.coords[2 * delaunator.triangles[i + 1] + 1],//ty1
            delaunator.coords[2 * delaunator.triangles[i + 2]],    //tx2
            delaunator.coords[2 * delaunator.triangles[i + 2] + 1] //ty2
        );
    }
    image.save_image("delaunator_output.bmp");


    using edge_t = unsigned int;
    using v_edge_t = std::vector<edge_t>;
    auto next_half_edge = [](edge_t edge) { return (edge % 3 == 2) ? edge - 2 : edge + 1;  };

    //function triangleOfEdge(e)  { return Math.floor(e / 3); }
    auto triangle_of_edge = [](edge_t e) { return (edge_t)std::floor(((double)e) / 3.0); };

    //function edgesOfTriangle(t) { return [3 * t, 3 * t + 1, 3 * t + 2]; }

    //returns 3 edge IDs
    auto edgesOfTriangle = [](edge_t t) {
        return v_edge_t{3 * t, 3 * t + 1, 3 * t + 2};
    };

    //function pointsOfTriangle(delaunay, t) {
    //    return edgesOfTriangle(t)
    //        .map(e => delaunay.triangles[e]);
    //}

    //finds 3 points for a given triangle id
    auto pointsOfTriangle = [edgesOfTriangle](const delaunator::Delaunator& delaunator, edge_t tri_id) {
        auto edges = edgesOfTriangle(tri_id);

        v_edge_t points;
        auto find_tri_for_edge = [delaunator](edge_t e){ return delaunator.triangles[e]; };
        std::transform(edges.begin(), edges.end(), std::back_inserter(points), find_tri_for_edge);

        return points;
    };

    auto edgesAroundPoint = [next_half_edge](const delaunator::Delaunator& delaunator, edge_t edge) {
        std::vector<edge_t> result;

        edge_t incoming = edge;
        do {
            result.push_back(edge);
            const edge_t outgoing = next_half_edge(incoming);
            incoming = delaunator.halfedges[outgoing];
        } while (incoming != -1 && incoming != edge);

        return result;
    };

    //function triangleCenter(points, delaunay, t) {
    //    const vertices = pointsOfTriangle(delaunay, t).map(p => points[p]);
    //    return circumcenter(vertices[0], vertices[1], vertices[2]);
    //}
    auto triangleCenter = [pointsOfTriangle](std::vector<coord_t>& points, delaunator::Delaunator& delaunator, edge_t tri_id)
    {
        std::vector<edge_t> tri_points = pointsOfTriangle(delaunator, tri_id);
        std::vector<coord_t> vertices{};
        std::transform(tri_points.begin(), tri_points.end(), std::back_inserter(vertices), [&points](edge_t tri_point) {return points[tri_point]; });
#define UNPACK2(pair) pair.first, pair.second
        auto result = delaunator::circumcenter(UNPACK2(vertices[0]), UNPACK2(vertices[1]), UNPACK2(vertices[2]));
        return std::pair<int, int>(UNPACK2(result));
#undef UNPACK2
    };

    auto forEachVoronoiCell = [&](std::vector<coord_t> points, delaunator::Delaunator& delaunator, std::function<void(edge_t, std::vector<coord_t>)> callback) {

        std::set<unsigned int> seen_points;
        for (const int& edge : delaunator.triangles) {
            unsigned int point = delaunator.triangles[next_half_edge(edge)];

            if (std::find(seen_points.begin(), seen_points.end(), point) == seen_points.end()) {
                seen_points.insert(point);
                std::vector<edge_t> edges = edgesAroundPoint(delaunator, point);


                std::vector<edge_t> triangles{};
                std::transform(edges.begin(), edges.end(), std::back_inserter(triangles), triangle_of_edge);

                std::vector<coord_t> vertices{};
                std::transform(triangles.begin(), triangles.end(), std::back_inserter(vertices), [&triangleCenter, &points, &delaunator](edge_t tri_id) {
                    auto result = triangleCenter(points, delaunator, tri_id);

                    return result;
                });

                callback(point, vertices);
            }
        }
    };

    auto draw_vertices = [&drawer](edge_t point, std::vector<coord_t>& vertices) {
        auto draw_a_to_b = [&drawer](coord_t& a, coord_t& b) {
            drawer.line_segment(a.first, a.second, b.first, b.second);
        };
        draw_a_to_b(vertices[0], vertices[1]); //a b
        draw_a_to_b(vertices[1], vertices[2]); //b c
        draw_a_to_b(vertices[2], vertices[0]); //c a
    };

    delaunator::Delaunator delaunator2(points);
    forEachVoronoiCell(edge_points, delaunator2, draw_vertices);
    image.save_image("delaunator_output_vornoi.bmp");


    //for (int h = 0; h < color_data.width; h++) {
    //    for (int w = 0; w < color_data.width; w++) {
    //        color_data.red  [w + color_data.width *  h] = 165;
    //        color_data.green[w + color_data.width *  h] = 42;
    //        color_data.blue [w + color_data.width *  h] = 200;
    //    }
    //}
    //for (int i = 0; i < w; i++) {
    //    color_data.red[i * color_data.width + 2] = 255;
    //    color_data.green[i * color_data.width + 2] = 255;
    //    color_data.blue[i * color_data.width + 2] = 255;
    //}
    //auto new_texture_data = create_texture_from_memory(red, green, blue, width, height);
    TextureData* new_texture_data = create_texture_from_rgb_array(color_data);

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

    static int rng_seed = 12345;

    noise = new FastNoise;
    noise->SetSeed(rng_seed);
    noise->SetCellularDistance2Indicies(0, 1);
    noise->SetInterp(FastNoise::Hermite);
    noise->SetNoiseType(FastNoise::Cellular);
    noise->SetFrequency(0.02f);
    noise->SetCellularJitter(0.4f);
    noise->SetCellularReturnType(FastNoise::Distance);
    noise->SetCellularDistanceFunction(FastNoise::CellularDistanceFunction::Euclidean);


    color_data = create_voronoi_color_data(width_height, rng_seed);
    new_texture_data = create_texture_from_rgb_array(color_data);
    new_texture_data = create_texture_data_from_image(&image);


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

            ImGui::InputInt("Voronoi Seed", &rng_seed, 1, 100);

            if (ImGui::Button("Regenerate")) {
                //TEXTURE_DATA = create_texture_from_memory(red, green, blue, width, height);
                ColorData color_data = create_voronoi_color_data(width_height, rng_seed);
                TEXTURE_DATA = create_texture_from_rgb_array(color_data);
            }
            ImGui::Text("pointer = %p", TEXTURE_DATA->texture_id);
            ImGui::Text("size = %d x %d", TEXTURE_DATA->image_width, TEXTURE_DATA->image_height);


            ImGuiWindow* window = ImGui::GetCurrentWindow();
            ImGui::LabelText("Mouse Pos:", "%f %f", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
            ImVec2 image_pos = window->DC.CursorPos;
            ImVec2 mouse_pos = ImVec2(ImGui::GetIO().MousePos.x - image_pos.x,  ImGui::GetIO().MousePos.y - image_pos.y);
            ImGui::Image((void*)TEXTURE_DATA->texture_id, ImVec2((float)TEXTURE_DATA->image_width, (float)TEXTURE_DATA->image_height));
            if (ImGui::IsItemHovered()) {
                ImGui::LabelText("Mouse Pos", "%f %f", mouse_pos.x, mouse_pos.y);
                noise->SetCellularReturnType(FastNoise::Distance);
                float distance = noise->GetNoise(mouse_pos.x, mouse_pos.y);
                noise->SetCellularReturnType(FastNoise::CellValue);
                float cell_value = noise->GetNoise(mouse_pos.x, mouse_pos.y);
                ImGui::LabelText("Distance, Id", "%f, %f", distance, (cell_value+1)/2*255);
            }
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
