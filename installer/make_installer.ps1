<#
.SYNOPSIS
Builds the JamulusPlus packaged outputs and compiles a Windows installer.

.DESCRIPTION
1. Builds the Release VST3 bundle and standalone app.
2. Copies packaged outputs into dist/.
3. Compiles the Inno Setup installer with optional plugin/standalone components.
#>

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRootDir = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ProjectRootDir "b"
$Vst3Bundle = Join-Path $ProjectRootDir "dist\JamulusPlus.vst3"
$StandaloneDir = Join-Path $ProjectRootDir "dist\JamulusPlus-Standalone"

Write-Host "--- Starting JamulusPlus installer build ---" -ForegroundColor Cyan

if (-not (Test-Path $BuildDir)) {
    throw "Build directory not found at $BuildDir. Configure CMake first."
}

Push-Location $ProjectRootDir
try {
    & cmake --build .\b --config Release --target jamulus_dist
    & cmake --build .\b --config Release --target jamulus_dist_standalone
} finally {
    Pop-Location
}

if (-not (Test-Path $Vst3Bundle)) {
    throw "Packaged VST3 bundle not found at $Vst3Bundle"
}
if (-not (Test-Path $StandaloneDir)) {
    throw "Packaged standalone directory not found at $StandaloneDir"
}

$iscc = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $iscc)) {
    $iscc = Get-Command iscc.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
}
if (-not $iscc) {
    throw "Inno Setup Compiler (ISCC.exe) not found. Install Inno Setup 6."
}

$IssFile = Join-Path $ScriptDir "jamulus_vst3.iss"
& $iscc $IssFile
if ($LASTEXITCODE -ne 0) {
    throw "Installer compilation failed."
}

$OutputDir = Join-Path $ScriptDir "installer_output"
Write-Host "Installer created in: $OutputDir" -ForegroundColor Green
