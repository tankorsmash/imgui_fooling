﻿// dear imgui - standalone example application for DirectX 11
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
#include <numeric>
#include <random>
#include <set>
#include <thread>
#include <unordered_map>
#include <unordered_set>



#include "ui.h"
#include "FastNoise.h"
#include "imgui_internal.h"

#include "voronoi.h"
#include "delaunator.hpp"

#define BEND(what) what.begin(), what.end()

enum class ColorType;
using coord_t = std::pair<int, int>;
using edge_t = size_t;
using v_double_t = std::vector<double>;
using double_pair_t = std::pair<double, double>;
using v_double_pair_t = std::vector<double_pair_t>;

static v_double_t ORIG_POINTS{};
static int rng_seed = 12345;

void set_pen_color(ColorType color_type);

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

struct RGB
{
    unsigned char r, g, b;
};
enum class ColorType
{
    CellHighlight,
    CellEdge,
    CellCorner,
    TriangleEdge,
    TriangleCorner,
    Coord,
    TriangleCenter,
    HullColor
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

//draws points to canvas, but also creates the Delaunator instance
delaunator::Delaunator* draw_del_points_to_canvas(const std::vector<double>& points, cartesian_canvas* canvas, image_drawer* drawer)
{
    //v_double_t points2 = points;
    delaunator::Delaunator* del = new delaunator::Delaunator(points);
    canvas->image().clear();
    canvas->image().set_all_channels(200, 200, 200);

    for (std::size_t i = 0; i < del->triangles.size(); i += 3) {
        double x1 = del->coords[2 * del->triangles[i + 0] + 0]; //tx0
        double y1 = del->coords[2 * del->triangles[i + 0] + 1]; //ty0
        double x2 = del->coords[2 * del->triangles[i + 1] + 0]; //tx1
        double y2 = del->coords[2 * del->triangles[i + 1] + 1]; //ty1
        double x3 = del->coords[2 * del->triangles[i + 2] + 0]; //tx2
        double y3 = del->coords[2 * del->triangles[i + 2] + 1]; //ty2
        set_pen_color(ColorType::TriangleEdge);
        drawer->triangle( x1, y1, x2, y2, x3, y3 );

        drawer->pen_color(0, 0, 255);
        auto center = circumcenter(double_pair_t{x1, y1}, double_pair_t{x2, y2}, double_pair_t{x3, y3});
        //drawer->circle(center.first, center.second, 5);

        //draw the centers of each point bluish
        set_pen_color(ColorType::TriangleCorner);
        drawer->circle(x1, y1, 8);
        drawer->circle(x2, y2, 8);
        drawer->circle(x3, y3, 8);

    }


    //draw the raw coords
    for (int i = 0; i < del->coords.size(); i+=2) {
        set_pen_color(ColorType::Coord);
        auto x = del->coords[i];
        auto y = del->coords[i+1];
        drawer->circle(
            x, y, 5
        );
    }

    return del;
}

// A hash function used to hash a pair of any kind 
struct hash_pair { 
    template <class T1, class T2> 
    size_t operator()(const std::pair<T1, T2>& p) const
    { 
        auto hash1 = std::hash<T1>{}(p.first); 
        auto hash2 = std::hash<T2>{}(p.second); 
        return hash1 ^ hash2; 
    } 
}; 

namespace lloyd
{
    //X is all the points
    //mu is a random subselection of X
    std::unordered_map<size_t, v_double_pair_t> cluster_points(v_double_pair_t X, v_double_pair_t mu)
    {
        //mu idx : norm
        std::unordered_map<size_t, v_double_pair_t> result;

        for (const auto& pair : X) {
            double px = pair.first;
            double py = pair.second;

            //find the lowest norm in mu from the pair
            v_double_pair_t norms;
            for (int i = 0; i < mu.size(); i++) {
                double mx = mu[i].first;
                double my = mu[i].second;

                double_pair_t tmp = {px - mx, py - my};
                double norm = std::sqrt(tmp.first * tmp.first + tmp.second * tmp.second); //pretty sure norm is just the magnitude/length of vector
                norms.push_back({(double)i, norm});
            }

            auto smallest_norm = std::min_element(norms.begin(), norms.end(), [](double_pair_t& left, double_pair_t& right) {
                return left.second < right.second;
            });

            result[smallest_norm->first].push_back(pair);

        }

        return result;
    }

    //`mu` is the subsection of total points im testing
    //`clusters` is how far apart they are from each point in the total set of points
    v_double_pair_t reevaluate_centers(v_double_pair_t& mu, std::unordered_map<unsigned, v_double_pair_t>& clusters)
    {
        v_double_pair_t new_mu{};

        std::vector<unsigned> keys;
        for (auto& cluster: clusters) {
            keys.push_back(cluster.first);
        }
        std::sort(keys.begin(), keys.end());

        //std::sort(clusters.begin(), clusters.end(), [](const std::pair<unsigned, v_double_t> cluster) {
        //    return cluster.first;
        //});

        //calc average of the points in cluster
        for (auto& k: keys) {
            auto cluster = clusters[k];
            double x_mean = std::accumulate(cluster.begin(), cluster.end(), 0.0, [](double total, const double_pair_t& coord) {
                return coord.first + total;
            })/cluster.size();
            double y_mean = std::accumulate(cluster.begin(), cluster.end(), 0.0, [](double total, const double_pair_t& coord) {
                return coord.second + total;
            })/cluster.size();
            new_mu.push_back({x_mean, y_mean});
        }

        return new_mu;
    }

    bool has_converged(v_double_pair_t& mu, v_double_pair_t& old_mu)
    {
        std::unordered_set<double_pair_t, hash_pair> mu_set{};
        for (auto& mval: mu) {
            mu_set.insert(mval);
        }

        std::unordered_set<double_pair_t, hash_pair> old_mu_set{};
        for (auto& old_mval: old_mu) {
            old_mu_set.insert(old_mval);
        }

        return mu_set == old_mu_set;
    }

    //X are all points, K is the amount of centers i want to find
    std::pair<v_double_pair_t, std::unordered_map<unsigned, v_double_pair_t>> find_centers(v_double_pair_t& X, unsigned K)
    {
        assert(K <= X.size());

        //hack to randomly choose K-count points from X
        v_double_pair_t old_mu{};
        std::random_shuffle(X.begin(), X.end());
        old_mu.insert(old_mu.begin(), X.begin(), X.begin() + K);

        //hack to randomly choose K-count points from X
        v_double_pair_t mu{};
        std::random_shuffle(X.begin(), X.end());
        mu.insert(mu.begin(), X.begin(), X.begin() + K);

        std::unordered_map<size_t, v_double_pair_t> clusters;
        while (!has_converged(mu, old_mu)) {
            old_mu = mu;

            //assign all points in X to clusters
            clusters = cluster_points(X, mu);

            //reeval centers
            mu = reevaluate_centers(mu, clusters);
        }

        std::pair<v_double_pair_t, std::unordered_map<unsigned, v_double_pair_t>> result = std::make_pair(mu, clusters);
        return result;
    }

}

void generate_points_for_del(int width_height, const ColorData& color_data, int num_points, std::vector<double>& points)
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rng_seed); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(0, color_data.width);
    points.clear();
    ORIG_POINTS.clear();


    v_double_pair_t point_pairs{};

     for (int i = 0; i < num_points; i++) {
         int x = distrib(gen);
         int y = distrib(gen);
         points.push_back((double)x);
         points.push_back((double)y);
         ORIG_POINTS.push_back((double)x);
         ORIG_POINTS.push_back((double)y);

         point_pairs.push_back({x, y});
     }

     //double num_clusters = 250;
     //point_pairs = lloyd::find_centers(point_pairs, num_clusters).first;
     //point_pairs = lloyd::find_centers(point_pairs, num_clusters).first;
     //point_pairs = lloyd::find_centers(point_pairs, num_clusters).first;
     //point_pairs = lloyd::find_centers(point_pairs, num_clusters).first;

    //now point_pairs has a limited set of points clustered together, I think

     //points.clear();
     //for (auto& point: point_pairs) {
     //    points.push_back(point.first);
     //    points.push_back(point.second);
     //}
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

static int width_height = 512 * 1;
static cartesian_canvas canvas(width_height, width_height);
static image_drawer drawer(canvas.image());
static delaunator::Delaunator* del;
std::vector<double_pair_t> hull_verts;

static std::unordered_map<ColorType, RGB> COLOR_MAP{};

void set_pen_color(ColorType color_type)
{
    static bool _colors_setup = false;
    if (_colors_setup == false) {
        COLOR_MAP[ColorType::CellHighlight] = {175, 94, 255};
        COLOR_MAP[ColorType::CellEdge] = {94, 255, 255};
        COLOR_MAP[ColorType::CellCorner] = {255, 100, 190};
        COLOR_MAP[ColorType::TriangleEdge] = {255, 215, 94};
        COLOR_MAP[ColorType::TriangleCorner] = {80, 255, 10};
        COLOR_MAP[ColorType::Coord] = {0, 0, 255};
        COLOR_MAP[ColorType::TriangleCenter] = {255, 115, 0};
        COLOR_MAP[ColorType::HullColor] = {0, 150, 0};
    }

    RGB pen_color = COLOR_MAP.at(color_type);
    drawer.pen_color(pen_color.r, pen_color.g, pen_color.b);
}

void draw_a_to_b(double_pair_t& a, double_pair_t& b) {
    //if (a.first > 10000 || a.second > 10000 || b.first > 10000 || b.second > 100000) {
    //    __debugbreak();
    //    return;
    //}
    drawer.line_segment(a.first, a.second, b.first, b.second);
};

void draw_a_to_b(double x1, double y1, double x2, double y2)
{
    draw_a_to_b(double_pair_t{x1, y1}, double_pair_t{x2, y2});
}

void draw_a_to_b(coord_t& a, coord_t& b) {
    if (a.first == delaunator::INVALID_INDEX ||
        a.second == delaunator::INVALID_INDEX ||
        b.first == delaunator::INVALID_INDEX ||
        b.second == delaunator::INVALID_INDEX) {
        return;
    }

    draw_a_to_b(a.first, a.second, b.first, b.second);
};

bool point_in_poly(v_double_pair_t& verts, double point_x, double point_y)
{
    bool in_poly = false;
    auto num_verts = verts.size();
    for (int i = 0, j = num_verts - 1; i < num_verts; j = i++) {
        double x1 = verts[i].first;
        double y1 = verts[i].second;
        double x2 = verts[j].first;
        double y2 = verts[j].second;

        if (((y1 > point_y) != (y2 > point_y)) &&
            (point_x < (x2 - x1) * (point_y - y1) / (y2 - y1) + x1))
            in_poly = !in_poly;
    }
    return in_poly;
}

int pnpoly(int num_verts, double *vert_xs, double *vert_ys, double point_x, double point_y)
{
    int c = 0;
    for (int i = 0, j = num_verts - 1; i < num_verts; j = i++) {
        if (((vert_ys[i] > point_y) != (vert_ys[j] > point_y)) &&
            (point_x < (vert_xs[j] - vert_xs[i]) * (point_y - vert_ys[i]) / (vert_ys[j] - vert_ys[i]) + vert_xs[i]))
            c = !c;
    }
    return c;
}

void forEachVoronoiCell(delaunator::Delaunator& delaunator,
    std::function<void(edge_t, std::vector<coord_t>&)> callback)
{

    std::set<edge_t> seen_points;
    //for (const int& edge : delaunator.triangles) {
    for (edge_t edge = 0; edge < delaunator.triangles.size(); edge++) {
        auto next_edge = nextHalfEdge(edge);
        edge_t triangle_id = delaunator.triangles[next_edge];

        if (std::find(BEND(seen_points), triangle_id) == seen_points.end()) {
            seen_points.insert(triangle_id);
            std::vector<edge_t> edges = edgesAroundPoint(delaunator, edge);


            //get the triangle_id for each edge
            std::vector<edge_t> triangles{};
            std::transform(BEND(edges), std::back_inserter(triangles), triangleOfEdge);

            std::vector<double> coords;
            for (edge_t& tri_id: triangles) {

                for (edge_t& edge_id : pointsOfTriangle(*del, tri_id)) {
                    double x = del->coords[edge_id * 2];
                    double y = del->coords[edge_id * 2 + 1];
                    coords.push_back(x);
                    coords.push_back(y);
                }
            }

            //for each triangle found, calculate the circumcenter
            std::vector<coord_t> vertices{};
            std::transform(
                BEND(triangles), std::back_inserter(vertices),
                [&delaunator](edge_t tri_id) {
                    auto result = triangleCenter(delaunator, tri_id);
                    return result;
                }
            );

            //draw the triangle
            callback(triangle_id, vertices);
        }
    }



    set_pen_color(ColorType::HullColor);
    hull_verts.clear();
    size_t e = del->hull_start;
    do {
        double x1 = del->coords[2 * e];
        double y1 = del->coords[2 * e + 1];
        double x2 = del->coords[2 * del->hull_prev[e]];
        double y2 = del->coords[2 * del->hull_prev[e] + 1];
        draw_a_to_b(double_pair_t{x1, y1}, double_pair_t{x2, y2});
        //hull_verts.push_back(double_pair_t{x1, y1});
        hull_verts.push_back(double_pair_t{x2, y2});
        e = del->hull_next[e];
    } while (e != del->hull_start);

        
};

void regenerate_canvas()
{
    std::vector<double>  points{};
    ColorData color_data;
    color_data.width = width_height;
    color_data.height = width_height;
    color_data.red = new unsigned char[color_data.height*color_data.width];
    color_data.green = new unsigned char[color_data.height*color_data.width];
    color_data.blue = new unsigned char[color_data.height*color_data.width];

    int num_points = 50;
    generate_points_for_del(width_height, color_data, num_points, points);
    del = draw_del_points_to_canvas(points, &canvas, &drawer);
};


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


    regenerate_canvas();

     auto draw_vertices_coord_with_id = [](edge_t cell_id, std::vector<coord_t>& vertices, double point_id) {

         auto size = vertices.size();
         if (size <= 2) {
             return;
         }

         drawer.pen_color(255, 0, 0);
         bool is_valid = true;
         for (edge_t i = 0; i < size; i += 1) {
             coord_t a = vertices[i];
             coord_t b = {0, 0};
             if (i != size - 1) {
                 b = vertices[i + 1];
             } else {
                 b = vertices[0];
             }

             if ((!point_in_poly(hull_verts, a.first, a.second)) ||
                 (!point_in_poly(hull_verts, b.first, b.second))) {
                 is_valid = false;
             }
         }
         if (is_valid) {
             for (edge_t i = 0; i < size; i += 1) {
                 coord_t& a = vertices[i];
                 coord_t& b = coord_t{0, 0};
                 if (i != size - 1) {
                     b = vertices[i + 1];
                 } else {
                     b = vertices[0];
                 }

                 draw_a_to_b(a, b);
             }

         }

         if (cell_id == point_id )
         {
             for (edge_t i = 0; i < size; i += 1) {
                 for (edge_t j = 0; j < size; j += 1) {
                     set_pen_color(ColorType::CellHighlight);
                     draw_a_to_b(vertices[i], vertices[j]);
                 }
             }
         }
     };
     auto draw_vertices_coord = [](edge_t cell_id, std::vector<coord_t>& vertices) {
         auto size = vertices.size();
         if (size <= 1) {
             //my_print(L"skipping: num vertices: " + std::to_wstring(size));
             return;
         }

         for (unsigned int i = 0; i < size; i+=1) {
             coord_t& a = vertices[i];
             coord_t& b = coord_t{-1, -1};

             if (i != size - 1){
                 b = vertices[i + 1];
             } else {
                 b = vertices[0];
             }

             set_pen_color(ColorType::CellEdge);
             draw_a_to_b(a, b);

             set_pen_color(ColorType::CellCorner); //TODO better color for this
             drawer.circle(a.first, a.second, 5);
             drawer.circle(b.first, b.second, 5);
         }
     };


    forEachVoronoiCell(*del, draw_vertices_coord);


    static TextureData* TEXTURE_DATA = create_texture_data_from_image(&canvas.image());


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

            ImGui::Text("framerate %f", ImGui::GetIO().Framerate);

            if (ImGui::InputInt("Voronoi Seed", &rng_seed, 1, 100)) {

                regenerate_canvas();
                //forEachVoronoiEdge(*del, draw_edges);
                forEachVoronoiCell(*del, draw_vertices_coord);
                TEXTURE_DATA = create_texture_data_from_image(&canvas.image());
            }

            if (ImGui::Button("Regenerate")) {
                //TEXTURE_DATA = create_texture_from_memory(red, green, blue, width, height);
                //ColorData color_data = create_voronoi_color_data(width_height, rng_seed);
                //TEXTURE_DATA = create_texture_from_rgb_array(color_data);
                regenerate_canvas();
                //forEachVoronoiEdge(*del, draw_edges);
                forEachVoronoiCell(*del, draw_vertices_coord);
                TEXTURE_DATA = create_texture_data_from_image(&canvas.image());
            }
            ImGui::Text("pointer = %p", TEXTURE_DATA->texture_id);
            ImGui::Text("size = %d x %d", TEXTURE_DATA->image_width, TEXTURE_DATA->image_height);


            ImGuiWindow* window = ImGui::GetCurrentWindow();
            ImGui::LabelText("Mouse Pos:", "%f %f", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);

            auto color_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoPicker;

            std::vector<std::pair<ColorType, std::string>> color_names = {
                {ColorType::TriangleEdge, std::string("Triangle Edge")},
                {ColorType::TriangleCenter, std::string("Triangle Center")},
                {ColorType::CellEdge, std::string("Cell Edge")},
                {ColorType::CellHighlight, std::string("Cell Highlight")},
                {ColorType::Coord, std::string("Coord")},
                {ColorType::TriangleCorner, std::string("Triangle Corner")},
                {ColorType::HullColor, std::string("Hull Color")}
            };

            int i = 0;
            for (auto& color_name : color_names) {
                if (i % 3 != 0) {
                    ImGui::SameLine();
                }

                ImGui::BeginGroup();
                ImVec4 color = ImVec4(0, 0, 0, 0);
                auto rgb = &COLOR_MAP[color_name.first];
                color.x = rgb->r/255.0;
                color.y = rgb->g/255.0;
                color.z = rgb->b/255.0;
                ImGui::ColorEdit3(color_name.second.c_str(), (float*)&color, color_flags);
                ImGui::SameLine();
                ImGui::Text(color_name.second.c_str());
                ImGui::EndGroup();

                i++;
            }

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
                static float cached_cell_id = -10.0f;
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
                double found_x = -1;
                double found_y = -1;
                for (int i = 0; i < del->coords.size(); i+=2) {
                    auto x = del->coords[i];
                    auto y = del->coords[i+1];

                    double dist = sqrt(pow(x - mouse_pos.x, 2)+ pow(y - mouse_pos.y, 2));
                    if (dist < min_distance) {
                        cell_id = i;
                        distance = dist;
                        min_distance = dist;

                        found_x = x;
                        found_y = y;
                    }
                }

                if (cell_id != -1) {
                    double_pair_t point = {found_x, found_y};

                    for (int i = 0; i < del->coords.size(); i+=2) {
                        double x = del->coords[i];
                        double y = del->coords[i+1];

                        if (x == point.first) {
                            if (y == point.second) {
                                if (cell_id != cached_cell_id){
                                    canvas.image().clear();
                                    canvas.image().set_all_channels(100, 100, 100);

                                    forEachVoronoiCell(*del, [&](edge_t edge_id, std::vector<coord_t>& vertices){
                                        draw_vertices_coord_with_id(edge_id, vertices, cell_id / 2);
                                    });
                                    TEXTURE_DATA = create_texture_data_from_image(&canvas.image());

                                    cached_cell_id = cell_id;
                                }
                            } else {
                                //my_print(L"no it doesn't");
                            }
                        }
                    }
                    //auto edges = edgesAroundPoint(*del, )
                }

                ImGui::LabelText("Distance, Id", "%f, %f", distance, cell_id);

                int num_verts = hull_verts.size();
                double* vert_xs = new double[num_verts];
                double* vert_ys = new double[num_verts];
                for (int i = 0; i < num_verts; i++) {
                    vert_xs[i] = hull_verts[i].first;
                    vert_ys[i] = hull_verts[i].second;
                }
                int pnp = pnpoly(num_verts, vert_xs, vert_ys, mouse_pos.x, mouse_pos.y);

                ImGui::LabelText("Mouse in poly SO", "%i", pnp);
                ImGui::LabelText("Mouse in poly mine", "%i", point_in_poly(hull_verts, mouse_pos.x, mouse_pos.y));
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
