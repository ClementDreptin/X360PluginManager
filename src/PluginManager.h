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
        std::string Path;
        bool Loaded;
        bool NewLoadState;
    };

    std::vector<Plugin> m_Plugins;

    HRESULT LoadPlugins();

    HRESULT UnloadPlugins();

    void ApplyChanges();

    HRESULT ReadPluginsDir(const std::string &pluginsDir);

    static HRESULT DoWork(PluginManager *This);

    HRESULT MountDrives();
};
