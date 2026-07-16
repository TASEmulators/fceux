param(
    [string]$Configuration = "Release",
    [string]$Platform = "Win32"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

& (Join-Path $PSScriptRoot "win32-localization-check.ps1") `
    -ResourceFile (Join-Path $repoRoot "src/drivers/win/res.rc") `
    -ResourceHeader (Join-Path $repoRoot "src/drivers/win/resource.h")

$msbuild = (Get-Command MSBuild.exe -ErrorAction SilentlyContinue).Source
if (-not $msbuild) {
    $candidates = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
    )
    $msbuild = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}

if (-not $msbuild) {
    throw "MSBuild.exe was not found. Install Visual Studio 2022 C++ build tools with the v143 toolset."
}

Push-Location $repoRoot
try {
    & $msbuild "vc\vc14_fceux.sln" /m /t:Build /p:Configuration=$Configuration /p:Platform=$Platform /p:PlatformToolset=v143
} finally {
    Pop-Location
}
