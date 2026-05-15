param(
    [string]$Vst3Path = "C:\Program Files\Common Files\VST3\JamulusVST3.vst3",
    [string]$QtBinPath = "C:\Qt\6.5.3\msvc2019_64\bin"
)

$ErrorActionPreference = "Stop"

$src = Join-Path $Vst3Path "Contents\x86_64-win\JamulusVST3.vst3"
if (-not (Test-Path $src)) { Write-Error "Source plugin not found: $src"; exit 1 }

$temp = Join-Path ([IO.Path]::GetTempPath()) ("jamulus_inspect_{0}" -f ([Guid]::NewGuid().ToString()))
New-Item -ItemType Directory -Path $temp | Out-Null
$dest = Join-Path $temp "JamulusVST3.dll"
Copy-Item -Path $src -Destination $dest -Force
Write-Host "Copied plugin to: $dest"

# Inspect PE header
$fs = [IO.File]::OpenRead($dest)
$br = New-Object IO.BinaryReader($fs)
try {
    $mzBytes = $br.ReadBytes(2)
    $mz = [System.Text.Encoding]::ASCII.GetString($mzBytes)
    $fs.Seek(0x3C, [System.IO.SeekOrigin]::Begin) | Out-Null
    $e_lfanew = $br.ReadUInt32()
    $fs.Seek($e_lfanew, [System.IO.SeekOrigin]::Begin) | Out-Null
    $peBytes = $br.ReadBytes(4)
    $pe = [System.Text.Encoding]::ASCII.GetString($peBytes)
    $machine = $br.ReadUInt16()
} finally {
    $br.Close(); $fs.Close()
}

Write-Host "MZ: $mz"
Write-Host "PE: $pe"
Write-Host ('Machine: 0x{0:X4}' -f $machine)
switch ($machine) {
    0x8664 { Write-Host 'Arch: x86_64' }
    0x14c  { Write-Host 'Arch: x86' }
    0xAA64 { Write-Host 'Arch: ARM64' }
    default { Write-Host 'Arch: Unknown' }
}

# Run windeployqt with verbose to see why it fails to detect binary
$windeploy = Join-Path $QtBinPath 'windeployqt.exe'
if (-not (Test-Path $windeploy)) { Write-Error "windeployqt not found at: $windeploy"; exit 1 }

Write-Host "Running: $windeploy --release --no-translations --verbose $dest"
& $windeploy --release --no-translations --verbose $dest
Write-Host "windeployqt exit code: $LASTEXITCODE"
Write-Host "Temp folder retained at: $temp"