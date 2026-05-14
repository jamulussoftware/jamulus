param(
    [string]$BuildVstPath = "e:\\Web stuff\\Jamulus VST Wrapper\\jamulus\\juce-wrapper\\build\\plugin\\jamulus_vst3_artefacts\\Release\\VST3\\JamulusVST3.vst3",
    [string]$QtBinPath = "C:\\Qt\\6.5.3\\msvc2019_64\\bin"
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $BuildVstPath)) { Write-Error "Build artifact not found: $BuildVstPath"; exit 1 }

$tempDir = Join-Path ([IO.Path]::GetTempPath()) ("jamulus_build_inspect_{0}" -f ([Guid]::NewGuid().ToString()))
New-Item -ItemType Directory -Path $tempDir | Out-Null
$src = $BuildVstPath
$dest = Join-Path $tempDir 'JamulusVST3.dll'
Copy-Item -Path $src -Destination $dest -Force
Write-Host "Copied build artifact to: $dest"

$bytes = [System.IO.File]::ReadAllBytes($dest)
$sz = $bytes.Length
if ($sz -lt 0x40) { Write-Error 'File too small to be PE'; exit 1 }
$mz = [System.Text.Encoding]::ASCII.GetString($bytes,0,2)
$e_lfanew = [BitConverter]::ToUInt32($bytes,0x3C)
$pe = [System.Text.Encoding]::ASCII.GetString($bytes,$e_lfanew,4)
$machine = [BitConverter]::ToUInt16($bytes,$e_lfanew+4)

Write-Host "File size: $sz"
Write-Host "MZ: $mz"
Write-Host "PE: $pe"
Write-Host ('Machine: 0x{0:X4}' -f $machine)
switch ($machine) {
    0x8664 { Write-Host 'Arch: x86_64' }
    0x14c  { Write-Host 'Arch: x86' }
    0xAA64 { Write-Host 'Arch: ARM64' }
    default { Write-Host 'Arch: Unknown' }
}

$windeploy = Join-Path $QtBinPath 'windeployqt.exe'
if (-not (Test-Path $windeploy)) { Write-Error "windeployqt not found at: $windeploy"; exit 1 }
Write-Host "Running windeployqt --verbose on: $dest"
& $windeploy --release --no-translations --verbose $dest
Write-Host "windeployqt exit code: $LASTEXITCODE"
Write-Host "Temp folder retained at: $tempDir"