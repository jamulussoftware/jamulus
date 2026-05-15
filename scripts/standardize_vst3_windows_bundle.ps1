param(
    [string]$BundlePath = 'C:\Program Files\Common Files\VST3\JamulusVST3.vst3',
    [string]$ModuleBaseName = 'JamulusVST3'
)

function Fail([string]$msg){ Write-Error $msg; exit 1 }

if (-not (Test-Path $BundlePath)) { Fail "Bundle not found: $BundlePath" }

$nested = Join-Path $BundlePath 'Contents\x86_64-win'
if (-not (Test-Path $nested)) { Fail "Missing expected folder: $nested" }

$dll = Join-Path $nested ("{0}.dll" -f $ModuleBaseName)
$vst3bin = Join-Path $nested ("{0}.vst3" -f $ModuleBaseName)

# Backup key files
$backupRoot = "${BundlePath}.prestandardize.bak"
if (Test-Path $backupRoot) { Remove-Item -Recurse -Force $backupRoot }
Copy-Item -LiteralPath $BundlePath -Destination $backupRoot -Recurse -Force
Write-Output "Backup created: $backupRoot"

# Ensure module binary is a .vst3 file (PE DLL with .vst3 extension)
if ((Test-Path $dll) -and -not (Test-Path $vst3bin)) {
    Rename-Item -LiteralPath $dll -NewName ("{0}.vst3" -f $ModuleBaseName) -Force
    Write-Output "Renamed module: $dll -> $vst3bin"
} elseif (Test-Path $vst3bin) {
    Write-Output "Module already present: $vst3bin"
} else {
    Fail "Neither module found: $dll or $vst3bin"
}

# Update manifests to reference the .vst3 module filename
$manifestPaths = @(
    (Join-Path $BundlePath 'manifest.ttl'),
    (Join-Path $nested 'manifest.ttl')
)

foreach ($mp in $manifestPaths) {
    if (-not (Test-Path $mp)) {
        Write-Warning "Manifest missing: $mp"
        continue
    }

    $txt = Get-Content -LiteralPath $mp -Raw
    $txt2 = $txt -replace 'JamulusVST3\.dll', 'JamulusVST3.vst3'
    Set-Content -LiteralPath $mp -Value $txt2 -Encoding UTF8
    Write-Output "Updated manifest binary reference: $mp"
}

Write-Output "Standardization complete."
Write-Output "Expected module path: $vst3bin"
