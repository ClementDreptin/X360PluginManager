#include "pch.h"
#include "Config.h"

#include "Console.h"

Config::Config()
    : m_File("game:\\config.ini")
{
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

    // Make sure the file format is correct
    if (!m_Structure.has("plugins_directory"))
    {
        g_Console.Error("Incorrect config file format. Expected to find a 'plugins_directory' section.");
        return E_FAIL;
    }
    if (!m_Structure["plugins_directory"].has("path"))
    {
        g_Console.Error("Incorrect config file format. Expected to find a 'path' key under the 'plugins_directory' section.");
        return E_FAIL;
    }
    if (m_Structure["plugins_directory"]["path"].empty())
    {
        g_Console.Error("Incorrect config file format. 'path' cannot be empty.");
        return E_FAIL;
    }

    m_PluginsDir = m_Structure["plugins_directory"]["path"];

    return S_OK;
}
