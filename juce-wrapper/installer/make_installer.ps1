<#
.SYNOPSIS
Builds and packages the Jamulus VST3 plugin into a single installer .exe.

.DESCRIPTION
1. Ensures the Release build is up to date.
2. Runs windeployqt to collect Qt dependencies.
3. Compiles the Inno Setup script into a redistributable installer.
#>

param(
    [string]$QtBinPath = ""
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRootDir = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ProjectRootDir "build"
$ReleaseBundle = Join-Path $BuildDir "plugin\jamulus_vst3_artefacts\Release\VST3\JamulusVST3.vst3"

Write-Host "--- Starting Installer Build Process ---" -ForegroundColor Cyan

# 1. Ensure Build Exists
if (-not (Test-Path $ReleaseBundle)) {
    Write-Host "Release bundle not found at $ReleaseBundle." -ForegroundColor Yellow
    Write-Host "Please build the 'Release' configuration in CMake/IDEs first."
    exit 1
}

# 2. Deploy Qt Dependencies
Write-Host "Step 1: Deploying Qt dependencies..." -ForegroundColor Cyan
$DeployScript = Join-Path $ProjectRootDir "scripts\deploy_vst3_deps.ps1"
& $DeployScript -Vst3Path $ReleaseBundle -QtBinPath $QtBinPath

# 3. Find Inno Setup Compiler (ISCC)
$iscc = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $iscc)) {
    $iscc = Get-Command iscc.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
}

if (-not $iscc) {
    Write-Host "Inno Setup Compiler (ISCC.exe) not found!" -ForegroundColor Red
    Write-Host "Please install Inno Setup 6 from https://jrsoftware.org/isdl.php"
    exit 1
}

# 4. Compile Installer
Write-Host "Step 2: Compiling Installer..." -ForegroundColor Cyan
$IssFile = Join-Path $ScriptDir "jamulus_vst3.iss"
& $iscc $IssFile

if ($LASTEXITCODE -eq 0) {
    $OutputDir = Join-Path $ScriptDir "installer_output"
    Write-Host "`n--- Success! ---" -ForegroundColor Green
    Write-Host "Installer created in: $OutputDir"
} else {
    Write-Error "Installer compilation failed."
}
