#include "pch.h"

#include "Config.h"
#include "Console.h"
#include "Modules.h"
#include "UI.h"

Config g_Config;
Console g_Console;

void __cdecl main()
{
    HRESULT hr = UI::Init();
    if (FAILED(hr))
        return;

    // Start a system thread to load the plugins, plugins are almost always system DLLs and those
    // can only be loaded from system threads
    HANDLE threadHandle = Memory::ThreadEx(
        reinterpret_cast<PTHREAD_START_ROUTINE>(Modules::DoWork),
        nullptr,
        static_cast<EXCREATETHREAD_FLAG>(EXCREATETHREAD_SYSTEM | EXCREATETHREAD_SUSPENDED)
    );
    XSetThreadProcessor(threadHandle, 4); // System threads can't be started on the same hardware thread as the title
    ResumeThread(threadHandle);

    // Main loop
    for (;;)
    {
        UI::BeginFrame();

        g_Console.Render();

        UI::EndFrame();
    }
}
