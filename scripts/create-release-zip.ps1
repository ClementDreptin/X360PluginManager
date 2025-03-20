$ErrorActionPreference = "Stop"

$RootDir = "$PSScriptRoot\.."
$BuildDir = "$RootDir\build\Release\bin"
$DefaultXexPath = "$BuildDir\default.xex"

function Build {
    # Path to the 32-bit version of MSBuild
    $MsbuildExePath = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

    # Build the app
    & $MsbuildExePath "$RootDir\X360PluginManager.sln" /p:Configuration=Release
    if (-not $?) {
        throw "Couldn't build."
    }
}

function PatchXexForRetail {
    # You need to have XEX tool in your PATH
    XexTool -m r $DefaultXexPath
}

function CreateZip {
    $OutputZip = "$RootDir\X360PluginManager.zip"
    $TmpDir = "$([System.IO.Path]::GetTempPath())\X360PluginManager"

    # Create a temporary directory which has the same structure as the output zip
    New-Item -ItemType Directory -Path $TmpDir -Force

    # Copy the appropriate files into the temporary directory
    Copy-Item -Path "$BuildDir\Media" -Destination $TmpDir -Recurse
    Copy-Item -Path $DefaultXexPath -Destination $TmpDir
    Copy-Item -Path "$RootDir\src\config.ini" -Destination $TmpDir

    # Create the zip
    Compress-Archive -Path "$TmpDir\*" -DestinationPath $OutputZip -Force

    # Remove the temporary directory
    Remove-Item -Path $TmpDir -Recurse -Force
}

Build

PatchXexForRetail

CreateZip
