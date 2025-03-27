#include "pch.h"

#include "Config.h"
#include "Console.h"

#define INDENT "    "
#define BLANKLINE ""

// Found here:
// https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-erref/596a1078-e883-4972-9bbc-49e60bebca55
#define NTSTATUS_OBJECT_NAME_COLLISION 0xC0000035

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

        // Check if the plugin is already loaded
        HANDLE handle = GetModuleHandle(pluginPath.c_str());
        if (handle != nullptr)
        {
            g_Console.Warn(Formatter::Format(INDENT "\"%s\" is already loaded, skipping.", pluginPath.c_str()));
            continue;
        }

        // Check if the file exists on disk
        std::ifstream file(pluginPath);
        if (!file.good())
        {
            g_Console.Warn(Formatter::Format(INDENT "\"%s\" doesn't exist, skipping.", pluginPath.c_str()));
            continue;
        }

        // Load the plugin
        hr = XexLoadImage(pluginPath.c_str(), XEX_LOADING_FLAG_DLL, 0, nullptr);
        if (FAILED(hr))
        {
            g_Console.Error(Formatter::Format(INDENT "Failed to load \"%s\" (%X).", pluginPath.c_str(), hr));
            continue;
        }

        g_Console.Success(Formatter::Format(INDENT "Loaded \"%s\".", pluginsToLoad[i].c_str()));
    }

    return hr;
}

static HRESULT UnloadPlugins()
{
    HRESULT hr = S_OK;

    const std::vector<std::string> &pluginsToUnload = g_Config.GetPluginsToUnload();
    if (pluginsToUnload.empty())
    {
        g_Console.Info("No plugins to unload.");
        return hr;
    }

    g_Console.Info("Unloading plugins...");

    for (size_t i = 0; i < pluginsToUnload.size(); i++)
    {
        const std::string &pluginPath = pluginsToUnload[i];

        // Check if the plugin is already loaded
        HANDLE handle = GetModuleHandle(pluginPath.c_str());
        if (handle == nullptr)
        {
            g_Console.Warn(Formatter::Format(INDENT "\"%s\" is not loaded, skipping.", pluginPath.c_str()));
            continue;
        }

        // The plugin load count is normally set to 0xFFFF (I don't know why) so we need to set it to 1
        // to make XexUnloadImage actually unmap the plugin
        LDR_DATA_TABLE_ENTRY *pDataTable = static_cast<LDR_DATA_TABLE_ENTRY *>(handle);
        pDataTable->LoadCount = 1;

        // Unload the plugin
        hr = XexUnloadImage(handle);
        if (FAILED(hr))
        {
            g_Console.Error(Formatter::Format(INDENT "Failed to unload \"%s\" (%X).", pluginPath.c_str(), hr));
            continue;
        }

        g_Console.Success(Formatter::Format(INDENT "Unloaded \"%s\".", pluginsToUnload[i].c_str()));
    }

    return hr;
}

static uint32_t WorkerThread(void *)
{
    // Collisions are expected because, when running on a console with Dashlaunch, there is
    // already system symlinks named "hdd:" and "usb:". Collisions can also happen when running the app
    // multiple times because system symlinks continue to live after the app exits.

    HRESULT hr = Xam::MountHdd();
    if (FAILED(hr) && hr != NTSTATUS_OBJECT_NAME_COLLISION)
    {
        g_Console.Error(Formatter::Format("Couldn't mound HDD. (%X)", hr));
        return 1;
    }

    hr = Xam::MountUsb();
    if (FAILED(hr) && hr != NTSTATUS_OBJECT_NAME_COLLISION)
    {
        g_Console.Error(Formatter::Format("Couldn't mound USB. (%X)", hr));
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
        ATG::FatalError("Failed to create console: %X\n", hr);

    // Start a system thread to load the plugins, plugins are almost always system DLLs and those
    // can only be loaded from system threads
    HANDLE threadHandle = Memory::ThreadEx(
        reinterpret_cast<PTHREAD_START_ROUTINE>(WorkerThread),
        nullptr,
        static_cast<EXCREATETHREAD_FLAG>(EXCREATETHREAD_SYSTEM | EXCREATETHREAD_SUSPENDED)
    );
    XSetThreadProcessor(threadHandle, 4); // System threads can't be started on the same hardware thread as the title
    ResumeThread(threadHandle);

    // Main update/render loop
    for (;;)
    {
        g_Console.Update();

        g_Console.Render();
    }
}
