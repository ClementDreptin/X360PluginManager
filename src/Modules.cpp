#include "pch.h"
#include "Modules.h"

#include "Config.h"
#include "Console.h"

#define INDENT "    "
#define BLANKLINE ""

namespace Modules
{

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

HRESULT DoWork(void *)
{
    // Collisions are expected because, when running on a console with Dashlaunch, there is
    // already system symlinks named "hdd:" and "usb:". Collisions can also happen when running the app
    // multiple times because system symlinks continue to live after the app exits.

    HRESULT hr = S_OK;

    hr = Xam::MountHdd();
    if (FAILED(hr) && hr != STATUS_OBJECT_NAME_COLLISION)
    {
        g_Console.Error(Formatter::Format("Couldn't mound HDD. (%X)", hr));
        return hr;
    }

    hr = Xam::MountUsb();
    if (FAILED(hr) && hr != STATUS_OBJECT_NAME_COLLISION)
    {
        g_Console.Error(Formatter::Format("Couldn't mound USB. (%X)", hr));
        return hr;
    }

    hr = g_Config.LoadFromDisk();
    if (FAILED(hr))
        return hr;

    hr = UnloadPlugins();
    if (FAILED(hr))
        return hr;

    g_Console.Info(BLANKLINE);

    hr = LoadPlugins();
    if (FAILED(hr))
        return hr;

    g_Console.Info(BLANKLINE);
    g_Console.Info("All done! You can close X360PluginManager now.");

    return hr;
}

}
