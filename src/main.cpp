#include "pch.h"

#include "Config.h"
#include "Console.h"

Config g_Config;
Console g_Console;

uint32_t WorkerThread(void *)
{
    HRESULT hr = g_Config.LoadFromDisk();
    if (FAILED(hr))
        return 1;

    const std::vector<std::string> &pluginsToUnload = g_Config.GetPluginsToUnload();
    const std::vector<std::string> &pluginsToLoad = g_Config.GetPluginsToLoad();

    if (!pluginsToUnload.empty())
    {
        for (size_t i = 0; i < pluginsToUnload.size(); i++)
        {
            std::string message = XexUtils::Formatter::Format("Unloaded %s.", pluginsToUnload[i].c_str());
            g_Console.Success(message);
        }
    }
    else
        g_Console.Info("No plugins to unload.");

    if (!pluginsToLoad.empty())
    {
        for (size_t i = 0; i < pluginsToLoad.size(); i++)
        {
            std::string message = XexUtils::Formatter::Format("Loaded %s.", pluginsToLoad[i].c_str());
            g_Console.Success(message);
        }
    }
    else
        g_Console.Info("No plugins to load.");

    g_Console.Info("");
    g_Console.Info("All done! You can close X360PluginManager now.");

    return 0;
}

void __cdecl main()
{
    HRESULT hr = g_Console.Create();
    if (FAILED(hr))
        ATG::FatalError("Failed to create console: %x\n", hr);

    // Start a system thread to load the plugins, plugins are almost always system DLLs and those
    // can only be loaded from system threads
    // TODO: Use XexUtils once the threading API is more flexible
    HANDLE hThread = nullptr;
    ExCreateThread(
        &hThread,
        0,
        nullptr,
        nullptr,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(WorkerThread),
        nullptr,
        static_cast<EXCREATETHREAD_FLAG>(EXCREATETHREAD_SYSTEM | EXCREATETHREAD_SUSPENDED)
    );
    XSetThreadProcessor(hThread, 4); // System threads can't be started on the same hardware thread as the title
    ResumeThread(hThread);

    for (;;)
    {
        g_Console.Update();

        g_Console.Render();
    }
}
