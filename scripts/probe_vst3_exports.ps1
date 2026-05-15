param(
    [Parameter(Mandatory=$true)][string]$DllPath,
    [switch]$AddDllDir
)

function Fail([string]$msg){ Write-Error $msg; exit 1 }

if (-not (Test-Path $DllPath)) { Fail "DLL not found: $DllPath" }

$signature = @'
using System;
using System.Runtime.InteropServices;

public static class Kernel32
{
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
    public static extern IntPtr LoadLibraryW(string lpFileName);

    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
    public static extern IntPtr LoadLibraryExW(string lpFileName, IntPtr hFile, uint dwFlags);

    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern bool FreeLibrary(IntPtr hModule);

    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Ansi)]
    public static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern bool SetDefaultDllDirectories(uint DirectoryFlags);

    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
    public static extern IntPtr AddDllDirectory(string NewDirectory);
}
'@

Add-Type -TypeDefinition $signature -ErrorAction Stop | Out-Null

Write-Output "Probing DLL: $DllPath"

if ($AddDllDir) {
    $dir = Split-Path -Parent $DllPath
    Write-Output "Adding DLL directory: $dir"
    # LOAD_LIBRARY_SEARCH_DEFAULT_DIRS = 0x00001000
    [void][Kernel32]::SetDefaultDllDirectories(0x00001000)
    # Add the plugin folder so its Qt6*.dll etc can be resolved
    [void][Kernel32]::AddDllDirectory($dir)
}

$h = if ($AddDllDir) {
    # LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR (0x00000100) | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS (0x00001000)
    [Kernel32]::LoadLibraryExW($DllPath, [IntPtr]::Zero, 0x00000100 -bor 0x00001000)
} else {
    [Kernel32]::LoadLibraryW($DllPath)
}
if ($h -eq [IntPtr]::Zero) {
    $err = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
    Fail ("LoadLibraryW failed. Win32Error={0}" -f $err)
}

try {
    $p = [Kernel32]::GetProcAddress($h, 'GetPluginFactory')
    if ($p -eq [IntPtr]::Zero) {
        Write-Output "GetPluginFactory export: MISSING (this is likely NOT a valid VST3 module)"
        exit 2
    }

    Write-Output ("GetPluginFactory export: OK at 0x{0}" -f $p.ToString('X'))
    Write-Output "Result: DLL loads and exposes VST3 factory entrypoint."
    exit 0
}
finally {
    [Kernel32]::FreeLibrary($h) | Out-Null
}
