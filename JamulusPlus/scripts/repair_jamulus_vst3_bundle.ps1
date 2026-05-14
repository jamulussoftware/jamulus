param(
    [string]$BundlePath = 'C:\Program Files\Common Files\VST3\JamulusVST3.vst3',
    [string]$ModuleBaseName = 'JamulusVST3'
)

function Fail([string]$msg){ Write-Error $msg; exit 1 }

if (-not (Test-Path $BundlePath)) { Fail "Bundle not found: $BundlePath" }

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

$flatten = Join-Path $scriptDir 'flatten_and_fix_vst3.ps1'
$standardize = Join-Path $scriptDir 'standardize_vst3_windows_bundle.ps1'
$probe = Join-Path $scriptDir 'probe_vst3_exports.ps1'

if (Test-Path $flatten) {
    & powershell -NoProfile -ExecutionPolicy Bypass -File $flatten -BundlePath $BundlePath
} else {
    Write-Warning "Missing helper: $flatten"
}

if (Test-Path $standardize) {
    & powershell -NoProfile -ExecutionPolicy Bypass -File $standardize -BundlePath $BundlePath -ModuleBaseName $ModuleBaseName
} else {
    Write-Warning "Missing helper: $standardize"
}

$resources = Join-Path $BundlePath 'Contents\Resources'
$moduleinfo = Join-Path $resources 'moduleinfo.json'
if (-not (Test-Path $moduleinfo)) { Fail "Missing moduleinfo.json: $moduleinfo" }

# Normalize moduleinfo.json to strict JSON (no trailing commas etc)
$miBackup = "${moduleinfo}.prejsonfix.bak"
Copy-Item -LiteralPath $moduleinfo -Destination $miBackup -Force
Write-Output "Backup moduleinfo.json: $miBackup"

$jsonObj = $null
try {
    $raw = Get-Content -LiteralPath $moduleinfo -Raw
    # Windows PowerShell 5.1's ConvertFrom-Json is strict and will reject trailing commas.
    # Sanitise common non-strict JSON patterns (trailing commas before } or ]).
    $sanitised = [Regex]::Replace($raw, ',\s*([\}\]])', '$1')
    $jsonObj = $sanitised | ConvertFrom-Json -ErrorAction Stop

    # ConvertTo-Json produces strict JSON output.
    $strict = $jsonObj | ConvertTo-Json -Depth 50
    Set-Content -LiteralPath $moduleinfo -Value $strict -Encoding UTF8
    Write-Output "Rewrote moduleinfo.json as strict JSON: $moduleinfo"
} catch {
    Fail ("Failed to parse/sanitise moduleinfo.json: {0}" -f $_)
}

# Rebuild manifest.ttl from moduleinfo.json (use Classes entries)
$vendor = 'unknown'
$version = '0.0'
$classes = @()
if ($jsonObj.'Factory Info' -and $jsonObj.'Factory Info'.Vendor) { $vendor = $jsonObj.'Factory Info'.Vendor }
if ($jsonObj.Version) { $version = $jsonObj.Version }
if ($jsonObj.Classes) { $classes = $jsonObj.Classes }

if (-not $classes -or $classes.Count -eq 0) {
    Fail 'moduleinfo.json has no Classes; cannot generate manifest.ttl'
}

$binaryName = ("{0}.vst3" -f $ModuleBaseName)
$componentLines = @()
foreach ($c in $classes) {
    $cid = ($c.CID -replace '"','')
    $cname = ($c.Name -replace '"','')
    $ccat = ($c.Category -replace '"','')
    $componentLines += ('      [ vst3:cid "' + $cid + '" ; vst3:name "' + $cname + '" ; vst3:category "' + $ccat + '" ; vst3:binary "' + $binaryName + '" ]')
}

$manifestLines = @(
    '@prefix vst3: <http://www.steinberg.net/ns/vst3#> .',
    '@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .',
    '',
    '<> a vst3:Factory ;',
    ('    vst3:vendor "' + $vendor + '" ;'),
    ('    vst3:version "' + $version + '" ;'),
    '    vst3:component (',
    ($componentLines -join "`n"),
    '    ) .'
)

$manifest = $manifestLines -join "`n"

$topManifest = Join-Path $BundlePath 'manifest.ttl'
$archManifest = Join-Path $BundlePath ('Contents\x86_64-win\manifest.ttl')

Set-Content -LiteralPath $topManifest -Value $manifest -Encoding UTF8
Set-Content -LiteralPath $archManifest -Value $manifest -Encoding UTF8
Write-Output "Wrote manifests: $topManifest and $archManifest"

# Validate expected module path
$modulePath = Join-Path $BundlePath ("Contents\\x86_64-win\\{0}.vst3" -f $ModuleBaseName)
if (-not (Test-Path $modulePath)) { Fail "Missing VST3 module file: $modulePath" }

Write-Output "Attempting headless VST3 factory probe (host-like DLL search)..."
if (Test-Path $probe) {
    & powershell -NoProfile -ExecutionPolicy Bypass -File $probe -DllPath $modulePath -AddDllDir
} else {
    Write-Warning "Missing probe script: $probe"
}

Write-Output "Repair complete. Restart AudioPluginHost/Cubase and rescan."
