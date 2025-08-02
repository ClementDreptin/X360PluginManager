#include "pch.h"
#include "PluginManager.h"

#include "Config.h"
#include "Console.h"
#include "UI.h"

HRESULT PluginManager::Init()
{
    HRESULT hr = S_OK;

    hr = MountDrives();
    if (FAILED(hr))
        return hr;

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
    // Place the window on the right half of the screen
    const ImVec2 &padding = ImGui::GetStyle().WindowPadding;
    ImVec2 windowPos(UI::SafeAreaOffsetX, UI::SafeAreaOffsetY);
    ImVec2 windowSize(UI::SafeAreaWidth / 2.0f - padding.x / 2.0f, UI::SafeAreaHeight);
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);

    // Create the window
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize;
    ImGui::Begin("Plugins", nullptr, windowFlags);

    if (!m_Plugins.empty())
    {
        // Render a checkbox for each plugin
        for (size_t i = 0; i < m_Plugins.size(); i++)
            ImGui::Checkbox(m_Plugins[i].Name.c_str(), &m_Plugins[i].NewLoadState);

        ImGui::NewLine();

        // Apply changes button
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 5.0f));
        if (ImGui::Button("Apply changes"))
            ApplyChanges();
        ImGui::PopStyleVar();
    }
    else
        ImGui::TextUnformatted("No plugins found.");

    ImGui::End();
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

        // Load the plugin
        hr = XexLoadImage(plugin.Path.c_str(), XEX_LOADING_FLAG_DLL, 0, nullptr);
        if (FAILED(hr))
        {
            g_Console.Error(Formatter::Format("Failed to load \"%s\" (%X).", plugin.Name.c_str(), hr));
            plugin.NewLoadState = false;
            continue;
        }

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

        // If the plugin was unloaded via an external process between when the app
        // started and now, warn the user and skip
        HANDLE handle = GetModuleHandle(plugin.Name.c_str());
        if (handle == nullptr)
        {
            g_Console.Warn(Formatter::Format("\"%s\" was already unloaded.", plugin.Name.c_str()));
            continue;
        }

        // The plugin load count is normally set to 0xFFFF (I don't know why) so we need
        // to set it to 1 to make XexUnloadImage actually unmap the plugin
        LDR_DATA_TABLE_ENTRY *pDataTable = static_cast<LDR_DATA_TABLE_ENTRY *>(handle);
        pDataTable->LoadCount = 1;

        // Unload the plugin
        hr = XexUnloadImage(handle);
        if (FAILED(hr))
        {
            g_Console.Error(Formatter::Format("Failed to unload \"%s\" (%X).", plugin.Name.c_str(), hr));
            plugin.NewLoadState = true;
            continue;
        }

        plugin.Loaded = false;
        g_Console.Success(Formatter::Format("Unloaded \"%s\".", plugin.Name.c_str()));
    }

    return hr;
}

void PluginManager::ApplyChanges()
{
    // Start a system thread to load the plugins, plugins are almost always system DLLs and those
    // can only be loaded from system threads
    HANDLE threadHandle = Memory::ThreadEx(
        reinterpret_cast<PTHREAD_START_ROUTINE>(DoWork),
        this,
        static_cast<EXCREATETHREAD_FLAG>(EXCREATETHREAD_SYSTEM | EXCREATETHREAD_SUSPENDED)
    );
    XSetThreadProcessor(threadHandle, 4); // System threads can't be started on the same hardware thread as the title
    ResumeThread(threadHandle);
}

HRESULT PluginManager::ReadPluginsDir(const std::string &pluginsDir)
{
    // Create the search pattern
    const std::string &pluginsDirWithFinalSeparator = pluginsDir.back() == '\\' ? pluginsDir : pluginsDir + '\\';
    std::string searchPattern = pluginsDirWithFinalSeparator + "*.*";

    // Find the first file corresponding to the pattern
    WIN32_FIND_DATA fileInfo = {};
    HANDLE fileHandle = FindFirstFile(searchPattern.c_str(), &fileInfo);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        g_Console.Error(Formatter::Format("Couldn't find the first file in \"%s\".", pluginsDir.c_str()));
        return E_FAIL;
    }

    // Loop as long as there are files in the directory
    do
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
        if (strcmp(extension, ".xex") == 0)
        {
            Plugin plugin = {};
            plugin.Name = fileInfo.cFileName;
            plugin.Path = pluginsDirWithFinalSeparator + fileInfo.cFileName;
            plugin.Loaded = GetModuleHandle(fileInfo.cFileName) != nullptr;
            plugin.NewLoadState = plugin.Loaded;
            m_Plugins.emplace_back(plugin);
        }
    } while (FindNextFile(fileHandle, &fileInfo));

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

HRESULT PluginManager::DoWork(PluginManager *This)
{
    HRESULT hr = S_OK;

    hr = This->UnloadPlugins();
    if (FAILED(hr))
        return hr;

    hr = This->LoadPlugins();
    if (FAILED(hr))
        return hr;

    g_Console.Info("");

    return hr;
}

HRESULT PluginManager::MountDrives()
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

    return hr;
}
