#Requires -Version 5.1
<#
.SYNOPSIS
Downloads Qt GUI dependencies and generates an x64 Visual Studio 2022 solution.
.DESCRIPTION
Requires Windows x64, Git, and VS2022 Desktop development with C++ (including
the Windows SDK and C++ CMake tools). No administrator access is required.
Downloads are cached; rerunning preserves build trees unless -Clean is specified.
-Clean deletes the repository build folder and recreates the default workspace.
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [string]$BuildDirectory,
    [string]$DependencyDirectory,
    [string]$QtRoot,
    [string]$VulkanSdk,
    [switch]$SkipVulkan,
    [switch]$Clean,
    [switch]$EnableRetroAchievements = $true
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$setupTimer = [Diagnostics.Stopwatch]::StartNew()
trap {
    Write-Host "`nVS2022 Qt workspace setup FAILED after $($setupTimer.Elapsed.ToString('hh\:mm\:ss'))." -ForegroundColor Red
    Write-Host "Reason: $($_.Exception.Message)"
    Write-Host 'Setup did not complete. Completed downloads/installations are retained; fix the error and rerun.'
    throw
}

function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "'$Program $($Arguments -join ' ')' failed (exit code $LASTEXITCODE)."
    }
}

function Get-Download {
    param([string]$Url, [string]$Name)
    $destination = Join-Path $downloads $Name
    if (!(Test-Path -LiteralPath $destination -PathType Leaf)) {
        Write-Host "Downloading $Name"
        Invoke-Checked 'curl.exe' @('--fail', '--location', '--retry', '3',
            '--output', "$destination.partial", $Url)
        Move-Item -LiteralPath "$destination.partial" -Destination $destination -Force
    }
    return $destination
}

function Assert-Files {
    param([string]$Root, [string[]]$Files)
    foreach ($file in $Files) {
        if (!(Test-Path -LiteralPath (Join-Path $Root $file) -PathType Leaf)) {
            throw "Dependency is incomplete: missing '$Root\$file'. Remove or repair that dependency and rerun."
        }
    }
}

if ($env:OS -ne 'Windows_NT' -or ![Environment]::Is64BitOperatingSystem) {
    throw 'This script requires 64-bit Windows.'
}
if ($SkipVulkan -and $VulkanSdk) {
    throw '-SkipVulkan and -VulkanSdk cannot be used together.'
}
if (!$SkipVulkan -and !(Test-Path -LiteralPath "$env:SystemRoot\System32\vulkan-1.dll")) {
    throw 'Install a graphics driver with the Vulkan runtime, or use -SkipVulkan. The private SDK install does not install system graphics drivers.'
}
$source = [IO.Path]::GetFullPath("$PSScriptRoot\..")
if ($Clean -and ($BuildDirectory -or $DependencyDirectory -or $QtRoot -or $VulkanSdk)) {
    throw '-Clean recreates the default workspace and cannot be combined with -BuildDirectory, -DependencyDirectory, -QtRoot, or -VulkanSdk.'
}
if (!$BuildDirectory) {
    $BuildDirectory = Join-Path $source 'build\qt-vs2022'
}
if (!$DependencyDirectory) {
    $DependencyDirectory = Join-Path $source 'build\qt-deps'
}
$BuildDirectory = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BuildDirectory)
$DependencyDirectory = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($DependencyDirectory)
if ($BuildDirectory -eq $source -or $DependencyDirectory -eq $source -or
    $BuildDirectory -eq $DependencyDirectory) {
    throw 'Source, build, and dependency directories must be distinct; in-source builds are not supported.'
}

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (!(Test-Path -LiteralPath $vswhere)) {
    throw 'Install Visual Studio 2022 with the Desktop development with C++ workload first.'
}
$vs = & $vswhere -latest -version '[17.0,18.0)' -products '*' `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
if ($LASTEXITCODE -ne 0 -or !$vs) {
    throw 'VS2022 C++ tools were not found. Install the Desktop development with C++ workload and a Windows SDK.'
}
$cmake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
if (!(Test-Path -LiteralPath $cmake)) {
    $cmakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if (!$cmakeCommand) {
        throw 'Install the C++ CMake tools for Windows component in the Visual Studio Installer.'
    }
    $cmake = $cmakeCommand.Source
}
$git = Get-Command git.exe -ErrorAction SilentlyContinue
if (!$git) {
    throw 'Install Git for Windows and make git.exe available on PATH, then rerun.'
}
if (!(Get-Command curl.exe -ErrorAction SilentlyContinue)) {
    throw 'curl.exe is required (included with Windows 10/11).'
}

# Keep versions aligned with the Windows Qt CI build.
$qtVersion = '6.9.3'
$sdlVersion = '2.32.10'
$ffmpegVersion = '5.1.2'
$vulkanVersion = '1.4.363.0'
$aqtVersion = '3.3.0'
$aqtHash = '4f74d4c95c464d238d7e17ec2d9b7f22a7c333f0f5270a62584e2b47fc765150'
$registry = Get-Content -LiteralPath "$source\vcpkg\vcpkg-configuration.json" -Raw | ConvertFrom-Json
$baseline = $registry.'default-registry'.baseline
$downloads = Join-Path $DependencyDirectory 'downloads'

Write-Host "`nFCEUX Qt GUI - VS2022 x64 workspace setup"
Write-Host "Visual Studio (already installed): $vs"
Write-Host "Dependencies: $DependencyDirectory"
Write-Host "Download cache: $downloads"
Write-Host "Solution to generate: $BuildDirectory\fceux.sln"
if ($Clean) {
    Write-Host "CLEAN START: the entire '$source\build' folder will be deleted, including all downloads, builds, and user files." -ForegroundColor Yellow
    Write-Host 'Close Visual Studio and any programs running from that folder before continuing.'
}
Write-Host "`nDependencies to install or reuse:"
if ($QtRoot) {
    Write-Host "  Qt (use existing): $($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($QtRoot))"
}
else {
    Write-Host "  Qt $qtVersion with Multimedia: $DependencyDirectory\Qt\$qtVersion\msvc2022_64"
    Write-Host "  aqt installer ${aqtVersion}: $downloads"
}
Write-Host "  SDL2 $sdlVersion`: $DependencyDirectory\SDL2-$sdlVersion"
Write-Host "  FFmpeg $ffmpegVersion`: $DependencyDirectory\ffmpeg-$ffmpegVersion-full_build-shared"
Write-Host "  vcpkg ($baseline): $DependencyDirectory\vcpkg-$baseline"
Write-Host "  libarchive and its dependencies: $DependencyDirectory\installed\x64-windows-rel"
if ($SkipVulkan) {
    Write-Host '  Vulkan: disabled'
}
elseif ($VulkanSdk) {
    Write-Host "  Vulkan SDK (use existing): $($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($VulkanSdk))"
}
else {
    Write-Host "  Vulkan SDK $vulkanVersion`: $DependencyDirectory\VulkanSDK\$vulkanVersion"
}
if ($EnableRetroAchievements) {
    Write-Host "  rcheevos v12.5.0 (RetroAchievements): $DependencyDirectory\rcheevos-v12.5.0"
}
else {
    Write-Host '  RetroAchievements: disabled'
}
if (!$Clean) {
    Write-Host "`nExisting dependencies/downloads will be reused."
}
Write-Host 'No system-wide install or permanent PATH changes.'
Write-Host 'Dependency tools may use temporary files and per-user caches outside these directories.'
Write-Host 'This generates the workspace; build the emulator in Visual Studio afterward.'
Write-Host 'The Vulkan SDK installer accepts its license terms noninteractively when installation is needed.'
Write-Host ''
if ($Clean) {
    $cleanDirectory = Join-Path $source 'build'
    if (Test-Path -LiteralPath $cleanDirectory) {
        $cleanItem = Get-Item -LiteralPath $cleanDirectory -Force
        if (!$cleanItem.PSIsContainer -or ($cleanItem.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "Refusing to clean '$cleanDirectory': expected a regular directory, not a file or link."
        }
        $links = Get-ChildItem -LiteralPath $cleanDirectory -Force -Recurse |
            Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint } |
            Select-Object -First 1
        if ($links) {
            throw "Refusing to clean '$cleanDirectory': it contains a link at '$($links.FullName)'. Remove the link manually and rerun."
        }
    }
    if (!$PSCmdlet.ShouldProcess($cleanDirectory, 'Delete the entire build folder and recreate the default Qt VS2022 workspace')) {
        Write-Host 'Clean setup was not performed; no files were changed.'
        return
    }
    if (Test-Path -LiteralPath $cleanDirectory) {
        Remove-Item -LiteralPath $cleanDirectory -Recurse -Force
    }
    Write-Host "Clean start: removed any previous build folder at '$cleanDirectory'."
}
elseif ($WhatIfPreference) {
    Write-Host 'Setup preview only; no files were changed.'
    return
}
New-Item -ItemType Directory -Force $downloads, $BuildDirectory | Out-Null

if (!$QtRoot) {
    $QtRoot = Join-Path $DependencyDirectory "Qt\$qtVersion\msvc2022_64"
    $qtMarker = Join-Path $QtRoot '.fceux-installed'
    if (!(Test-Path -LiteralPath $qtMarker)) {
        $aqt = Get-Download "https://github.com/miurahr/aqtinstall/releases/download/v$aqtVersion/aqt_x64.exe" "aqt-$aqtVersion.exe"
        if ((Get-FileHash -LiteralPath $aqt -Algorithm SHA256).Hash -ne $aqtHash) {
            throw "SHA256 mismatch for '$aqt'. Delete the cached download and retry."
        }
        Push-Location $DependencyDirectory
        try {
            Invoke-Checked $aqt @('install-qt', 'windows', 'desktop', $qtVersion,
                'win64_msvc2022_64', '--outputdir', "$DependencyDirectory\Qt",
                '--modules', 'qtmultimedia')
        }
        finally {
            Pop-Location
        }
        Assert-Files $QtRoot @('bin\windeployqt.exe', 'bin\Qt6Cored.dll',
            'lib\cmake\Qt6Multimedia\Qt6MultimediaConfig.cmake')
        New-Item -ItemType File -Path $qtMarker -Force | Out-Null
    }
}
$QtRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($QtRoot)
Assert-Files $QtRoot @('bin\windeployqt.exe', 'bin\Qt6Core.dll', 'bin\Qt6Cored.dll',
    'lib\cmake\Qt6\Qt6Config.cmake', 'lib\cmake\Qt6Qml\Qt6QmlConfig.cmake',
    'lib\cmake\Qt6UiTools\Qt6UiToolsConfig.cmake',
    'lib\cmake\Qt6Multimedia\Qt6MultimediaConfig.cmake')

$sdl = Join-Path $DependencyDirectory "SDL2-$sdlVersion"
if (!(Test-Path -LiteralPath "$sdl\lib\x64\SDL2.lib")) {
    $zip = Get-Download "https://github.com/libsdl-org/SDL/releases/download/release-$sdlVersion/SDL2-devel-$sdlVersion-VC.zip" "SDL2-$sdlVersion.zip"
    Expand-Archive -LiteralPath $zip -DestinationPath $DependencyDirectory -Force
}
Assert-Files $sdl @('include\SDL.h', 'lib\x64\SDL2.lib', 'lib\x64\SDL2.dll')

$ffmpeg = Join-Path $DependencyDirectory "ffmpeg-$ffmpegVersion-full_build-shared"
if (!(Test-Path -LiteralPath "$ffmpeg\lib\avcodec.lib") -and
    !(Test-Path -LiteralPath "$ffmpeg\ffmpeg-$ffmpegVersion-full_build-shared\lib\avcodec.lib")) {
    $zip = Get-Download "https://github.com/GyanD/codexffmpeg/releases/download/$ffmpegVersion/ffmpeg-$ffmpegVersion-full_build-shared.zip" "ffmpeg-$ffmpegVersion.zip"
    Expand-Archive -LiteralPath $zip -DestinationPath $DependencyDirectory -Force
}
# Some FFmpeg archives contain a second directory with the same name.
if (Test-Path -LiteralPath "$ffmpeg\ffmpeg-$ffmpegVersion-full_build-shared\lib\avcodec.lib") {
    $ffmpeg = Join-Path $ffmpeg "ffmpeg-$ffmpegVersion-full_build-shared"
}
Assert-Files $ffmpeg @('include\libavcodec\avcodec.h', 'lib\avcodec.lib',
    'lib\avformat.lib', 'lib\avutil.lib', 'lib\swscale.lib', 'lib\swresample.lib',
    'bin\avcodec-59.dll')

$vcpkgRoot = Join-Path $DependencyDirectory "vcpkg-$baseline"
if (!(Test-Path -LiteralPath "$vcpkgRoot\.git")) {
    Invoke-Checked $git.Source @('clone', '--filter=blob:none', '--no-checkout',
        'https://github.com/microsoft/vcpkg.git', $vcpkgRoot)
}
Invoke-Checked $git.Source @('-C', $vcpkgRoot, 'checkout', '--detach', $baseline)
$vcpkg = Join-Path $vcpkgRoot 'vcpkg.exe'
if (!(Test-Path -LiteralPath $vcpkg)) {
    Invoke-Checked "$vcpkgRoot\bootstrap-vcpkg.bat" @('-disableMetrics')
}
$archiveRoot = Join-Path $DependencyDirectory 'installed\x64-windows-rel'
$oldVsPath = $env:VCPKG_VISUAL_STUDIO_PATH
try {
    $env:VCPKG_VISUAL_STUDIO_PATH = $vs
    Invoke-Checked $vcpkg @('install', "--x-manifest-root=$source\vcpkg",
        "--x-install-root=$DependencyDirectory\installed", '--triplet=x64-windows-rel',
        "--overlay-triplets=$source\vcpkg\custom-triplets")
}
finally {
    $env:VCPKG_VISUAL_STUDIO_PATH = $oldVsPath
}
Assert-Files $archiveRoot @('include\archive.h', 'lib\archive.lib', 'bin\archive.dll')

if (!$SkipVulkan) {
    if (!$VulkanSdk) {
        $VulkanSdk = Join-Path $DependencyDirectory "VulkanSDK\$vulkanVersion"
        if (!(Test-Path -LiteralPath "$VulkanSdk\Include\vulkan\vulkan.h") -or
            !(Test-Path -LiteralPath "$VulkanSdk\Lib\vulkan-1.lib")) {
            $installer = Get-Download "https://sdk.lunarg.com/sdk/download/$vulkanVersion/windows/vulkansdk-windows-X64-$vulkanVersion.exe" "vulkansdk-$vulkanVersion.exe"
            Invoke-Checked $installer @('--root', $VulkanSdk, '--accept-licenses',
                '--default-answer', '--confirm-command', 'install', 'copy_only=1')
        }
    }
    $VulkanSdk = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($VulkanSdk)
    Assert-Files $VulkanSdk @('Include\vulkan\vulkan.h', 'Lib\vulkan-1.lib')
}

$configure = @('-S', $source, '-B', $BuildDirectory,
    '-G', 'Visual Studio 17 2022', '-A', 'x64', '-T', 'v143',
    "-DCMAKE_GENERATOR_INSTANCE:PATH=$vs",
    "-DCMAKE_PREFIX_PATH:PATH=$QtRoot", "-DQt6_DIR:PATH=$QtRoot\lib\cmake\Qt6",
    '-DQT=6', '-DPUBLIC_RELEASE=0', '-DUSE_LIBAV=ON',
    "-DSDL_INSTALL_PREFIX:PATH=$sdl", "-DFFMPEG_INSTALL_PREFIX:PATH=$ffmpeg",
    "-DLIBARCHIVE_INSTALL_PREFIX:PATH=$archiveRoot",
    '-DFCEUX_QT_DEVELOPER_SETUP=ON',
    "-DCMAKE_DISABLE_FIND_PACKAGE_Vulkan:BOOL=$($SkipVulkan.IsPresent)",
    "-DRETRO_ACHIEVEMENTS:BOOL=$($EnableRetroAchievements.IsPresent)")
if (!$SkipVulkan) {
    $configure += @("-DVulkan_INCLUDE_DIR:PATH=$VulkanSdk\Include",
        "-DVulkan_LIBRARY:FILEPATH=$VulkanSdk\Lib\vulkan-1.lib")
}
if ($EnableRetroAchievements) {
    $rcheevos = Join-Path $DependencyDirectory 'rcheevos-v12.5.0'
    if (!(Test-Path -LiteralPath "$rcheevos\.git")) {
        Invoke-Checked $git.Source @('clone', '--depth', '1', '--branch', 'v12.5.0',
            'https://github.com/RetroAchievements/rcheevos.git', $rcheevos)
    }
    Assert-Files $rcheevos @('include\rc_client.h')
    $configure += "-DRCHEEVOS_DIR:PATH=$rcheevos"
}
Invoke-Checked $cmake $configure
Assert-Files $BuildDirectory @('fceux.sln')
Write-Host "`nVS2022 Qt workspace setup SUCCEEDED in $($setupTimer.Elapsed.ToString('hh\:mm\:ss'))." -ForegroundColor Green
if ($Clean) {
    Write-Host "Clean start completed: the repository build folder was recreated at '$source\build'."
}
Write-Host 'Dependencies installed or reused and validated:'
Write-Host "  Qt: $QtRoot"
Write-Host "  SDL2: $sdl"
Write-Host "  FFmpeg: $ffmpeg"
Write-Host "  libarchive (via vcpkg): $archiveRoot"
if (!$SkipVulkan) {
    Write-Host "  Vulkan SDK: $VulkanSdk"
}
else {
    Write-Host '  Vulkan: disabled'
}
if ($EnableRetroAchievements) {
    Write-Host "  RetroAchievements: enabled ($rcheevos)"
}
else {
    Write-Host '  RetroAchievements: disabled'
}
Write-Host "Generated solution: $BuildDirectory\fceux.sln"
Write-Host 'The emulator has not been built by this script.'
Write-Host 'Open the solution, select Debug or RelWithDebInfo / x64, and press F5.'
Write-Host 'The first build deploys runtime DLLs and data automatically. No Qt VS extension is required.'
