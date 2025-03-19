#pragma once

#include "Console.h"

#include <mINI.h>

class Config
{
public:
    Config();

    HRESULT LoadFromDisk();

    inline const std::vector<std::string> &GetPluginsToLoad() { return m_PluginsToLoad; }

    inline const std::vector<std::string> &GetPluginsToUnload() { return m_PluginsToUnload; }

private:
    mINI::INIFile m_File;
    mINI::INIStructure m_Structure;

    std::vector<std::string> m_PluginsToLoad;
    std::vector<std::string> m_PluginsToUnload;
};

extern Config g_Config;
