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
        bool Loaded;
        bool NewLoadState;
    };

    std::vector<Plugin> m_Plugins;

    HRESULT LoadPlugins();

    HRESULT UnloadPlugins();

    HRESULT ReadPluginsDir(const std::string &pluginsDir);

    HRESULT DoWork(void *);
};
