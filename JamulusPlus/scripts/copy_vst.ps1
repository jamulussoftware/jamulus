$src = 'E:\Web stuff\Jamulus VST Wrapper\jamulus\juce-wrapper\build\plugin\jamulus_vst3_artefacts\Release\VST3\JamulusVST3.vst3'
$dstDir = 'C:\Program Files\Common Files\VST3'
if (-not (Test-Path -LiteralPath $dstDir)) { New-Item -ItemType Directory -Path $dstDir -Force | Out-Null }
try {
    Copy-Item -LiteralPath $src -Destination $dstDir -Recurse -Force -ErrorAction Stop
    if (Test-Path -LiteralPath (Join-Path $dstDir 'JamulusVST3.vst3')) {
        Write-Output "COPIED"
        Get-Item -LiteralPath (Join-Path $dstDir 'JamulusVST3.vst3') | Select-Object FullName,Length
    } else {
        Write-Error "COPY_FAILED"
        exit 2
    }
} catch {
    Write-Error $_.Exception.Message
    exit 1
}
