#pragma once

#include <mINI.h>

class Config
{
public:
    Config();

    HRESULT LoadFromDisk();

    inline const std::string &GetPluginsDir() { return m_PluginsDir; }

private:
    mINI::INIFile m_File;
    mINI::INIStructure m_Structure;

    std::string m_PluginsDir;
};

extern Config g_Config;
