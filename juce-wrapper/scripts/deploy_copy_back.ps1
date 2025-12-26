param(
    [string]$BundleX86 = "C:\Program Files\Common Files\VST3\JamulusVST3.vst3\Contents\x86_64-win\JamulusVST3.vst3"
)
$ErrorActionPreference = 'Stop'
$tempRoot = [IO.Path]::GetTempPath()
$dirs = Get-ChildItem -Path $tempRoot -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -like 'jamulus_windeploy_*' } | Sort-Object LastWriteTime -Descending
if (-not $dirs -or $dirs.Count -eq 0) { Write-Error 'No jamulus_windeploy_* temp folder found'; exit 1 }
$src = $dirs[0].FullName
Write-Host "Using temp folder: $src"
Write-Host "Copying files to bundle folder: $BundleX86"
if (-not (Test-Path $BundleX86)) { Write-Error "Bundle x86 folder not found: $BundleX86"; exit 1 }
try {
    Get-ChildItem -Path $src -Recurse -File | ForEach-Object {
        $rel = $_.FullName.Substring($src.Length).TrimStart('\')
        $dest = Join-Path $BundleX86 $rel
        $destDir = Split-Path -Parent $dest
        if (-not (Test-Path $destDir)) { New-Item -ItemType Directory -Path $destDir | Out-Null }
        Copy-Item -Path $_.FullName -Destination $dest -Force
        Write-Host "Copied: $rel"
    }
    Write-Host "Copy complete."
} catch {
    Write-Error "Copy failed: $_"
    Write-Host "If permission errors occurred, re-run this script as Administrator:"
    Write-Host "powershell -NoProfile -ExecutionPolicy Bypass -File \"$PSCommandPath\" -BundleX86 \"$BundleX86\""
    exit 1
}