Set-Location -LiteralPath "$PSScriptRoot/.."

$jackZip = Join-Path $PWD 'jack2-master.zip'
$url = 'https://github.com/jackaudio/jack2/archive/refs/heads/master.zip'
Write-Output "Downloading jack2 from $url to $jackZip"
Invoke-WebRequest -Uri $url -OutFile $jackZip -UseBasicParsing

$extractDir = Join-Path $PWD 'jack2-src'
if (Test-Path $extractDir) { Remove-Item -Recurse -Force $extractDir }
Write-Output "Extracting $jackZip to $extractDir"
Expand-Archive -Force $jackZip -DestinationPath $extractDir

$dest = Join-Path $PWD 'libjamulus\include_jack'
New-Item -ItemType Directory -Path $dest -Force | Out-Null
Write-Output "Copying header files to $dest"
Get-ChildItem -Path $extractDir -Include '*.h' -Recurse -ErrorAction SilentlyContinue | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $dest -Force }

# Ensure unistd.h stub exists in include_stub
$stubDir = Join-Path $PWD 'libjamulus\include_stub'
New-Item -ItemType Directory -Path $stubDir -Force | Out-Null
$unistdPath = Join-Path $stubDir 'unistd.h'
if (-not (Test-Path $unistdPath)) {
    @"
#pragma once
// Minimal unistd.h stub for Windows to satisfy upstream Unix-only includes
#ifndef _UNISTD_H_
#define _UNISTD_H_
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#define sleep(x) Sleep(x)
#endif
"@ | Out-File -Encoding ascii -FilePath $unistdPath
    Write-Output "Wrote stub: $unistdPath"
} else {
    Write-Output "Stub already exists: $unistdPath"
}

# Reconfigure CMake to use the local ASIO and JACK headers
$qtDir = 'C:/Qt/6.7.3/msvc2019_64/lib/cmake/Qt6'
$asioInclude = 'e:/Web stuff/Jamulus VST Wrapper/jamulus/juce-wrapper/libjamulus/include_asio'
Write-Output "Running CMake configure with ASIO: $asioInclude"
cmake -S . -B build -DQt6_DIR="$qtDir" -DASIO_SDK_DIR="$asioInclude"

Write-Output 'Building...'
cmake --build build --config Release -- /m
