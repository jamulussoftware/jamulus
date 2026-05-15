
$code = @"
using System;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Collections.Generic;

public class DllLoader {
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr LoadLibrary(string lpFileName);

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool FreeLibrary(IntPtr hModule);
}
"@
Add-Type -TypeDefinition $code

$vstPath = "build\plugin\jamulus_vst3_artefacts\Release\VST3\JamulusVST3.vst3\Contents\x86_64-win\JamulusVST3.vst3"
$vstPathFull = [System.IO.Path]::GetFullPath($vstPath)

Write-Host "Attempting to load: $vstPathFull"

$handle = [DllLoader]::LoadLibrary($vstPathFull)

if ($handle -eq [IntPtr]::Zero) {
    Write-Host "Failed to load library. Error code: $((New-Object System.ComponentModel.Win32Exception([System.Runtime.InteropServices.Marshal]::GetLastWin32Error())).Message)" 
}
else {
    Write-Host "Library loaded successfully."
    
    $process = [System.Diagnostics.Process]::GetCurrentProcess()
    $modules = $process.Modules
    
    Write-Host "`nLoaded Modules (excluding C:\Windows):"
    foreach ($module in $modules) {
        try {
            if ($module.FileName -and -not $module.FileName.StartsWith("C:\Windows", [System.StringComparison]::OrdinalIgnoreCase)) {
                Write-Host $module.FileName
            }
        }
        catch {
            # Ignore access denied etc
        }
    }
    
    [DllLoader]::FreeLibrary($handle) | Out-Null
}
