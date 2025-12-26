[CmdletBinding()]
param(
    # Path to the built VST3 module file (Windows: this is a DLL with a .vst3 extension)
    [string]$SourceModulePath = "E:\Web stuff\Jamulus VST Wrapper\jamulus\juce-wrapper\build\plugin\jamulus_vst3_artefacts\Release\VST3\JamulusVST3.vst3\Contents\x86_64-win\JamulusVST3.vst3",

    # Path to the installed VST3 bundle folder
    [string]$DestBundlePath = "C:\Program Files\Common Files\VST3\JamulusVST3.vst3",

    # Module file name inside Contents\x86_64-win
    [string]$ModuleFileName = "JamulusVST3.vst3",

    # Replace existing module (existing is backed up first)
    [switch]$Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Test-IsAdmin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    $p = New-Object Security.Principal.WindowsPrincipal($id)
    return $p.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Relaunch-Elevated {
    param([string[]]$PassThruArgs)

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = "powershell.exe"
    $psi.Arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" " + ($PassThruArgs -join ' ')
    $psi.Verb = "runas"
    [Diagnostics.Process]::Start($psi) | Out-Null
}

# Elevate if needed
if (-not (Test-IsAdmin)) {
    $argsList = @(
        "-SourceModulePath `"$SourceModulePath`"",
        "-DestBundlePath `"$DestBundlePath`"",
        "-ModuleFileName `"$ModuleFileName`""
    )
    if ($Force) { $argsList += "-Force" }

    Write-Host "Re-launching elevated to install into Program Files..."
    Relaunch-Elevated -PassThruArgs $argsList
    exit 0
}

$source = (Resolve-Path -LiteralPath $SourceModulePath).Path
if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
    throw "Source module not found: $SourceModulePath"
}

$destBundle = (Resolve-Path -LiteralPath $DestBundlePath -ErrorAction Stop).Path
if (-not (Test-Path -LiteralPath $destBundle -PathType Container)) {
    throw "Destination bundle folder not found: $DestBundlePath"
}

$destModuleDir = Join-Path $destBundle "Contents\x86_64-win"
$destModule = Join-Path $destModuleDir $ModuleFileName

if (-not (Test-Path -LiteralPath $destModuleDir -PathType Container)) {
    throw "Destination module folder not found: $destModuleDir"
}

if (Test-Path -LiteralPath $destModule) {
    if (-not $Force) {
        throw "Destination module already exists: $destModule`nRe-run with -Force to replace (existing will be backed up)."
    }
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $backup = "${destModule}.bak_${stamp}"
    Write-Host "Backing up existing module:" $destModule "->" $backup
    Copy-Item -LiteralPath $destModule -Destination $backup -Force
}

Write-Host "Updating module:" $source "->" $destModule
Copy-Item -LiteralPath $source -Destination $destModule -Force

# Copy qt.conf alongside the module to help Qt find plugins
$qtConfSource = Join-Path (Split-Path -Parent $PSCommandPath) "..\plugin\qt.conf"
if (Test-Path -LiteralPath $qtConfSource) {
    $qtConfDest = Join-Path $destModuleDir "qt.conf"
    Write-Host "Copying qt.conf:" $qtConfSource "->" $qtConfDest
    Copy-Item -LiteralPath $qtConfSource -Destination $qtConfDest -Force
}

# Run our manifest/structure fix script after install (creates top-level manifest.ttl etc)
$scriptDir = Split-Path -Parent $PSCommandPath
$fixScript = Join-Path $scriptDir "fix_installed_vst3_manifest.ps1"

if (Test-Path -LiteralPath $fixScript) {
    Write-Host "Running manifest/structure repair:" $fixScript
    & $fixScript -BundlePath $destBundle
} else {
    Write-Warning "Could not find fix script at: $fixScript (skipping repair)"
}

# Add the plugin's module directory to the user PATH so hosts can find Qt DLLs.
# This is necessary because most VST3 hosts don't add plugin directories to DLL search paths.
$currentPath = [Environment]::GetEnvironmentVariable("PATH", "User")
if ($currentPath -notlike "*$destModuleDir*") {
    Write-Host "Adding plugin directory to user PATH: $destModuleDir"
    $newPath = "$currentPath;$destModuleDir"
    [Environment]::SetEnvironmentVariable("PATH", $newPath, "User")
    Write-Host "NOTE: You may need to restart your DAW/host for the PATH change to take effect."
} else {
    Write-Host "Plugin directory already in user PATH: $destModuleDir"
}

Write-Host "Done. Updated bundle at: $destBundle"
