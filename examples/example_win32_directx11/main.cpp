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
#include <random>


#include "ui.h"
#include <thread>
#include "FastNoise.h"
#include "imgui_internal.h"
#include "delaunator.hpp"
#include <set>

using coord_t = std::pair<unsigned int, unsigned int>;
static int rng_seed = 12345;
    using edge_t = size_t;
    using v_double_t = std::vector<double>;
    using double_pair_t = std::pair<double, double>;
    using v_double_pair_t = std::vector<double_pair_t>;

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
    const int w = width, h = height;
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
    chars_read = (size_t)in_stream.gcount(); // get amount of characters really read.

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
    for (unsigned i = 0; i < buf_len; i++) { //assuming 200k is size of buffer
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
    for (unsigned int i = 0; i < buf_len; i++) { //assuming 200k is size of buffer
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
    unsigned min_dist = 9999;

    PointData * const* closest_point = &points.front(); //always have one, no matter what
    for (auto& point_data : points) {
        const std::array<int, 2>& pt = point_data->pos;
        int x_dist  = (int)std::floor(std::pow(w - pt[0], 2));
        int y_dist  = (int)std::floor(std::pow(h - pt[1], 2));
        double root = sqrt(x_dist + y_dist);

        if (root < min_dist) {
            closest_point = &point_data;
        }
        min_dist    = std::min(min_dist, (unsigned int)root);
    }

    //convert to a 0-255 color
    min_dist = (float)(((float)(min_dist)) / (color_data.width) * 255);

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

const BYTE* get_noise_colors(int width_height, int /*rng_seed*/)
{
    BYTE* result = new BYTE[width_height*width_height*2];

    int i = 0;

    FN_DECIMAL low_val = 99999;
    FN_DECIMAL hi_val = -999999;
    for (int h = 0; h < width_height; h++) {
        for (int w = 0; w < width_height; w++) {
            //int addr = w + width_height*  h;

            noise->SetCellularReturnType(FastNoise::Distance);
            FN_DECIMAL distance = noise->GetNoise(FN_DECIMAL(w), FN_DECIMAL(h));
            noise->SetCellularReturnType(FastNoise::CellValue);
            FN_DECIMAL raw_value = noise->GetNoise(FN_DECIMAL(w), FN_DECIMAL(h));

            //color
            auto raw_color = (distance * 255.0f);
            raw_color = clamp(raw_color, 0.0f, 255.0f);
            BYTE color = (BYTE)raw_color;

            //value
            FN_DECIMAL value = (FN_DECIMAL)clamp((int)((raw_value+1)/2*255), 0, 255);
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

double_pair_t circumcenter(double_pair_t a, double_pair_t b, double_pair_t c)
{
    //function circumcenter(a, b, c) {
    //const ad = a[0] * a[0] + a[1] * a[1];
    //const bd = b[0] * b[0] + b[1] * b[1];
    //const cd = c[0] * c[0] + c[1] * c[1];
    //const D = 2 * (a[0] * (b[1] - c[1]) + b[0] * (c[1] - a[1]) + c[0] * (a[1] - b[1]));
    //return [
    //    1 / D * (ad * (b[1] - c[1]) + bd * (c[1] - a[1]) + cd * (a[1] - b[1])),
    //    1 / D * (ad * (c[0] - b[0]) + bd * (a[0] - c[0]) + cd * (b[0] - a[0])),
    //];
    //}
    double ad = a.first * a.first + a.second * a.second;
    double bd = b.first * b.first + b.second * b.second;
    double cd = c.first * c.first + c.second * c.second;
    double D = 2 * (a.first * (b.second - c.second) + b.first * (c.second - a.second) + c.first * (a.second - b.second));
    return double_pair_t{
        1 / D * (ad * (b.second - c.second) + bd * (c.second - a.second) + cd * (a.second - b.second)),
        1 / D * (ad * (c.first - b.first) + bd * (a.first - c.first) + cd * (b.first - a.first)),
    };
};

ColorData create_voronoi_color_data(int width_height, int /*rng_seed*/)
{
    ColorData color_data;
    color_data.width = width_height;
    color_data.height = width_height;
    color_data.red   = new unsigned char[color_data.height*color_data.width];
    color_data.green = new unsigned char[color_data.height*color_data.width];
    color_data.blue  = new unsigned char[color_data.height*color_data.width];

    auto start = std::chrono::high_resolution_clock::now();

    //const BYTE* noise_data = get_noise_colors(width_height, rng_seed);

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

delaunator::Delaunator* draw_del_points_to_canvas(std::vector<double>& points, cartesian_canvas* canvas, image_drawer* drawer)
{
    delaunator::Delaunator* del = new delaunator::Delaunator(points);
    canvas->image().clear();
    canvas->image().set_all_channels(240, 240, 240);
    for(std::size_t i = 0; i < del->triangles.size(); i+=3) {
        drawer->pen_color(255, 0, 0);
        double x1 = del->coords[2 * del->triangles[i]];         //tx0
        double y1 = del->coords[2 * del->triangles[i] + 1];     //ty0
        double x2 = del->coords[2 * del->triangles[i + 1]];     //tx1
        double y2 = del->coords[2 * del->triangles[i + 1] + 1]; //ty1
        double x3 = del->coords[2 * del->triangles[i + 2]];    //tx2
        double y3 = del->coords[2 * del->triangles[i + 2] + 1];  //ty2
        drawer->triangle( x1, y1, x2, y2, x3, y3 );

        //drawer->triangle(
        //    del->coords[2 * del->triangles[i]],         //tx0
        //    del->coords[2 * del->triangles[i] + 1],     //ty0
        //    del->coords[2 * del->triangles[i + 1]],     //tx1
        //    del->coords[2 * del->triangles[i + 1] + 1], //ty1
        //    del->coords[2 * del->triangles[i + 2]],     //tx2
        //    del->coords[2 * del->triangles[i + 2] + 1]  //ty2
        //);



        auto center = circumcenter(double_pair_t{x1, y1}, double_pair_t{x2, y2}, double_pair_t{x3, y3});
        drawer->pen_color(0, 0, 0);
        drawer->circle(center.first, center.second, 5);

        //draw the centers of each point bluish
        drawer->pen_color(0, 255, 255);
        drawer->circle(x1, y1, 5);
        drawer->circle(x2, y2, 5);
        drawer->circle(x3, y3, 5);

    }
    canvas->image().save_image("delaunator_output.bmp");

    return del;
}

void generate_points_for_del(int width_height, const ColorData& color_data, int num_points, std::vector<double>& points, v_double_pair_t& point_pairs)
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rng_seed); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(0, color_data.width);
    //std::uniform_int_distribution<> y_distrib(0, color_data.height);
    points.clear();
    point_pairs.clear();

    // for (int i = 0; i < num_points; i++) {
    //     int x = distrib(gen);
    //     int y = distrib(gen);
    //     points.push_back((double)x);
    //     points.push_back((double)y);
    //     edge_points.push_back(std::make_pair((double)x, (double)y));
    // }

    point_pairs.push_back(std::make_pair(338, 601));
    point_pairs.push_back(std::make_pair(357, 469));
    point_pairs.push_back(std::make_pair(200, 583));
    point_pairs.push_back(std::make_pair(424, 634));
    point_pairs.push_back(std::make_pair(302, 516));
    point_pairs.push_back(std::make_pair(265, 650));
    point_pairs.push_back(std::make_pair(459, 570));
    point_pairs.push_back(std::make_pair(367, 723));
    point_pairs.push_back(std::make_pair(453, 454));
    point_pairs.push_back(std::make_pair(220, 472));
    point_pairs.push_back(std::make_pair(326, 367));
    point_pairs.push_back(std::make_pair(424, 369));
    point_pairs.push_back(std::make_pair(393, 544));
    point_pairs.push_back(std::make_pair(121, 661));
    point_pairs.push_back(std::make_pair(180, 704));
    point_pairs.push_back(std::make_pair(126, 516));
    point_pairs.push_back(std::make_pair(87,  588));
    point_pairs.push_back(std::make_pair(523, 614));
    point_pairs.push_back(std::make_pair(472, 730));
    point_pairs.push_back(std::make_pair(277, 751));
    point_pairs.push_back(std::make_pair(585, 554));
    point_pairs.push_back(std::make_pair(532, 497));
    point_pairs.push_back(std::make_pair(351, 835));
    point_pairs.push_back(std::make_pair(450, 819));
    point_pairs.push_back(std::make_pair(511, 377));
    point_pairs.push_back(std::make_pair(586, 429));
    point_pairs.push_back(std::make_pair(251, 396));
    point_pairs.push_back(std::make_pair(164, 416));
    point_pairs.push_back(std::make_pair(224, 322));
    point_pairs.push_back(std::make_pair(299, 238));
    point_pairs.push_back(std::make_pair(404, 275));
    point_pairs.push_back(std::make_pair(470, 315));
    point_pairs.push_back(std::make_pair(121, 762));
    point_pairs.push_back(std::make_pair(2,   630));
    point_pairs.push_back(std::make_pair(21,  699));
    point_pairs.push_back(std::make_pair(186, 828));
    point_pairs.push_back(std::make_pair(40,  412));
    point_pairs.push_back(std::make_pair(28,  496));
    point_pairs.push_back(std::make_pair(572, 699));
    point_pairs.push_back(std::make_pair(646, 616));
    point_pairs.push_back(std::make_pair(566, 811));
    point_pairs.push_back(std::make_pair(275, 856));
    point_pairs.push_back(std::make_pair(622, 493));
    point_pairs.push_back(std::make_pair(720, 565));
    point_pairs.push_back(std::make_pair(382, 945));
    point_pairs.push_back(std::make_pair(257, 933));
    point_pairs.push_back(std::make_pair(475, 895));
    point_pairs.push_back(std::make_pair(554, 890));
    point_pairs.push_back(std::make_pair(512, 255));
    point_pairs.push_back(std::make_pair(627, 367));
    point_pairs.push_back(std::make_pair(570, 314));
    point_pairs.push_back(std::make_pair(712, 448));
    point_pairs.push_back(std::make_pair(126, 337));
    point_pairs.push_back(std::make_pair(122, 265));
    point_pairs.push_back(std::make_pair(212, 216));
    point_pairs.push_back(std::make_pair(397, 183));
    point_pairs.push_back(std::make_pair(326, 154));
    point_pairs.push_back(std::make_pair(35 , 833));
    point_pairs.push_back(std::make_pair(117, 848));
    point_pairs.push_back(std::make_pair(145, 935));
    point_pairs.push_back(std::make_pair(58,  296));
    point_pairs.push_back(std::make_pair(652, 769));
    point_pairs.push_back(std::make_pair(653, 686));
    point_pairs.push_back(std::make_pair(770, 664));
    point_pairs.push_back(std::make_pair(628, 884));
    point_pairs.push_back(std::make_pair(697, 846));
    point_pairs.push_back(std::make_pair(816, 549));
    point_pairs.push_back(std::make_pair(454, 980));
    point_pairs.push_back(std::make_pair(586, 971));
    point_pairs.push_back(std::make_pair(670, 968));
    point_pairs.push_back(std::make_pair(508, 119));
    point_pairs.push_back(std::make_pair(573, 179));
    point_pairs.push_back(std::make_pair(697, 247));
    point_pairs.push_back(std::make_pair(755, 324));
    point_pairs.push_back(std::make_pair(625, 264));
    point_pairs.push_back(std::make_pair(783, 442));
    point_pairs.push_back(std::make_pair(73,  178));
    point_pairs.push_back(std::make_pair(142, 186));
    point_pairs.push_back(std::make_pair(188, 123));
    point_pairs.push_back(std::make_pair(408,  98));
    point_pairs.push_back(std::make_pair(318,  68));
    point_pairs.push_back(std::make_pair(21,  948));
    point_pairs.push_back(std::make_pair(9,   207));
    point_pairs.push_back(std::make_pair(782, 761));
    point_pairs.push_back(std::make_pair(717, 730));
    point_pairs.push_back(std::make_pair(854, 729));
    point_pairs.push_back(std::make_pair(894, 659));
    point_pairs.push_back(std::make_pair(741, 916));
    point_pairs.push_back(std::make_pair(815, 896));
    point_pairs.push_back(std::make_pair(884, 580));
    point_pairs.push_back(std::make_pair(856, 416));
    point_pairs.push_back(std::make_pair(898, 511));
    point_pairs.push_back(std::make_pair(804, 996));
    point_pairs.push_back(std::make_pair(599,  69));
    point_pairs.push_back(std::make_pair(472,  10));
    point_pairs.push_back(std::make_pair(665, 138));
    point_pairs.push_back(std::make_pair(821, 232));
    point_pairs.push_back(std::make_pair(775, 147));
    point_pairs.push_back(std::make_pair(884, 279));
    point_pairs.push_back(std::make_pair(98,   53));
    point_pairs.push_back(std::make_pair(17,   94));
    point_pairs.push_back(std::make_pair(208,  28));
    point_pairs.push_back(std::make_pair(381,   5));
    point_pairs.push_back(std::make_pair(91,  980));
    point_pairs.push_back(std::make_pair(836, 811));
    point_pairs.push_back(std::make_pair(918, 766));
    point_pairs.push_back(std::make_pair(974, 706));
    point_pairs.push_back(std::make_pair(900, 843));
    point_pairs.push_back(std::make_pair(962, 580));
    point_pairs.push_back(std::make_pair(939, 930));
    point_pairs.push_back(std::make_pair(874, 969));
    point_pairs.push_back(std::make_pair(970, 492));
    point_pairs.push_back(std::make_pair(958, 327));
    point_pairs.push_back(std::make_pair(938, 405));
    point_pairs.push_back(std::make_pair(567,   0));
    point_pairs.push_back(std::make_pair(700,  16));
    point_pairs.push_back(std::make_pair(910, 184));
    point_pairs.push_back(std::make_pair(863, 106));
    point_pairs.push_back(std::make_pair(956, 248));
    point_pairs.push_back(std::make_pair(780,  21));
    point_pairs.push_back(std::make_pair(890, 354));
    point_pairs.push_back(std::make_pair(987, 801));
    point_pairs.push_back(std::make_pair(979,  96));
    point_pairs.push_back(std::make_pair(742 , 82));
    point_pairs.push_back(std::make_pair(917,  44));
    point_pairs.push_back(std::make_pair(853,  14));
    point_pairs.push_back(std::make_pair(986, 876));
    point_pairs.push_back(std::make_pair(986, 171));
    point_pairs.push_back(std::make_pair(980,  11));

    for (auto& edge_point: point_pairs) {
        points.push_back(edge_point.first);
        points.push_back(edge_point.second);
    }

}


bool PointInTriangle(coord_t p, coord_t p0, coord_t p1, coord_t p2) {
    float px, py;
    std::tie(px, py) = p;
    float p0x, p0y;
    std::tie(p0x, p0y) = p0;
    float p1x, p1y;
    std::tie(p1x, p1y) = p1;
    float p2x, p2y;
    std::tie(p2x, p2y) = p2;

    float A = 1.0f / 2.0f * (-p1y * p2x + p0y * (-p1x + p2x) + p0x * (p1y - p2y) + p1x * p2y);
    int sign = A < 0 ? -1 : 1;
    float s = (p0y * p2x - p0x * p2y + (p2y - p0y) * px + (p0x - p2x) * py) * sign;
    float t = (p0x * p1y - p0y * p1x + (p0y - p1y) * px + (p1x - p0x) * py) * sign;
    
    return s > 0 && t > 0 && (s + t) < 2.0f * A * sign;
}
//float sign (coord_t p1, coord_t p2, coord_t p3)
//{
//    return (p1.first - p3.first) * (p2.second - p3.second) - (p2.first - p3.first) * (p1.second - p3.second);
//}

//bool PointInTriangle (coord_t pt, coord_t v1, coord_t v2, coord_t v3)
//{
//    float d1, d2, d3;
//    bool has_neg, has_pos;
//
//    d1 = sign(pt, v1, v2);
//    d2 = sign(pt, v2, v3);
//    d3 = sign(pt, v3, v1);
//
//    has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
//    has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
//
//    return !(has_neg && has_pos);
//}



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
    int width_height = 512*1;
    color_data.width = width_height;
    color_data.height = width_height;
    color_data.red   = new unsigned char [color_data.height*color_data.width];
    color_data.green = new unsigned char [color_data.height*color_data.width];
    color_data.blue  = new unsigned char [color_data.height*color_data.width];


    int num_points = 20;
    std::vector<double>  points{};
    v_double_pair_t point_pairs;

    cartesian_canvas canvas(width_height, width_height);
    image_drawer drawer(canvas.image());
    delaunator::Delaunator* del;
    auto regenerate_canvas = [&]() {
        generate_points_for_del(width_height, color_data, num_points, points, point_pairs);
        del = draw_del_points_to_canvas(points, &canvas, &drawer);

        //for (auto& coords: edge_points) {
        //    std::wstringstream ss;
        //    ss << "(" << coords.first << ", " << coords.second << ")";
        //    my_print(ss.str());
        //}
    };
    regenerate_canvas();


    auto nextHalfEdge = [](edge_t edge) { return (edge % 3 == 2) ? edge - 2 : edge + 1;  };

    auto triangleOfEdge = [](edge_t e) {
        return e / 3;
    };

    //returns 3 edge IDs
    auto edgesOfTriangle = [](edge_t t) -> std::vector<edge_t> {
        return std::vector<edge_t>{3* t, 3* t + 1, 3 * t + 2};
    };

    //finds 3 points for a given triangle id
    auto pointsOfTriangle = [edgesOfTriangle](const delaunator::Delaunator& delaunator, edge_t tri_id) ->std::vector<size_t> {
        auto edges = edgesOfTriangle(tri_id);

        std::vector<size_t> points;
        auto find_tri_for_edge = [delaunator](edge_t e) -> size_t{ return delaunator.triangles[e]; };
        std::transform(edges.begin(), edges.end(), std::back_inserter(points), find_tri_for_edge);

        return points;
    };

    auto edgesAroundPoint = [nextHalfEdge](const delaunator::Delaunator& delaunator, edge_t start) {
        std::vector<edge_t> result;

        edge_t incoming = start;
        do {
            result.push_back(incoming);
            const edge_t outgoing = nextHalfEdge(incoming);
            incoming = delaunator.halfedges[outgoing];
        } while (incoming != (edge_t)-1 && incoming != start);

        return result;
    };

    //circumcenter of triangle
    auto triangleCenter = [pointsOfTriangle](v_double_t points, delaunator::Delaunator& delaunator, edge_t tri_id) -> std::pair<double, double>
    {
        auto tri_points = pointsOfTriangle(delaunator, tri_id);
        std::vector<std::pair<double, double>> vertices{};
        std::transform(tri_points.begin(), tri_points.end(), std::back_inserter(vertices), [&points, &delaunator](edge_t tri_point) {
            return double_pair_t{delaunator.coords.at(tri_point*2), delaunator.coords.at(tri_point*2 + 1)};
        });

        auto result = delaunator::circumcenter(
            vertices[0].first, vertices[0].second,
            vertices[1].first, vertices[1].second,
            vertices[2].first, vertices[2].second
        );
        return std::pair<double, double>(result.first, result.second);
    };

    auto forEachVoronoiEdge = [&](delaunator::Delaunator& delaunator, std::function<void(edge_t, double_pair_t, double_pair_t)> callback) {
        //forEachTriangleEdge
        //for (unsigned e = 0;  e < delaunator.triangles.size(); e++) {
        //    auto halfedge = delaunator.halfedges[e];
        //    if (e < halfedge && halfedge != delaunator::INVALID_INDEX) {
        //        try{
        //            auto p = double_pair_t{delaunator.coords.at(delaunator.triangles[e] * 2), delaunator.coords.at(delaunator.triangles[e] * 2 + 1)};
        //            auto q = double_pair_t{delaunator.coords.at(delaunator.triangles[nextHalfEdge(e)] * 2), delaunator.coords.at(delaunator.triangles[nextHalfEdge(e)] * 2 + 1)};
        //            callback(e, p, q);
        //        }
        //        catch (std::out_of_range& ex) { my_print(L"invalid"); }
        //    }
        //}

        ////forEachVoronoiEdge
        for (unsigned e = 0;  e < delaunator.triangles.size(); e++) {
            auto halfedge = delaunator.halfedges[e];
            if (e > halfedge && halfedge != delaunator::INVALID_INDEX) {
                edge_t pt = triangleOfEdge(e);
                double_pair_t p = triangleCenter(delaunator.coords, delaunator, pt);

                edge_t qt = triangleOfEdge(delaunator.halfedges[e]);
                double_pair_t q = triangleCenter(delaunator.coords, delaunator, qt);

                callback(e, p, q);
            }
        }
    };

    auto forEachVoronoiCell = [nextHalfEdge, edgesAroundPoint, triangleOfEdge, triangleCenter](std::vector<coord_t> points, delaunator::Delaunator& delaunator, std::function<void(edge_t, std::vector<coord_t>)> callback) {

        std::set<unsigned int> seen_points;
        //for (const int& edge : delaunator.triangles) {
        for (unsigned edge = 0; edge < delaunator.triangles.size(); edge++) {
            unsigned int point = delaunator.triangles[nextHalfEdge(edge)];

            if (std::find(seen_points.begin(), seen_points.end(), point) == seen_points.end()) {
                seen_points.insert(point);
                std::vector<edge_t> edges = edgesAroundPoint(delaunator, edge);


                std::vector<edge_t> triangles{};
                std::transform(edges.begin(), edges.end(), std::back_inserter(triangles), triangleOfEdge);

                std::vector<coord_t> vertices{};
                std::transform(triangles.begin(), triangles.end(), std::back_inserter(vertices), [&triangleCenter, &points, &delaunator](edge_t tri_id) {
                    //auto result = triangleCenter(points, delaunator, tri_id);
                    //return result;

                    return coord_t{-1, -1};
                });

                callback(point, vertices);
            }
        }
    };

    //image_drawer drawer(canvas.image());
     auto draw_edges = [&drawer, &canvas, width_height](edge_t cell_id, double_pair_t e1, double_pair_t e2) {
         drawer.pen_color(0, 0, 0);
         auto draw_a_to_b = [&drawer, &canvas, width_height](double_pair_t& a, double_pair_t& b) {
             std::wstringstream ss;
             ss << "Drawing Edge: ";
             ss << "(" << a.first << ", " << a.second << ") ";
             ss << "(" << b.first << ", " << b.second << ") ";
             my_print(ss.str());
             int mul = 4;
             int x_offset = -(width_height/2);
             int y_offset = (width_height/2);
             int offset = 0;
             canvas.line_segment(a.first + x_offset, + y_offset - a.second , b.first + x_offset,  y_offset - b.second);
             //drawer.line_segment(a.first, a.second, b.first, b.second);
         };

         draw_a_to_b(e1, e2);
    
     };
     auto draw_vertices = [&drawer, &canvas, width_height](edge_t cell_id, std::vector<coord_t>& vertices) {
         drawer.pen_color(cell_id * 10, 0, 0);
         auto draw_a_to_b = [&drawer, &canvas](coord_t& a, coord_t& b) {
             canvas.line_segment(a.first, a.second, b.first, b.second);
             //drawer.line_segment(a.first, a.second, b.first, b.second);
         };
    
         auto size = vertices.size();
         if (size <= 2) {
             my_print(L"skipping: num vertices: " + std::to_wstring(size));
             return;
         }
    
         //my_print(L"num vertices: "+std::to_wstring(size));
         for (unsigned int i = 0; i < size; i+=1) {
             auto lerp = [width_height](edge_t val) {
                 return 0 + val*(width_height-0);
             };
    
             //canvas.line_segment(lerp(a.first)-0.5, lerp(a.second)-0.5, lerp(b.first)-0.5, lerp(b.second)-0.5);
             //canvas.fill_triangle(
             //    vertices[i].first, vertices[i].second,
             //    vertices[i + 1].first, vertices[i + 1].second,
             //    vertices[i + 2].first, vertices[i + 2].second
             //);
             if (i != size - 1){
                 draw_a_to_b(vertices[i], vertices[i + 1]);
             } else {
                 draw_a_to_b(vertices[i], vertices[0]);
             }
         }
     };

    //delaunator::Delaunator delaunator2(points);
    //canvas.image().clear();
    //canvas.image().set_all_channels(255, 255, 255);
    //forEachVoronoiCell(edge_points, *del, draw_vertices);

    //canvas.image().clear();
    //canvas.image().set_all_channels(240, 240, 240);
    forEachVoronoiEdge(*del, draw_edges);
    //canvas.image().save_image("delaunator_output_vornoi.bmp");

    TextureData* new_texture_data = nullptr;

    new_texture_data = create_texture_data_from_image(&canvas.image());


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

            if (ImGui::InputInt("Voronoi Seed", &rng_seed, 1, 100)) {
                
                regenerate_canvas();
                forEachVoronoiEdge(*del, draw_edges);
                TEXTURE_DATA = create_texture_data_from_image(&canvas.image());
            }

            if (ImGui::Button("Regenerate")) {
                //TEXTURE_DATA = create_texture_from_memory(red, green, blue, width, height);
                //ColorData color_data = create_voronoi_color_data(width_height, rng_seed);
                //TEXTURE_DATA = create_texture_from_rgb_array(color_data);
                regenerate_canvas();
                forEachVoronoiEdge(*del, draw_edges);
                TEXTURE_DATA = create_texture_data_from_image(&canvas.image());
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

                ////vornoi position
                //noise->SetCellularReturnType(FastNoise::Distance);
                //float distance = noise->GetNoise(mouse_pos.x, mouse_pos.y);
                //noise->SetCellularReturnType(FastNoise::CellValue);
                //float cell_value = noise->GetNoise(mouse_pos.x, mouse_pos.y);
                //ImGui::LabelText("Distance, Id", "%f, %f", distance, (cell_value+1)/2*255);

                //delanay triangle
                float distance = -1.0f;
                float cell_id = -1.0f;
                //for(std::size_t i = 0; i < del->triangles.size(); i+=3) {
                //    int x1 = del->coords[2 * del->triangles[i]];         //tx0
                //    int y1 = del->coords[2 * del->triangles[i] + 1];     //ty0
                //    int x2 = del->coords[2 * del->triangles[i + 1]];     //tx1
                //    int y2 = del->coords[2 * del->triangles[i + 1] + 1]; //ty1
                //    int x3 = del->coords[2 * del->triangles[i + 2]];    //tx2
                //    int y3 = del->coords[2 * del->triangles[i + 2] + 1];  //ty2

                //    coord_t mouse_pos_pair = std::make_pair(mouse_pos.x, mouse_pos.y);

                //    bool point_in_tri = PointInTriangle(
                //        mouse_pos_pair, std::make_pair(x1, y1),
                //        std::make_pair(x2, y2), std::make_pair(x3, y3)
                //    );
                //    if (point_in_tri) {
                //        cell_id = i;
                //    }
                //}

                double min_distance = 99999;
                for (int i = 0; i < del->coords.size(); i+=2) {
                    auto x = del->coords[i];
                    auto y = del->coords[i+1];

                    double dist = sqrt(pow(x - mouse_pos.x, 2)+ pow(y - mouse_pos.y, 2));
                    if (dist < min_distance) {
                        cell_id = i;
                        distance = dist;
                        min_distance = dist;
                    }
                }

                ImGui::LabelText("Distance, Id", "%f, %f", distance, cell_id);
            }
            ImGui::End();
        }

        //// 3. Show another simple window.
        //if (show_another_window)
        //{
        //    ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        //    ImGui::Text("Hello from another window!");
        //    if (ImGui::Button("Close Me"))
        //        show_another_window = false;
        //    ImGui::End();
        //}

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
