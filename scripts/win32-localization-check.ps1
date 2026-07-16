param(
    [string]$ResourceFile = "src/drivers/win/res.rc",
    [string]$ResourceHeader = "src/drivers/win/resource.h"
)

$ErrorActionPreference = "Stop"

function Get-ResourceBlock {
    param(
        [string]$Text,
        [string]$Start,
        [string]$End
    )

    $startIndex = $Text.IndexOf($Start)
    if ($startIndex -lt 0) {
        throw "Missing resource block start: $Start"
    }
    $endIndex = $Text.IndexOf($End, $startIndex)
    if ($endIndex -lt 0) {
        throw "Missing resource block end: $End"
    }
    return $Text.Substring($startIndex, $endIndex - $startIndex)
}

function Get-NamedResources {
    param([string]$Block)

    $items = @{}
    foreach ($line in ($Block -split "`r?`n")) {
        if ($line -match '^\s*([A-Za-z_][A-Za-z0-9_]*|"[^"]+"|IDD_[A-Za-z0-9_]+)\s+(DIALOGEX|DIALOG|MENU|MENUEX)\b') {
            $items[$matches[1] + "|" + $matches[2]] = $true
        }
    }
    return $items
}

function Get-StringIds {
    param([string]$Block)

    $ids = @{}
    foreach ($line in ($Block -split "`r?`n")) {
        if ($line -match '^\s*(IDS_LOC_[A-Za-z0-9_]+)\s+"') {
            $ids[$matches[1]] = $true
        }
    }
    return $ids
}

$rc = Get-Content -Raw -Encoding UTF8 $ResourceFile
$header = Get-Content -Raw $ResourceHeader

if ($rc -notmatch 'LANGUAGE\s+LANG_ENGLISH,\s*SUBLANG_ENGLISH_US') {
    throw "Missing en-US language block."
}
if ($rc -notmatch 'LANGUAGE\s+LANG_CHINESE,\s*SUBLANG_CHINESE_SIMPLIFIED') {
    throw "Missing zh-CN language block."
}
if ($rc -notmatch '#pragma\s+code_page\(65001\)') {
    throw "Missing UTF-8 code page pragma for localized resources."
}

$languageMenuIds = @("MENU_LANGUAGE_AUTO", "MENU_LANGUAGE_EN_US", "MENU_LANGUAGE_ZH_CN")
foreach ($id in $languageMenuIds) {
    if ($header -notmatch "\b$id\b") {
        throw "Missing $id in resource.h."
    }
}

$englishBlock = Get-ResourceBlock $rc "LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US" "#endif    // English (United States) resources"
$chineseBlock = Get-ResourceBlock $rc "LANGUAGE LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED" "#endif    // Chinese (Simplified, PRC) resources"

foreach ($id in $languageMenuIds) {
    if ($englishBlock -notmatch "\b$id\b") {
        throw "Missing $id in en-US resources."
    }
    if ($chineseBlock -notmatch "\b$id\b") {
        throw "Missing $id in zh-CN resources."
    }
}

$englishResources = Get-NamedResources $englishBlock
$chineseResources = Get-NamedResources $chineseBlock
$missingResources = @()
foreach ($key in $englishResources.Keys) {
    if (-not $chineseResources.ContainsKey($key)) {
        $missingResources += $key
    }
}
if ($missingResources.Count -gt 0) {
    throw "zh-CN is missing resource(s): $($missingResources -join ', ')"
}

$englishStrings = Get-StringIds $englishBlock
$chineseStrings = Get-StringIds $chineseBlock
$headerStrings = @()
foreach ($match in [regex]::Matches($header, '#define\s+(IDS_LOC_[A-Za-z0-9_]+)\s+\d+')) {
    $headerStrings += $match.Groups[1].Value
}
foreach ($id in $headerStrings) {
    if (-not $englishStrings.ContainsKey($id)) {
        throw "en-US string table missing $id."
    }
    if (-not $chineseStrings.ContainsKey($id)) {
        throw "zh-CN string table missing $id."
    }
}

if ($chineseBlock -match '"按钮"\s*,\s*BS_' -or $chineseBlock -match '"静态"') {
    throw "A Win32 control class name appears to have been translated."
}

Write-Host "Win32 localization check passed: $($englishResources.Count) resources and $($headerStrings.Count) dynamic strings covered."
