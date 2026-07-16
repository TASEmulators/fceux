param(
    [string]$ResourceFile = "src/drivers/win/res.rc",
    [string]$ResourceHeader = "src/drivers/win/resource.h",
    [string]$SourceAuditFile = "docs/localization/win32-source-string-audit.json",
    [string]$LocalizationSourceFile = "src/drivers/win/localization.cpp",
    [string]$Win32SourceRoot = "src/drivers/win"
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
        if ($line -match '^\s*(IDS_LOC_[A-Za-z0-9_]+)\s+"((?:\\.|[^"])*)"') {
            $ids[$matches[1]] = $matches[2]
        }
    }
    return $ids
}

function Get-FormatTokens {
    param([string]$Text)

    $tokens = @()
    foreach ($match in [regex]::Matches($Text, '%(?:%|[-+#0 ]*(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:hh|h|ll|l|I32|I64|I|w)?[diuoxXfFeEgGaAcCsSpn])')) {
        $token = $match.Value
        if ($token -ne '%%') {
            $tokens += $token
        }
    }
    return $tokens
}

function Unescape-CString {
    param([string]$Text)

    return $Text.Replace('\n', "`n").Replace('\"', '"').Replace('\\', '\')
}

function Get-DynamicStringMap {
    param([string]$Text)

    $items = @{}
    foreach ($match in [regex]::Matches($Text, 'strings\["((?:\\.|[^"])*)"\]\s*=\s*(IDS_LOC_[A-Za-z0-9_]+)')) {
        $items[(Unescape-CString $match.Groups[1].Value)] = $match.Groups[2].Value
    }
    return $items
}

$rc = Get-Content -Raw -Encoding UTF8 $ResourceFile
$header = Get-Content -Raw $ResourceHeader
$localizationSource = Get-Content -Raw $LocalizationSourceFile

if (-not (Test-Path $SourceAuditFile)) {
    throw "Missing Win32 source string audit file: $SourceAuditFile"
}
$sourceAudit = Get-Content -Raw -Encoding UTF8 $SourceAuditFile | ConvertFrom-Json

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

    $englishFormats = @(Get-FormatTokens $englishStrings[$id])
    $chineseFormats = @(Get-FormatTokens $chineseStrings[$id])
    if (($englishFormats -join '|') -ne ($chineseFormats -join '|')) {
        throw "Format specifier mismatch for $id. en-US=[$($englishFormats -join ', ')] zh-CN=[$($chineseFormats -join ', ')]"
    }
}

foreach ($token in $sourceAudit.preservedTokens) {
    foreach ($id in $headerStrings) {
        if ($englishStrings[$id].Contains($token) -and -not $chineseStrings[$id].Contains($token)) {
            throw "Preserved token '$token' was changed or removed in $id."
        }
    }
}

$dynamicStrings = Get-DynamicStringMap $localizationSource
if ($dynamicStrings.Count -lt $sourceAudit.dynamicStringCoverage.minimumResourceCount) {
    throw "Dynamic source string coverage regressed: expected at least $($sourceAudit.dynamicStringCoverage.minimumResourceCount), found $($dynamicStrings.Count)."
}
foreach ($id in $dynamicStrings.Values) {
    if (-not $englishStrings.ContainsKey($id) -or -not $chineseStrings.ContainsKey($id)) {
        throw "Dynamic source string map references unpaired resource $id."
    }
}

$sourceExceptions = @{}
foreach ($exception in $sourceAudit.knownSourceExceptions) {
    $sourceExceptions["$($exception.source):$($exception.line):$($exception.text)"] = $true
    if (-not $exception.reason) {
        throw "Source audit exception is missing a reason: $($exception.source):$($exception.line)"
    }
}

$unregistered = @()
$sourceFiles = Get-ChildItem -Path $Win32SourceRoot -Recurse -Include *.cpp,*.c,*.h
foreach ($file in $sourceFiles) {
    $relativePath = $file.FullName.Substring((Get-Location).Path.Length + 1).Replace('\', '/')
    $displayPath = $file.FullName.Substring((Get-Location).Path.Length + 1)
    $lines = Get-Content $file.FullName
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line -notmatch 'MessageBox\s*\(' -or $line -match '^\s*//') {
            continue
        }
        foreach ($match in [regex]::Matches($line, '"((?:\\.|[^"])*)"')) {
            $text = Unescape-CString $match.Groups[1].Value
            if ($text -notmatch '[A-Za-z]') {
                continue
            }
            $key = "${displayPath}:$($i + 1):$text"
            $altKey = "${relativePath}:$($i + 1):$text"
            if (-not $dynamicStrings.ContainsKey($text) -and -not $sourceExceptions.ContainsKey($key) -and -not $sourceExceptions.ContainsKey($altKey)) {
                $unregistered += $key
            }
        }
    }
}
if ($unregistered.Count -gt 0) {
    throw "Unregistered Win32 MessageBox source string(s): $($unregistered -join '; ')"
}

if ($chineseBlock -match '"按钮"\s*,\s*BS_' -or $chineseBlock -match '"静态"') {
    throw "A Win32 control class name appears to have been translated."
}

Write-Host "Win32 localization check passed: $($englishResources.Count) resources, $($headerStrings.Count) resource strings, and $($dynamicStrings.Count) source strings covered."
