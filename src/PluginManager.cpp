#include "pch.h"
#include "PluginManager.h"

#include "Config.h"
#include "Console.h"
#include "UI.h"

HRESULT PluginManager::Init()
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

    hr = ReadPluginsDir(g_Config.GetPluginsDir());
    if (FAILED(hr))
        return hr;

    return hr;
}

void PluginManager::Render()
{
    const ImVec2 &padding = ImGui::GetStyle().WindowPadding;
    ImVec2 windowPos(UI::SafeAreaOffsetX, UI::SafeAreaOffsetY);
    ImVec2 windowSize(UI::SafeAreaWidth, UI::SafeAreaHeight / 2.0f - padding.y / 2.0f);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize;
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::Begin("Plugins", nullptr, windowFlags);

    if (!m_Plugins.empty())
    {
        for (size_t i = 0; i < m_Plugins.size(); i++)
            ImGui::Checkbox(m_Plugins[i].Name.c_str(), &m_Plugins[i].NewLoadState);

        if (ImGui::Button("Apply"))
        {
            // TODO: start this in a system thread
            DoWork(nullptr);
        }
    }
    else
        ImGui::TextUnformatted("No plugins found.");

    ImGui::End();
    ImGui::PopStyleVar();
}

HRESULT PluginManager::LoadPlugins()
{
    HRESULT hr = S_OK;

    for (size_t i = 0; i < m_Plugins.size(); i++)
    {
        // If the plugin is still in the same state, skip it
        Plugin &plugin = m_Plugins[i];
        if (plugin.Loaded == plugin.NewLoadState)
            continue;

        // If the plugin doesn't need to be loaded, skip it
        if (plugin.NewLoadState == false)
            continue;

        // TODO: actually load the plugin
        plugin.Loaded = true;
        g_Console.Success(Formatter::Format("Loaded \"%s\".", plugin.Name.c_str()));
    }

    return hr;
}

HRESULT PluginManager::UnloadPlugins()
{
    HRESULT hr = S_OK;

    for (size_t i = 0; i < m_Plugins.size(); i++)
    {
        // If the plugin is still in the same state, skip it
        Plugin &plugin = m_Plugins[i];
        if (plugin.Loaded == plugin.NewLoadState)
            continue;

        // If the plugin doesn't need to be unloaded, skip it
        if (plugin.NewLoadState == true)
            continue;

        // TODO: actually unload the plugin
        plugin.Loaded = false;
        g_Console.Success(Formatter::Format("Unloaded \"%s\".", plugin.Name.c_str()));
    }

    return hr;
}

HRESULT PluginManager::ReadPluginsDir(const std::string &pluginsDir)
{
    std::string pluginExtension = ".xex";

    // Create the search pattern
    std::string searchPattern = pluginsDir;
    if (searchPattern.back() != '\\')
        searchPattern += '\\';
    searchPattern += "*.*";

    // Find the first file corresponding to the pattern
    WIN32_FIND_DATA fileInfo = {};
    HANDLE fileHandle = FindFirstFile(searchPattern.c_str(), &fileInfo);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        g_Console.Error(Formatter::Format("Couldn't find the first file in \"%s\".", pluginsDir.c_str()));
        return E_FAIL;
    }

    // Loop as long as there are files in the directory
    while (FindNextFile(fileHandle, &fileInfo))
    {
        // Extract the extension from the file name
        char extension[_MAX_EXT] = {};
        _splitpath_s(
            fileInfo.cFileName,
            nullptr, 0,
            nullptr, 0,
            nullptr, 0,
            extension, sizeof(extension)
        );

        // If the current file is a plugin, add its info to the list
        if (extension == pluginExtension)
        {
            Plugin plugin = {};
            plugin.Name = fileInfo.cFileName;
            plugin.Loaded = GetModuleHandle(fileInfo.cFileName) != nullptr;
            plugin.NewLoadState = plugin.Loaded;
            m_Plugins.emplace_back(plugin);
        }
    }

    FindClose(fileHandle);

    // Check for errors
    uint32_t error = GetLastError();
    if (error != ERROR_NO_MORE_FILES)
    {
        g_Console.Error(Formatter::Format("Couldn't finish reading the plugins directory (%X).", error));
        return E_FAIL;
    }

    return S_OK;
}

HRESULT PluginManager::DoWork(void *)
{
    HRESULT hr = S_OK;

    hr = UnloadPlugins();
    if (FAILED(hr))
        return hr;

    hr = LoadPlugins();
    if (FAILED(hr))
        return hr;

    g_Console.Info("");

    return hr;
}
