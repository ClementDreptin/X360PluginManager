#include "pch.h"

#include "Console.h"

Console g_Console;

void __cdecl main()
{
    HRESULT hr = g_Console.Create();

    if (FAILED(hr))
        ATG::FatalError("Failed to create console: %x\n", hr);

    g_Console.Info("Info");
    g_Console.Success("Success");
    g_Console.Warn("Warn");
    g_Console.Error("Error");

    for (;;)
    {
        g_Console.Update();

        g_Console.Render();
    }
}
