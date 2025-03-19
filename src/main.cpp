#include "pch.h"

#include "Console.h"

Console g_Console;

void __cdecl main()
{
    HRESULT hr = g_Console.Create();

    if (FAILED(hr))
        ATG::FatalError("Failed to create console: %x\n", hr);

    g_Console.Print("Hello, World!");

    for (;;)
    {
        g_Console.Update();

        g_Console.Render();
    }
}
