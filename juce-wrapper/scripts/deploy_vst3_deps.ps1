<#
.SYNOPSIS
Deploys Qt dependencies to the VST3 plugin bundle using windeployqt.

.DESCRIPTION
This script locates the installed VST3 plugin and runs windeployqt to copy the necessary
Qt DLLs (Core, Gui, Widgets, Network, etc.) into the plugin directory.
This is required for the plugin to load in a DAW.

.PARAMETER Vst3Path
Path to the installed VST3 bundle.
Default: "C:\Program Files\Common Files\VST3\JamulusVST3.vst3"

.PARAMETER QtBinPath
Path to the Qt bin directory containing windeployqt.exe.
If not provided, it tries to guess based on common locations or the user's previous input.
#>

param(
    [string]$Vst3Path = "C:\Program Files\Common Files\VST3\JamulusVST3.vst3",
    [string]$QtBinPath = ""
)

$ErrorActionPreference = "Stop"

# 1. Find the VST3 binary
$binaryPath = Join-Path $Vst3Path "Contents\x86_64-win\JamulusVST3.vst3"

if (-not (Test-Path $binaryPath)) {
    # Check if it was renamed to .dll by the other script
    $dllPath = Join-Path $Vst3Path "Contents\x86_64-win\JamulusVST3.dll"
    if (Test-Path $dllPath) {
        Write-Warning "Found binary as .dll instead of .vst3. Renaming it back to .vst3 for standard compliance."
        Rename-Item -Path $dllPath -NewName "JamulusVST3.vst3"
        $binaryPath = Join-Path $Vst3Path "Contents\x86_64-win\JamulusVST3.vst3"
    } else {
        Write-Error "VST3 binary not found at: $binaryPath"
        exit 1
    }
}

Write-Host "Found VST3 binary: $binaryPath"

# 2. Find windeployqt
if ([string]::IsNullOrEmpty($QtBinPath)) {
    # Try to guess based on the user's previous input or common paths
    $candidates = @(
        "C:\Qt\6.5.3\msvc2019_64\bin",
        "C:\Qt\6.5.0\msvc2019_64\bin",
        "C:\Qt\6.6.0\msvc2019_64\bin",
        "C:\Qt\5.15.2\msvc2019_64\bin"
    )
    
    foreach ($c in $candidates) {
        if (Test-Path (Join-Path $c "windeployqt.exe")) {
            $QtBinPath = $c
            break
        }
    }
}

if ([string]::IsNullOrEmpty($QtBinPath) -or -not (Test-Path (Join-Path $QtBinPath "windeployqt.exe"))) {
    Write-Error "Could not find windeployqt.exe. Please provide the path to your Qt bin directory (e.g., -QtBinPath 'C:\Qt\6.5.3\msvc2019_64\bin')."
    exit 1
}

$windeployqt = Join-Path $QtBinPath "windeployqt.exe"
Write-Host "Using windeployqt: $windeployqt"

# 3. Run windeployqt in a temporary folder
Write-Host "Deploying Qt dependencies (using temporary workspace)..."

$binaryDir = Split-Path -Parent $binaryPath

# create a temporary working folder
$tempDir = Join-Path ([IO.Path]::GetTempPath()) ("jamulus_vst3_deploy_{0}" -f ([Guid]::NewGuid().ToString()))
New-Item -ItemType Directory -Path $tempDir | Out-Null

$tempDll = Join-Path $tempDir "JamulusVST3.dll"
Copy-Item -Path $binaryPath -Destination $tempDll -Force

try {
    Write-Host "Running windeployqt on: $tempDll"
    & $windeployqt --release --no-translations --compiler-runtime "$tempDll"

    if ($LASTEXITCODE -ne 0) {
        Write-Error "windeployqt failed with exit code $LASTEXITCODE"
        exit 1
    }

    # Copy any generated files (DLLs, platforms, etc.) back into the plugin folder
    Get-ChildItem -Path $tempDir -Recurse -File | ForEach-Object {
        if ($_.FullName -ne $tempDll) {
            $relative = $_.FullName.Substring($tempDir.Length).TrimStart('\')
            $dest = Join-Path $binaryDir $relative
            $destDir = Split-Path -Parent $dest
            if (-not (Test-Path $destDir)) { New-Item -ItemType Directory -Path $destDir | Out-Null }
            Copy-Item -Path $_.FullName -Destination $dest -Force
            Write-Host "Deployed: $relative"
        }
    }

    Write-Host "Successfully deployed Qt dependencies. Please restart your DAW and rescan plugins."

} finally {
    Remove-Item -Recurse -Force $tempDir -ErrorAction SilentlyContinue
}
