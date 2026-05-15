param(
    [string]$BundlePath = 'C:\Program Files\Common Files\VST3\JamulusVST3.vst3'
)
function Fail($msg){ Write-Error $msg; exit 1 }

if (-not (Test-Path $BundlePath)) { Fail "Bundle not found: $BundlePath" }

$nestedFolder = Join-Path $BundlePath 'Contents\x86_64-win'
$innerSub = Join-Path $nestedFolder 'JamulusVST3.vst3'

Write-Output "Bundle: $BundlePath"
Write-Output "Nested folder: $nestedFolder"
Write-Output "Detected inner subfolder: $innerSub exists? $(Test-Path $innerSub)"

# If innerSub exists and is a folder, move its contents up one level
if (Test-Path $innerSub -PathType Container) {
    Write-Output "Flattening inner folder: moving contents of $innerSub to $nestedFolder"
    Get-ChildItem -LiteralPath $innerSub -Force | ForEach-Object {
        $src = $_.FullName
        $dest = Join-Path $nestedFolder $_.Name
        # If destination exists, back it up
        if (Test-Path $dest) {
            $bak = "${dest}.preflatten.bak"
            Write-Output "Backing up existing $dest to $bak"
            Remove-Item -Recurse -Force $bak -ErrorAction SilentlyContinue
            try { Move-Item -LiteralPath $dest -Destination $bak -Force -ErrorAction Stop } catch { Write-Warning ("Failed to backup {0}: {1}" -f $dest, $_) }
        }
        try {
            Move-Item -LiteralPath $src -Destination $nestedFolder -Force -ErrorAction Stop
            Write-Output "Moved: $src -> $nestedFolder"
        } catch {
            Write-Warning ("Failed to move {0}: {1}" -f $src, $_)
        }
    }
    # remove innerSub if empty
    try {
        Remove-Item -LiteralPath $innerSub -Recurse -Force -ErrorAction SilentlyContinue
        Write-Output "Removed empty inner folder: $innerSub"
    } catch { Write-Warning ("Could not remove inner folder: {0}" -f $_) }
} else {
    Write-Output "No inner .vst3 folder to flatten."
}

# Ensure a top-level manifest.ttl exists
$moduleinfo = Join-Path $BundlePath 'Contents\Resources\moduleinfo.json'
$vendor = 'yourcompany'
$version = '0.1'
$classes = @(@{ CID = 'ABCDEF019182FAEB4D616E754768626B'; Name = 'JamulusVST3'; Category = 'Audio Module Class' })
if (Test-Path $moduleinfo) {
    try {
        $raw = Get-Content -Raw -Path $moduleinfo
        $sanitised = [Regex]::Replace($raw, ',\s*([\}\]])', '$1')
        $json = $sanitised | ConvertFrom-Json -ErrorAction Stop
        if ($json.'Factory Info' -and $json.'Factory Info'.Vendor) { $vendor = $json.'Factory Info'.Vendor }
        if ($json.Version) { $version = $json.Version }
        if ($json.Classes) { $classes = $json.Classes }
    } catch {
        Write-Warning ("Failed to parse moduleinfo.json, falling back to placeholders. Error: {0}" -f $_)
    }
} else { Write-Output "No moduleinfo.json at $moduleinfo" }

$binaryName = 'JamulusVST3.dll'
$componentLines = @()
foreach ($c in $classes) {
    $cid = ($c.CID -replace '"','')
    $cname = ($c.Name -replace '"','')
    $ccat = ($c.Category -replace '"','')
    $componentLines += ('      [ vst3:cid "' + $cid + '" ; vst3:name "' + $cname + '" ; vst3:category "' + $ccat + '" ; vst3:binary "' + $binaryName + '" ]')
}
$componentsText = $componentLines -join "`n"
$manifestLines = @()
$manifestLines += '@prefix vst3: <http://www.steinberg.net/ns/vst3#> .'
$manifestLines += '@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .'
$manifestLines += ''
$manifestLines += '<> a vst3:Factory ;'
$manifestLines += ('    vst3:vendor "' + $vendor + '" ;')
$manifestLines += ('    vst3:version "' + $version + '" ;')
$manifestLines += '    vst3:component ('
$manifestLines += $componentsText
$manifestLines += '    ) .'

$manifestContent = $manifestLines -join "`n"
$manifestPathTop = Join-Path $BundlePath 'manifest.ttl'
try {
    Set-Content -Path $manifestPathTop -Value $manifestContent -Encoding UTF8 -Force
    Write-Output "Wrote top-level manifest: $manifestPathTop"
} catch {
    Write-Warning ("Failed to write top-level manifest: {0}" -f $_)
}

# Also ensure expected binary exists
$expectedBin = Join-Path $nestedFolder $binaryName
if (Test-Path $expectedBin) {
    Write-Output "Binary present at expected path: $expectedBin"
} else {
    Write-Warning "Binary not present at expected path: $expectedBin"
}

Write-Output "Repair complete. Now running diagnostics..."
# run existing check script if present
$checkScript = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Definition) 'check_installed_vst3.ps1'
if (Test-Path $checkScript) {
    try {
        & powershell -NoProfile -ExecutionPolicy Bypass -File $checkScript
    } catch { Write-Warning ("Failed to run diagnostics: {0}" -f $_) }
} else {
    Write-Output "Diagnostics script not found: $checkScript"
}
