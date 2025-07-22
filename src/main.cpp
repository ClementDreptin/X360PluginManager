#include "pch.h"

#include "Config.h"
#include "Console.h"
#include "PluginManager.h"
#include "UI.h"

Config g_Config;
Console g_Console;
PluginManager g_PluginManager;

void __cdecl main()
{
    HRESULT hr = UI::Init();
    if (FAILED(hr))
        return;

    // We intentionally continue even if the initialization failed because we need
    // to show the potential error messages in the UI
    g_PluginManager.Init();

    // Main loop
    for (;;)
    {
        UI::BeginFrame();

        g_PluginManager.Render();
        g_Console.Render();

        UI::EndFrame();
    }
}
