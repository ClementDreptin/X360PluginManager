# X360PluginManager

Utility app to load and unload plugins from an Xbox 360.

<p align="center"><img src="./resources/screenshots/example.jpg" alt="X360PluginManager user interface" /></p>

> [!NOTE]
> This app is primarly for people using the [Bad Update exploit](https://github.com/grimdoomer/Xbox360BadUpdate) who can't load plugins via Dashlaunch (as of March 22nd 2025). If Dashlaunch becomes available for the [Bad Update exploit](https://github.com/grimdoomer/Xbox360BadUpdate), please use that instead.

## Installation

-   Download the latest build from the [releases](https://github.com/ClementDreptin/X360PluginManager/releases).
-   Unzip `X360PluginManager.zip` somewhere.
-   Modify `X360PluginManager\config.ini` to your needs.
-   Move the `X360PluginManager` directory to your console.
-   Launch `X360PluginManager\default.xex`.

## How to use `config.ini`?

The file has the following format:

```ini
[plugins_directory]
path = hdd:\path\to\plugins
```

Pretty self explanatory, `path` under `[plugins_directory]` is the directory where X360PluginManager will look for plugins to load or unload. This implies that all of your plugins are in the same directory. The search is **not** recursive, meaning you cannot put plugins inside of subdirectories.

Plugins can either be loaded from the hard drive using the `hdd:` device name, or from the first USB device using the `usb:` device name.

## Building

Clone the repository and the submodules:

```
git clone --recursive https://github.com/ClementDreptin/X360PluginManager.git
```

### Requirements

-   Having the Xbox 360 Software Development Kit (XDK) installed.

### Visual Studio 2010

Open `X360PluginManager.sln` in Visual Studio.

> [!NOTE]
> Using Visual Studio 2010 is the only way to run X360PluginManager in a debugger (sadly).

### Visual Studio 2022 (or any environment)

You can't build with the 64-bit version of MSBuild so you'll need to run the 32-bit version manually. Open PowerShell (which can be done in `View > Terminal` in Visual Studio) and run the following command:

```PS1
# Create an alias to the 32-bit version of MSBuild named msbuild
# The default installation path of VS2022 is C:\Program Files\Microsoft Visual Studio\2022\Community
Set-Alias msbuild "<path_vs2022>\MSBuild\Current\Bin\MSBuild.exe"
```

To debug X360PluginManager, you need to build and run the debugger from Visual Studio 2010.

To build X360PluginManager in release mode, deploy it to the console and launch it there, start a release build, either through the Visual Studio interface or with the following command:

```PS1
msbuild /p:Configuration=Release
```
