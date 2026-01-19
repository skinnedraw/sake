#define IMGUI_DEFINE_MATH_OPERATORS
#include <render/render.h>

#include <dwmapi.h>
#include <cstdio>
#include <chrono>
#include <thread>

#include <settings.h>
#include <features/esp/esp.h>
#include <keybind/keybind.h>
#include <Windows.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
    {
        return true;
    }

    switch (msg)
    {
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
        {
            return 0;
        }
        break;

    case WM_SYSKEYDOWN:
        if (wParam == VK_F4) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

render_t::render_t()
{
    detail = std::make_unique<detail_t>();
}

render_t::~render_t()
{
    destroy_imgui();
    destroy_window();
    destroy_device();
}

bool render_t::create_window()
{
    detail->window_class.cbSize = sizeof(detail->window_class);
    detail->window_class.style = CS_CLASSDC;
    detail->window_class.lpszClassName = "T4";
    detail->window_class.hInstance = GetModuleHandleA(0);
    detail->window_class.lpfnWndProc = wnd_proc;

    RegisterClassExA(&detail->window_class);

    detail->window = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        detail->window_class.lpszClassName,
        "T4",
        WS_POPUP,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        0,
        0,
        detail->window_class.hInstance,
        0
    );

    if (!detail->window)
    {
        return false;
    }

    SetLayeredWindowAttributes(detail->window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

    RECT client_area{};
    RECT window_area{};

    GetClientRect(detail->window, &client_area);
    GetWindowRect(detail->window, &window_area);

    POINT diff{};
    ClientToScreen(detail->window, &diff);

    MARGINS margins
    {
        window_area.left + (diff.x - window_area.left),
        window_area.top + (diff.y - window_area.top),
        window_area.right,
        window_area.bottom,
    };

    DwmExtendFrameIntoClientArea(detail->window, &margins);

    ShowWindow(detail->window, SW_SHOW);
    UpdateWindow(detail->window);

    return true;
}

bool render_t::create_device()
{
    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};

    swap_chain_desc.BufferCount = 1;

    swap_chain_desc.BufferDesc.Width = 0;
    swap_chain_desc.BufferDesc.Height = 0;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    swap_chain_desc.OutputWindow = detail->window;

    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swap_chain_desc.Windowed = 1;

    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    swap_chain_desc.SampleDesc.Count = 2;
    swap_chain_desc.SampleDesc.Quality = 0;

    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    D3D_FEATURE_LEVEL feature_level;
    D3D_FEATURE_LEVEL feature_level_list[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        feature_level_list,
        2,
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        &detail->swap_chain,
        &detail->device,
        &feature_level,
        &detail->device_context
    );

    if (result == DXGI_ERROR_UNSUPPORTED)
    {
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            feature_level_list,
            2,
            D3D11_SDK_VERSION,
            &swap_chain_desc,
            &detail->swap_chain,
            &detail->device,
            &feature_level,
            &detail->device_context
        );
    }

    if (result != S_OK)
    {
        MessageBoxA(nullptr, "This software can not run on your computer.", "Critical Problem", MB_ICONERROR | MB_OK);
    }

    ID3D11Texture2D* back_buffer{ nullptr };
    detail->swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));

    if (back_buffer)
    {
        detail->device->CreateRenderTargetView(back_buffer, nullptr, &detail->render_target_view);
        back_buffer->Release();

        return true;
    }

    return false;
}

bool render_t::create_imgui()
{
    using namespace ImGui;
    CreateContext();
    StyleColorsDark();

	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
	style.ScaleAllSizes(main_scale);
	style.FontScaleDpi = main_scale;
	
	io.Fonts->AddFontDefault();
	
	char windows_path[MAX_PATH];
	GetWindowsDirectoryA(windows_path, MAX_PATH);
	std::string tahoma_path = std::string(windows_path) + "\\Fonts\\tahoma.ttf";
	Visualize.font = io.Fonts->AddFontFromFileTTF(tahoma_path.c_str(), 13.0f);
	if (!Visualize.font)
	{
		Visualize.font = io.Fonts->AddFontDefault();
	}

    if (!ImGui_ImplWin32_Init(detail->window))
    {
        return false;
    }

    if (!detail->device || !detail->device_context)
    {
        return false;
    }

    if (!ImGui_ImplDX11_Init(detail->device, detail->device_context))
    {
        return false;
    }

    return true;
}

void render_t::destroy_device()
{
	if (detail->render_target_view) detail->render_target_view->Release();
	if (detail->swap_chain) detail->swap_chain->Release();
	if (detail->device_context) detail->device_context->Release();
	if (detail->device) detail->device->Release();
}

void render_t::destroy_window()
{
    DestroyWindow(detail->window);
    UnregisterClassA(detail->window_class.lpszClassName, detail->window_class.hInstance);
}

void render_t::destroy_imgui()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void render_t::start_render()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (GetAsyncKeyState(VK_INSERT) & 1)
    {
        running = !running;

        if (running)
        {
            SetWindowLong(detail->window, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT);
        }
        else
        {
            SetWindowLong(detail->window, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED);
        }
    }
}

void render_t::end_render()
{
    ImGui::Render();

    float clear_color[4]{ 0, 0, 0, 0 };
    detail->device_context->OMSetRenderTargets(1, &detail->render_target_view, nullptr);
    detail->device_context->ClearRenderTargetView(detail->render_target_view, clear_color);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    detail->swap_chain->Present(0, 0);
}

void render_t::render_menu()
{
    ImGui::SetNextWindowSize({ 500, 500 }, ImGuiCond_Always);
    ImGui::Begin("sake | developer 0.0", (bool*)0, ImGuiWindowFlags_NoResize);

    if (ImGui::BeginTabBar("MainTabs"))
    {
        if (ImGui::BeginTabItem("Aimbot"))
        {
            ImGui::Checkbox("Enable", &settings::aimbot::enabled);

            ImGui::SameLine();

            static bool aimbot_waiting_for_input = false;
            keybind aimbot_keybind_obj("aimbot");
            aimbot_keybind_obj.key = settings::aimbot::keybind;

            std::string aimbot_keybind_text = aimbot_keybind_obj.get_key_name();
            if (aimbot_waiting_for_input)
                aimbot_keybind_text = "[...]";

            ImGui::PushID("aimbot_keybind");
            if (ImGui::Button(aimbot_keybind_text.c_str(), ImVec2(120, 0)))
            {
                if (!aimbot_waiting_for_input)
                {
                    aimbot_waiting_for_input = true;
                }
            }

            if (ImGui::BeginPopupContextItem("aimbot_keybind_context", ImGuiPopupFlags_MouseButtonRight))
            {
                if (ImGui::Selectable("Hold", settings::aimbot::keybind_mode == 0))
                {
                    settings::aimbot::keybind_mode = 0;
                }
                if (ImGui::Selectable("Toggle", settings::aimbot::keybind_mode == 1))
                {
                    settings::aimbot::keybind_mode = 1;
                }
                if (ImGui::Selectable("Always", settings::aimbot::keybind_mode == 2))
                {
                    settings::aimbot::keybind_mode = 2;
                }
                if (ImGui::Selectable("Clear"))
                {
                    settings::aimbot::keybind = 0;
                }
                ImGui::EndPopup();
            }

            if (aimbot_waiting_for_input)
            {
                if (ImGui::GetIO().MouseClicked[0] && !ImGui::IsItemHovered())
                {
                    settings::aimbot::keybind = VK_LBUTTON;
                    aimbot_waiting_for_input = false;
                }
                else
                {
                    if (aimbot_keybind_obj.set_key())
                    {
                        settings::aimbot::keybind = aimbot_keybind_obj.key;
                        aimbot_waiting_for_input = false;
                    }
                }
            }

            ImGui::PopID();

            ImGui::Spacing();

            const char* aimbot_types[] = { "Camera", "Mouse" };
            ImGui::Combo("Type", &settings::aimbot::aim_type, aimbot_types, IM_ARRAYSIZE(aimbot_types));

            ImGui::Spacing();

            ImGui::Checkbox("Sticky Aim", &settings::aimbot::sticky_aim);

            ImGui::Spacing();

            ImGui::SliderFloat("FOV", &settings::aimbot::fov, 1.0f, 1000.0f, "%.1f");

            ImGui::Spacing();

            ImGui::Checkbox("Show FOV", &settings::aimbot::show_fov);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10.f);
            ImGui::ColorEdit4("## aimbot fov color", settings::aimbot::fov_color, ImGuiColorEditFlags_NoInputs);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Visuals"))
        {
            ImGui::Checkbox("Draw Box", &settings::visuals::box);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10.f);
            ImGui::ColorEdit4("## box color", settings::visuals::box_color, ImGuiColorEditFlags_NoInputs);

            ImGui::Checkbox("Draw Name", &settings::visuals::name);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10.f);
            ImGui::ColorEdit4("## name color", settings::visuals::name_color, ImGuiColorEditFlags_NoInputs);

            ImGui::Checkbox("Distance", &settings::visuals::distance);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10.f);
            ImGui::ColorEdit4("## distance color", settings::visuals::distance_color, ImGuiColorEditFlags_NoInputs);

            ImGui::Checkbox("Tool", &settings::visuals::tool);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10.f);
            ImGui::ColorEdit4("## tool color", settings::visuals::tool_color, ImGuiColorEditFlags_NoInputs);

            ImGui::Checkbox("Local Player", &settings::visuals::localplayer);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Exploits"))
        {
            ImGui::Checkbox("Walkspeed", &settings::walkspeed::enabled);
            
            ImGui::SameLine();
            
            static bool walkspeed_waiting_for_input = false;
            keybind walkspeed_keybind_obj("walkspeed");
            walkspeed_keybind_obj.key = settings::walkspeed::keybind;
            
            std::string walkspeed_keybind_text = walkspeed_keybind_obj.get_key_name();
            if (walkspeed_waiting_for_input)
                walkspeed_keybind_text = "[...]";
            
            ImGui::PushID("walkspeed_keybind");
            if (ImGui::Button(walkspeed_keybind_text.c_str(), ImVec2(120, 0)))
            {
                if (!walkspeed_waiting_for_input)
                {
                    walkspeed_waiting_for_input = true;
                }
            }
            
            if (ImGui::BeginPopupContextItem("walkspeed_keybind_context", ImGuiPopupFlags_MouseButtonRight))
            {
                if (ImGui::Selectable("Hold", settings::walkspeed::keybind_mode == 0))
                {
                    settings::walkspeed::keybind_mode = 0;
                }
                if (ImGui::Selectable("Toggle", settings::walkspeed::keybind_mode == 1))
                {
                    settings::walkspeed::keybind_mode = 1;
                }
                if (ImGui::Selectable("Always", settings::walkspeed::keybind_mode == 2))
                {
                    settings::walkspeed::keybind_mode = 2;
                }
                if (ImGui::Selectable("Clear"))
                {
                    settings::walkspeed::keybind = 0;
                }
                ImGui::EndPopup();
            }
            
            if (walkspeed_waiting_for_input)
            {
                if (ImGui::GetIO().MouseClicked[0] && !ImGui::IsItemHovered())
                {
                    settings::walkspeed::keybind = VK_LBUTTON;
                    walkspeed_waiting_for_input = false;
                }
                else
                {
                    if (walkspeed_keybind_obj.set_key())
                    {
                        settings::walkspeed::keybind = walkspeed_keybind_obj.key;
                        walkspeed_waiting_for_input = false;
                    }
                }
            }
            
            ImGui::PopID();
            
            if (settings::walkspeed::enabled)
            {
                ImGui::Spacing();
                
                ImGui::SliderFloat("Speed", &settings::walkspeed::speed, 1.0f, 1000.0f, "%.1f");
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Settings"))
        {
            if (ImGui::Checkbox("Hide Console", &settings::settings::hide_console))
            {
                HWND console_window = GetConsoleWindow();
                if (console_window)
                {
                    ShowWindow(console_window, settings::settings::hide_console ? SW_HIDE : SW_SHOW);
                }
            }

            ImGui::Spacing();

            if (ImGui::Button("Unload", ImVec2(200, 30)))
            {
                settings::settings::should_unload = true;
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void render_t::render_visuals()
{
    esp::run();

    if (settings::aimbot::show_fov)
    {
        HWND rblxWnd = FindWindowA(nullptr, "Roblox");
        if (rblxWnd)
        {
            ImVec2 center{};

            // Dynamic FOV center based on aimbot type
            if (settings::aimbot::aim_type == 0)  // Camera mode - center of screen
            {
                RECT windowRect;
                GetClientRect(rblxWnd, &windowRect);
                center = ImVec2(
                    static_cast<float>((windowRect.right - windowRect.left) / 2),
                    static_cast<float>((windowRect.bottom - windowRect.top) / 2)
                );
            }
            else if (settings::aimbot::aim_type == 1)  // Mouse mode - cursor position
            {
                POINT cursor_point;
                GetCursorPos(&cursor_point);
                ScreenToClient(rblxWnd, &cursor_point);
                center = ImVec2(static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y));
            }

            float radius = settings::aimbot::fov;

            ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(
                settings::aimbot::fov_color[0],
                settings::aimbot::fov_color[1],
                settings::aimbot::fov_color[2],
                settings::aimbot::fov_color[3]
            ));

            ImDrawList* draw = ImGui::GetBackgroundDrawList();
            draw->AddCircle(center, radius, color, 0, 2.0f);
        }
    }
}