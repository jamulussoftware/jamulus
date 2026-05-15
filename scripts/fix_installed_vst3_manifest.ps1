<#
fix_installed_vst3_manifest.ps1

Ensures the installed VST3 bundle has a correct structure:
- The module file is at Contents\x86_64-win\<name>.vst3 (a DLL with .vst3 extension)
- A manifest.ttl exists alongside the module (at Contents\x86_64-win\manifest.ttl)
- moduleinfo.json in Resources is valid JSON (removes trailing commas)

Usage (run as Administrator):
    .\fix_installed_vst3_manifest.ps1 -BundlePath "C:\Program Files\Common Files\VST3\JamulusVST3.vst3"
#>

param(
    [string]$BundlePath = 'C:\Program Files\Common Files\VST3\JamulusVST3.vst3',
    [string]$ModuleInfoPath = ''
)

function Fail($msg){ Write-Error $msg; exit 1 }

# admin check
$curr = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($curr)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)){
    Fail "This script must be run as Administrator. Open PowerShell as Administrator and re-run."
}

if (-not (Test-Path $BundlePath)) { Fail "Bundle not found: $BundlePath" }

$archDir = Join-Path $BundlePath 'Contents\x86_64-win'
$moduleFile = Join-Path $archDir 'JamulusVST3.vst3'

# Check if there's a nested folder structure that needs flattening
$nestedFolder = $moduleFile
$nestedDll = Join-Path $nestedFolder 'JamulusVST3.dll'

if ((Test-Path $nestedFolder -PathType Container) -and (Test-Path $nestedDll -PathType Leaf)) {
    Write-Output "Found nested folder structure. Flattening..."
    
    # Save existing manifest if present
    $nestedManifest = Join-Path $nestedFolder 'manifest.ttl'
    $manifestBackup = $null
    if (Test-Path $nestedManifest) {
        $manifestBackup = Join-Path $archDir 'manifest.ttl.from_nested'
        Copy-Item $nestedManifest $manifestBackup -Force
    }
    
    # Move the DLL out
    $tempDll = Join-Path $archDir 'JamulusVST3.dll.temp'
    Move-Item $nestedDll $tempDll -Force
    
    # Remove the nested folder
    Remove-Item $nestedFolder -Recurse -Force
    
    # Rename the DLL to .vst3
    Move-Item $tempDll $moduleFile -Force
    Write-Output "Flattened: $moduleFile is now the module file"
}

# Verify the module file exists and is a file (not a directory)
if (-not (Test-Path $moduleFile -PathType Leaf)) {
    # Maybe it's named .dll instead of .vst3?
    $dllFile = Join-Path $archDir 'JamulusVST3.dll'
    if (Test-Path $dllFile -PathType Leaf) {
        Write-Output "Renaming $dllFile to $moduleFile"
        Rename-Item $dllFile -NewName 'JamulusVST3.vst3' -Force
    } else {
        Fail "Module file not found at $moduleFile (and no .dll fallback found)"
    }
}

# locate moduleinfo.json
if ($ModuleInfoPath -and (Test-Path $ModuleInfoPath)) {
    $moduleinfo = $ModuleInfoPath
} else {
    $candidate1 = Join-Path $BundlePath 'Contents\Resources\moduleinfo.json'
    if (Test-Path $candidate1) { $moduleinfo = $candidate1 }
    else {
        # try common workspace build path relative to script
        $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
        $guess = Join-Path $scriptDir '..\build\plugin\jamulus_vst3_artefacts\Release\VST3\JamulusVST3.vst3\Contents\Resources\moduleinfo.json'
        $guess = (Resolve-Path -LiteralPath $guess -ErrorAction SilentlyContinue)
        if ($guess) { $moduleinfo = $guess.Path } else { $moduleinfo = $null }
    }
}

if (-not $moduleinfo -or -not (Test-Path $moduleinfo)) {
    Write-Warning "moduleinfo.json not found. A minimal manifest will be created with placeholder values."
}

# parse moduleinfo.json if present
$classes = @()
$vendor = 'yourcompany'
$version = '0.1'
if ($moduleinfo) {
    try {
        $raw = Get-Content -Raw -Path $moduleinfo
        # Some generators emit JSON with trailing commas; sanitise for stricter parsers.
        $sanitised = [Regex]::Replace($raw, ',\s*([\}\]])', '$1')
        $json = $sanitised | ConvertFrom-Json -ErrorAction Stop
        if ($json.'Factory Info' -and $json.'Factory Info'.Vendor) { $vendor = $json.'Factory Info'.Vendor }
        if ($json.Version) { $version = $json.Version }
        if ($json.Classes) { $classes = $json.Classes }
    } catch {
        Write-Warning "Failed to parse moduleinfo.json, falling back to placeholders. Error: $_"
    }
}

if (-not $classes -or $classes.Count -eq 0) {
    # create a default class entry
    $classes = @(@{ CID = 'ABCDEF019182FAEB4D616E754768626B'; Name = 'JamulusVST3'; Category = 'Audio Module Class' })
}

# Build manifest.ttl - place it at archDir level (Contents\x86_64-win\manifest.ttl)
$binaryName = 'JamulusVST3.vst3'
$componentLines = @()
foreach ($c in $classes) {
    $cid = ($c.CID -replace '"','')
    $cname = ($c.Name -replace '"','')
    $ccat = ($c.Category -replace '"','')
    $componentLines += "      [ vst3:cid ""$cid"" ; vst3:name ""$cname"" ; vst3:category ""$ccat"" ; vst3:binary ""$binaryName"" ]"
}

$componentsText = $componentLines -join "`n"

$manifestLines = @()
$manifestLines += '@prefix vst3: <http://www.steinberg.net/ns/vst3#> .'
$manifestLines += '@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .'
$manifestLines += ''
$manifestLines += '<> a vst3:Factory ;'
$manifestLines += ('    vst3:vendor "' + $vendor + '" ;')
$manifestLines += ('    vst3:version "' + $version + '" ;')
$manifestLines += '    vst3:component ('
$manifestLines += $componentsText
$manifestLines += '    ) .'

$manifestContent = $manifestLines -join "`n"
$manifestPath = Join-Path $archDir 'manifest.ttl'
Set-Content -Path $manifestPath -Value $manifestContent -Encoding UTF8
Write-Output "Wrote manifest: $manifestPath"

Write-Output "Done. Plugin structure fixed at: $BundlePath"
