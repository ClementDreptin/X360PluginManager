#pragma once

#include <xtl.h>
#include <imgui.h>

bool ImGui_ImplDX9_Init(IDirect3DDevice9 *device);

void ImGui_ImplDX9_NewFrame();

void ImGui_ImplDX9_RenderDrawData(ImDrawData *draw_data);
