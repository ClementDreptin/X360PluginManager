#include "pch.h"

#include "Config.h"
#include "Console.h"

#define INDENT "    "
#define BLANKLINE ""

Config g_Config;
Console g_Console;

static HRESULT LoadPlugins()
{
    HRESULT hr = S_OK;

    const std::vector<std::string> &pluginsToLoad = g_Config.GetPluginsToLoad();
    if (pluginsToLoad.empty())
    {
        g_Console.Info("No plugins to load.");
        return hr;
    }

    g_Console.Info("Loading plugins...");

    for (size_t i = 0; i < pluginsToLoad.size(); i++)
    {
        const std::string &pluginPath = pluginsToLoad[i];

        // Check if the file exists on disk
        std::ifstream file(pluginPath);
        if (!file.good())
        {
            g_Console.Warn(INDENT + pluginPath + " doesn't exist, skipping.");
            continue;
        }

        std::string message = XexUtils::Formatter::Format(INDENT "Loaded %s.", pluginsToLoad[i].c_str());
        g_Console.Success(message);
    }

    return hr;
}

static HRESULT UnloadPlugins()
{
    HRESULT hr = S_OK;

    const std::vector<std::string> &pluginsToLoad = g_Config.GetPluginsToLoad();
    if (pluginsToLoad.empty())
    {
        g_Console.Info("No plugins to unload.");
        return hr;
    }

    g_Console.Info("Unloading plugins...");

    for (size_t i = 0; i < pluginsToLoad.size(); i++)
    {
        const std::string &pluginPath = pluginsToLoad[i];

        // Check if the plugin is already loaded
        HANDLE handle = GetModuleHandle(pluginPath.c_str());
        if (handle == nullptr)
        {
            g_Console.Warn(INDENT + pluginPath + " not loaded, skipping.");
            continue;
        }

        std::string message = XexUtils::Formatter::Format(INDENT "Loaded %s.", pluginsToLoad[i].c_str());
        g_Console.Success(message);
    }

    return hr;
}

static uint32_t WorkerThread(void *)
{
    HRESULT hr = XexUtils::Xam::MountHdd();
    if (FAILED(hr))
    {
        g_Console.Error("Couldn't mound HDD.");
        return 1;
    }

    g_Console.Info(BLANKLINE);

    hr = g_Config.LoadFromDisk();
    if (FAILED(hr))
        return 1;

    g_Console.Info(BLANKLINE);

    hr = UnloadPlugins();
    if (FAILED(hr))
        return 1;

    g_Console.Info(BLANKLINE);

    hr = LoadPlugins();
    if (FAILED(hr))
        return 1;

    g_Console.Info(BLANKLINE);
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
