#include "pch.h"
#include "Config.h"

#define MAX_PLUGIN_COUNT 5

Config::Config()
    : m_File("game:\\config.ini")
{
    // Pre-allocate enough space for the max amount of plugins to load and/or unload
    m_PluginsToLoad.reserve(MAX_PLUGIN_COUNT);
    m_PluginsToUnload.reserve(MAX_PLUGIN_COUNT);
}

HRESULT Config::LoadFromDisk()
{
    // Load the config from disk
    bool canReadFile = m_File.read(m_Structure);
    if (!canReadFile)
    {
        g_Console.Error("Could not read config from disk.");
        return E_FAIL;
    }

    // Plugins to load
    if (m_Structure.has("load"))
    {
        for (size_t i = 0; i < MAX_PLUGIN_COUNT; i++)
        {
            std::string key = Formatter::Format("plugin%d", i + 1);
            if (m_Structure["load"].has(key) && !m_Structure["load"][key].empty())
                m_PluginsToLoad.push_back(m_Structure["load"][key]);
        }
    }

    // Plugins to unload
    if (m_Structure.has("unload"))
    {
        for (size_t i = 0; i < MAX_PLUGIN_COUNT; i++)
        {
            std::string key = Formatter::Format("plugin%d", i + 1);
            if (m_Structure["unload"].has(key) && !m_Structure["unload"][key].empty())
                m_PluginsToUnload.push_back(m_Structure["unload"][key]);
        }
    }

    return S_OK;
}
