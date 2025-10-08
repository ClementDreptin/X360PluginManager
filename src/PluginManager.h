#pragma once

class PluginManager
{
public:
    HRESULT Init();

    void Render();

private:
    struct Plugin
    {
        std::string Name;
        Fs::Path Path;
        bool Loaded;
        bool NewLoadState;
    };

    std::vector<Plugin> m_Plugins;

    HRESULT LoadPlugins();

    HRESULT UnloadPlugins();

    void ApplyChanges();

    HRESULT ReadPluginsDir(const Fs::Path &pluginsDir);

    static HRESULT DoWork(PluginManager *This);

    HRESULT MountDrives();
};
