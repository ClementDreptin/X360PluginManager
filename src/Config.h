#pragma once

#include <mINI.h>

class Config
{
public:
    Config();

    HRESULT LoadFromDisk();

    inline const Fs::Path &GetPluginsDir() { return m_PluginsDir; }

private:
    mINI::INIFile m_File;
    mINI::INIStructure m_Structure;

    Fs::Path m_PluginsDir;
};

extern Config g_Config;
