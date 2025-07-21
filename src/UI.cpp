#include "pch.h"
#include "UI.h"

#include "imgui_impl_dx9.h"
#include "imgui_impl_xbox360.h"

namespace UI
{

const float DisplayWidth = 1280.0f;
const float DisplayHeight = 720.0f;
const float SafeAreaPercentage = 90.0f;
const float SafeAreaWidth = DisplayWidth * SafeAreaPercentage / 100.0f;
const float SafeAreaHeight = DisplayHeight * SafeAreaPercentage / 100.0f;
const float SafeAreaOffsetX = (DisplayWidth - SafeAreaWidth) / 2.0f;
const float SafeAreaOffsetY = (DisplayHeight - SafeAreaHeight) / 2.0f;

static D3DDevice *s_pd3dDevice = nullptr;

// Ignore warning about "unused" pD3D variable
#pragma warning(push)
#pragma warning(disable : 4189)

static HRESULT InitD3D()
{
    // Instantiate D3D
    Direct3D *pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    XASSERT(pD3D != nullptr);

    // D3DDevice creation options
    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.BackBufferWidth = static_cast<uint32_t>(DisplayWidth);
    d3dpp.BackBufferHeight = static_cast<uint32_t>(DisplayHeight);
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    // Create device
    HRESULT hr = pD3D->CreateDevice(0, D3DDEVTYPE_HAL, nullptr, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &s_pd3dDevice);
    XASSERT(SUCCEEDED(hr));

    pD3D->Release();

    return hr;
}

#pragma warning(pop)

static void SetTheme()
{
    // Start from the base dark theme
    ImGui::StyleColorsDark();

    auto &colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.18f, 1.0f);
    colors[ImGuiCol_Border] = ImVec4(0.80f, 0.84f, 0.96f, 0.7f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.45f, 0.53f, 0.99f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.45f, 0.53f, 0.99f, 1.0f);
}

static HRESULT InitImGui()
{
    // Initialize ImGui and activate gamepad navigation
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup ImGui theme
    SetTheme();

    // Setup platform backend
    if (!ImGui_ImplXbox360_Init())
        return E_FAIL;

    // Setup renderer backend
    if (!ImGui_ImplDX9_Init(s_pd3dDevice))
        return E_FAIL;

    return S_OK;
}

HRESULT Init()
{
    HRESULT hr = S_OK;

    hr = InitD3D();
    if (FAILED(hr))
        return hr;

    hr = InitImGui();
    if (FAILED(hr))
        return hr;

    return hr;
}

void BeginFrame()
{
    // Setup the backend for a new frame
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplXbox360_NewFrame();
    ImGui::NewFrame();
}

void EndFrame()
{
    ImGui::EndFrame();

    // Clear the viewport
    s_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    s_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    s_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    s_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(30, 30, 46), 1.0f, 0);

    // Render ImGui
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    s_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
}

}
