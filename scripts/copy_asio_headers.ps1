$sdk='C:\SDKs\ASIOSDK'
$dest='e:\Web stuff\Jamulus VST Wrapper\jamulus\juce-wrapper\libjamulus\include_asio'
New-Item -ItemType Directory -Path $dest -Force | Out-Null
$patterns = @('asio.h','asiosys.h','asiodrivers.h','ASIO*.h','*.h')
foreach ($pat in $patterns) {
    Write-Output "Searching for pattern: $pat"
    $found = Get-ChildItem -Path $sdk -Filter $pat -Recurse -ErrorAction SilentlyContinue
    if ($found) {
        foreach ($f in $found) {
            Write-Output "Copying: $($f.FullName) -> $dest"
            Copy-Item -LiteralPath $f.FullName -Destination $dest -Force -ErrorAction SilentlyContinue
        }
    }
}
Write-Output '--- Files in include_asio ---'
Get-ChildItem -LiteralPath $dest -Recurse | Select-Object FullName | ForEach-Object { Write-Output $_.FullName }

Write-Output '--- Reconfiguring CMake to use include_asio ---'
cmake -S 'juce-wrapper' -B 'juce-wrapper\build' -DQt6_DIR='C:/Qt/6.7.3/msvc2019_64/lib/cmake/Qt6' -DASIO_SDK_DIR='e:/Web stuff/Jamulus VST Wrapper/jamulus/juce-wrapper/libjamulus/include_asio'

Write-Output '--- Building ---'
cmake --build 'juce-wrapper\build' --config Release -- /m
