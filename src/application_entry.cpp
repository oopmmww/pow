#include "runtime/vendor/api_hidden.h"
// Dear ImGui: standalone application for DirectX 11

#include "headers/application_includes.h"
#include "data/font_registry.h"
#include "runtime/feature_controller.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui_freetype.h"
#include <tchar.h>
#include <d3dx11.h>
#include <dwmapi.h>
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "dwmapi.lib")

static bool g_SwapChainOccluded = false;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Standalone GUI application entry point.
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    Strike_Initialize();

    // Create application window
    WNDCLASSEXW wc = {
        sizeof(wc), CS_CLASSDC, WndProc,          0L,     0L, GetModuleHandle(nullptr), nullptr, nullptr,
        nullptr,    nullptr,    L"ImGui Example", nullptr};
    ::RegisterClassExW(&wc);
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    int win_width = (int)elements->window.size.x;
    int win_height = (int)elements->window.size.y;
    int win_x = (screen_width - win_width) / 2;
    int win_y = (screen_height - win_height) / 2;
    HWND hwnd =
        ::CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED, wc.lpszClassName, L"FREE FOR YOU", WS_POPUP, win_x,
                          win_y, win_width, win_height, nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd)
    {
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    var->winapi.hwnd = hwnd;

    ::SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS margins = {-1, -1, -1, -1};
    ::DwmExtendFrameIntoClientArea(hwnd, &margins);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    std::string path = "icons\\";
    widgets->load_texture(path + "user_logo.png", &var->img.user_logo);
    widgets->load_texture(path + "logo.png", &var->img.logo);

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);
    ::SetForegroundWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(var->dx.device, var->dx.ctx);

    // Our state
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && var->dx.swap_chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            var->dx.swap_chain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Toggle Show / Hide GUI Menu on configured keybind (Default: Home)
        static bool menu_visible = true;
        static bool prev_home_down = false;
        int menu_vk = Strike_GetMenuToggleVk();
        if (menu_vk <= 0)
            menu_vk = VK_HOME;
        bool home_down = (GetAsyncKeyState(menu_vk) & 0x8000) != 0;
        if (home_down && !prev_home_down)
        {
            menu_visible = !menu_visible;
            ShowWindow(hwnd, menu_visible ? SW_SHOW : SW_HIDE);
            if (menu_visible)
            {
                SetForegroundWindow(hwnd);
            }
        }
        prev_home_down = home_down;

        // Always update background hotkeys regardless of whether menu is visible
        gui->update_hotkeys();

        if (!menu_visible)
        {
            // Yield CPU completely to the game when GUI is hidden - eliminates 100% CPU spinning and system
            // lag
            ::Sleep(15);
            continue;
        }

        font->update();

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        gui->render();

        // Smooth and reliable custom window dragging (restricted to top Title Bar only)
        static bool is_dragging = false;
        static POINT drag_start_cursor{};
        static RECT drag_start_win{};

        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
        {
            if (!is_dragging)
            {
                ImVec2 mouse_pos = ImGui::GetIO().MousePos;
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered() &&
                    !ImGui::IsAnyItemActive() && mouse_pos.y >= 0.0f && mouse_pos.y <= SCALE(42.0f) &&
                    mouse_pos.x >= 0.0f && mouse_pos.x <= (SCALE(elements->window.size.x) - SCALE(145.0f)))
                {
                    is_dragging = true;
                    ::GetCursorPos(&drag_start_cursor);
                    ::GetWindowRect(hwnd, &drag_start_win);
                }
            }
            else
            {
                POINT current_cursor;
                ::GetCursorPos(&current_cursor);
                int dx = current_cursor.x - drag_start_cursor.x;
                int dy = current_cursor.y - drag_start_cursor.y;
                ::SetWindowPos(hwnd, nullptr, drag_start_win.left + dx, drag_start_win.top + dy, 0, 0,
                               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        else
        {
            is_dragging = false;
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                                                 clear_color.z * clear_color.w, clear_color.w};
        var->dx.ctx->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        var->dx.ctx->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = var->dx.swap_chain->Present(1, 0); // Present with vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    Strike_Shutdown();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
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
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2,
        D3D11_SDK_VERSION, &sd, &var->dx.swap_chain, &var->dx.device, &featureLevel, &var->dx.ctx);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
                                            featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &var->dx.swap_chain,
                                            &var->dx.device, &featureLevel, &var->dx.ctx);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (var->dx.swap_chain)
    {
        var->dx.swap_chain->Release();
        var->dx.swap_chain = nullptr;
    }
    if (var->dx.ctx)
    {
        var->dx.ctx->Release();
        var->dx.ctx = nullptr;
    }
    if (var->dx.device)
    {
        var->dx.device->Release();
        var->dx.device = nullptr;
    }
}

void CreateRenderTarget()
{
    ID3D11Texture2D *pBackBuffer;
    var->dx.swap_chain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    var->dx.device->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
