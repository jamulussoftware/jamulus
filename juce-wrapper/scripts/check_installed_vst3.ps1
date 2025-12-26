$path='C:\Program Files\Common Files\VST3\JamulusVST3.vst3'
if (-not (Test-Path $path)) { Write-Output "Bundle not found: $path"; exit 0 }
Write-Output "--- Files ---"
Get-ChildItem -LiteralPath $path -Recurse | Select-Object FullName,Length | Format-List

$manifest = Join-Path $path 'manifest.ttl'
if (Test-Path $manifest) { Write-Output '--- manifest.ttl ---'; Get-Content $manifest -Raw } else { Write-Output 'No top-level manifest.ttl' }

$bin = Join-Path $path 'Contents\x86_64-win\JamulusVST3.dll'
if (Test-Path $bin) { Write-Output '--- Binary Info ---'; Get-Item $bin | Format-List FullName,Length } else { Write-Output "Binary not found at expected path: $bin" }

if (Test-Path $bin) {
    try {
        $fs=[System.IO.File]::OpenRead($bin)
        $br=New-Object System.IO.BinaryReader($fs)
        $fs.Seek(0x3C,'Begin') | Out-Null
        $e_lfanew=$br.ReadInt32()
        $fs.Seek($e_lfanew+4,'Begin') | Out-Null
        $machine=$br.ReadUInt16()
        $br.Close(); $fs.Close()
        Write-Output ('--- PE Machine ---')
        Write-Output ('PE Machine: 0x{0:X4}' -f $machine)
        switch ($machine) { 0x8664 {Write-Output 'x64 (AMD64)'} 0x014c {Write-Output 'x86 (Intel)'} default {Write-Output 'Unknown'} }
    } catch {
        Write-Output "Failed to read PE header: $_"
    }
}
