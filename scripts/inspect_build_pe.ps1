param(
    [string]$BuildVstPath = "e:\\Web stuff\\Jamulus VST Wrapper\\jamulus\\juce-wrapper\\build\\plugin\\jamulus_vst3_artefacts\\Release\\VST3\\JamulusVST3.vst3"
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path $BuildVstPath)) { Write-Error "Build artifact not found: $BuildVstPath"; exit 1 }

Write-Host "Reading: $BuildVstPath"
$bytes = [System.IO.File]::ReadAllBytes($BuildVstPath)
$sz = $bytes.Length
Write-Host "File size: $sz"
if ($sz -lt 0x40) { Write-Error 'File too small to be PE'; exit 1 }
$mz = [System.Text.Encoding]::ASCII.GetString($bytes,0,2)
$e_lfanew = [BitConverter]::ToUInt32($bytes,0x3C)
if ($e_lfanew -lt 0 -or $e_lfanew -gt $sz-4) { Write-Error 'Invalid e_lfanew'; exit 1 }
$pe = [System.Text.Encoding]::ASCII.GetString($bytes,$e_lfanew,4)
$machine = [BitConverter]::ToUInt16($bytes,$e_lfanew+4)
Write-Host "MZ: $mz"
Write-Host "PE: $pe"
Write-Host ('Machine: 0x{0:X4}' -f $machine)
switch ($machine) {
    0x8664 { Write-Host 'Arch: x86_64' }
    0x14c  { Write-Host 'Arch: x86' }
    0xAA64 { Write-Host 'Arch: ARM64' }
    default { Write-Host 'Arch: Unknown' }
}

# Output first 64 bytes as hex for manual inspection
$first = $bytes[0..63]
$hex = ($first | ForEach-Object { $_.ToString('X2') }) -join ' '
Write-Host "First 64 bytes: $hex"