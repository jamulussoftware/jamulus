param(
    [string]$NestedDll = "C:\Program Files\Common Files\VST3\JamulusVST3.vst3\Contents\x86_64-win\JamulusVST3.vst3\JamulusVST3.dll",
    [string]$QtBinPath = "C:\Qt\6.5.3\msvc2019_64\bin"
)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path $NestedDll)) { Write-Error "Nested DLL not found: $NestedDll"; exit 1 }
$temp = Join-Path ([IO.Path]::GetTempPath()) ("jamulus_windeploy_{0}" -f ([Guid]::NewGuid().ToString()))
New-Item -ItemType Directory -Path $temp | Out-Null
$copy = Join-Path $temp 'JamulusVST3.dll'
Copy-Item -Path $NestedDll -Destination $copy -Force
Write-Host "Copied nested DLL to: $copy"
$windeploy = Join-Path $QtBinPath 'windeployqt.exe'
if (-not (Test-Path $windeploy)) { Write-Error "windeployqt not found at: $windeploy"; exit 1 }
Write-Host "Running windeployqt --verbose on: $copy"
& $windeploy $copy --release --no-translations --verbose=1
Write-Host "windeployqt exit code: $LASTEXITCODE"
Write-Host "Temporary folder: $temp"
Write-Host "If deployment succeeded, copy files from $temp back into the installed bundle's x86_64-win folder (requires Administrator)."