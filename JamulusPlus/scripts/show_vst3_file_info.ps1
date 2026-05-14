param(
    [string]$Path = "C:\Program Files\Common Files\VST3\JamulusVST3.vst3\Contents\x86_64-win\JamulusVST3.vst3"
)

$ErrorActionPreference = 'Stop'
Write-Host "File: $Path"
if (-not (Test-Path $Path)) { Write-Error "Not found: $Path"; exit 1 }
$fi = Get-Item -Path $Path -Force
Write-Host "Length: $($fi.Length)"; Write-Host "Attributes: $($fi.Attributes)"
try {
    $acl = Get-Acl -Path $Path
    Write-Host "Owner: $($acl.Owner)"
    Write-Host "Access:"
    $acl.Access | ForEach-Object { Write-Host ("  {0} - {1} - {2}" -f $_.IdentityReference, $_.FileSystemRights, $_.AccessControlType) }
} catch {
    Write-Warning "Could not get ACL: $_"
}

# Also list directory contents for context
$dir = Split-Path -Parent $Path
Write-Host "Directory: $dir"
Get-ChildItem -Path $dir | ForEach-Object { Write-Host ("  {0} {1}" -f $_.Mode, $_.Name) }