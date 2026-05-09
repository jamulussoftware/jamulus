param (
    # Replace default path with system Qt installation folder if necessary
    [string]$QtInstallPath32="C:\Qt\5.15.2", [string]$QtInstallPath64="C:\Qt\6.8.1",
    [string]$QtCompile32="msvc2019", [string]$QtCompile64="msvc2022_64",
    # Important:
    # - Do not update ASIO SDK without checking for license-related changes.
    # - Do not copy (parts of) the ASIO SDK into the Jamulus source tree without
    #   further consideration as it would make the license situation more complicated.
    #
    # The following version pinnings are semi-automatically checked for
    # updates. Verify .github/workflows/bump-dependencies.yaml when changing those manually:
    [string]$AsioSDKUrl="https://download.steinberg.net/sdk_downloads/ASIO-SDK_2.3.4_2025-10-15.zip",
    [string]$NsisUrl="https://downloads.sourceforge.net/project/nsis/NSIS%203/3.12/nsis-3.12.zip",
    [string]$BuildOption="",
    # Toggles for debugging and targeted builds
    [switch]$DebugMode, [switch]$Skip64Bit, [switch]$Skip32Bit
)

# Fail early on all errors
$ErrorActionPreference = "Stop"

# Invoke-WebRequest is really slow by default because it renders a progress bar.
# Disabling this, improves vastly performance:
$ProgressPreference = 'SilentlyContinue'

# change directory to the directory above (if needed)
Set-Location -Path "$PSScriptRoot\..\"

# Global constants
$RootPath = "$PWD"
$BuildPath = "$RootPath\build"
$DeployPath = "$RootPath\deploy"
$WindowsPath ="$RootPath\windows"
$AppName = "Jamulus"

Function Write-Log {
    param([Parameter(Mandatory=$true)][AllowNull()][AllowEmptyString()][string]$Message, [ValidateSet("INFO","WARN","ERROR","STEP","DEBUG")][string]$Level="INFO")
    if ($null -eq $Message) { $Message = "" }
    $Stamp = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss"); $Log = "[$Stamp] [$Level] $Message"
    switch ($Level) {
        "INFO"  { Write-Host $Log }
        "STEP"  { Write-Host "`n>>> [$Stamp] $Message" -ForegroundColor Cyan }
        "WARN"  { Write-Host $Log -ForegroundColor Yellow }
        "ERROR" { Write-Host $Log -ForegroundColor Red }
        "DEBUG" { if ($DebugMode) { Write-Host $Log -ForegroundColor DarkGray } }
    }
}

# Execute native command with errorlevel handling
Function Invoke-Native-Command {
    param([Parameter(Mandatory)][string]$Command, [string[]]$Arguments, [switch]$SuppressStdErr)
    Write-Log "Executing: $Command $($Arguments -join ' ')" "DEBUG"
    $Out = [Collections.Generic.List[string]]::new()

    $PrevEA = $ErrorActionPreference; $ErrorActionPreference = "Continue"
    try {
        & $Command @Arguments 2>&1 | ForEach-Object {
            $IsErr = $_ -is [Management.Automation.ErrorRecord]
            if ($IsErr -and $SuppressStdErr) { return }

            $Line = if ($IsErr) { $_.TargetObject -as [string] } else { $_.ToString() }
            if ([string]::IsNullOrWhiteSpace($Line)) { return }

            $Out.Add($Line)
            $Color = if ($IsErr -and $DebugMode) { "DarkYellow" } else { "DarkGray" }
            if ($DebugMode -or -not $IsErr) { Write-Host "    $Line" -ForegroundColor $Color }
        }
    } finally { $ErrorActionPreference = $PrevEA }

    if ($LastExitCode) {
        Write-Log ($Err = "Native command $Command failed (Exit: $LastExitCode)") "ERROR"
        if (-not $DebugMode) { $Out.ForEach({ Write-Log $_ "ERROR" }) }
        throw $Err
    }
}

# Cleanup existing build folders
Function Clean-Build-Environment {
    Write-Log "Cleaning Build Environment..." -Level STEP
    if (Test-Path $BuildPath) { Remove-Item $BuildPath -Recurse -Force }
    if (Test-Path $DeployPath) { Remove-Item $DeployPath -Recurse -Force }
    New-Item $BuildPath, $DeployPath -ItemType Directory | Out-Null
    Write-Log "Build and Deploy directories initialized."
}

# For sourceforge links we need to get the correct mirror (especially NISIS) Thanks: https://www.powershellmagazine.com/2013/01/29/pstip-retrieve-a-redirected-url/
Function Get-RedirectedUrl {
    param([Parameter(Mandatory=$true)][string]$url)
    $sleep = 10; $maxSleep = 80
    for ($i = 1; $i -le 10; $i++) {
        try {
            $req = [System.Net.WebRequest]::Create($url); $req.AllowAutoRedirect = $true
            $res = $req.GetResponse(); $redirect = $res.ResponseUri.AbsoluteUri; $res.Close()
            return $redirect
        } catch {
            if ($i -lt 10) {
                Write-Log "Fetch attempt $i/10 for $url failed, retrying in ${sleep}s" -Level WARN
                Start-Sleep -Seconds $sleep; $sleep = [Math]::Min($sleep * 2, $maxSleep)
                continue
            }
            Write-Log "All 10 fetch attempts for $url failed" -Level ERROR; throw
        }
    }
}

# Download and uncompress dependency in ZIP format
Function Install-Dependency {
    param([Parameter(Mandatory=$true)][string]$Uri, [Parameter(Mandatory=$true)][string]$Destination)
    if (Test-Path "$WindowsPath\$Destination") { Write-Log "Using cached ${WindowsPath}\${Destination}"; return }
    $TempFile = [System.IO.Path]::GetTempFileName() + ".zip"

    # Create a unique empty directory to unpack into
    $TempDir = Join-Path ([System.IO.Path]::GetTempPath()) ([System.Guid]::NewGuid().ToString())
    New-Item -ItemType Directory -Path $TempDir | Out-Null

    if ($Uri -Match "downloads.sourceforge.net") { $Uri = Get-RedirectedUrl $Uri }
    Write-Log "Downloading $Uri..."
    Invoke-WebRequest -Uri $Uri -OutFile $TempFile
    Write-Log "Extracting to $WindowsPath\$Destination..."
    Expand-Archive -Path $TempFile -DestinationPath $TempDir -Force

    Move-Item "$TempDir\*" "$WindowsPath\$Destination" -Force
    Remove-Item $TempDir, $TempFile -Recurse -Force
}

# Install ASIO SDK and NSIS Installer
Function Install-Dependencies {
    Write-Log "Installing Dependencies..." -Level STEP
    Install-Dependency $NsisUrl "..\libs\NSIS\NSIS-source"

    if ($BuildOption -Notmatch "jack") {
        # Don't download ASIO SDK on Jamulus JACK builds to save resources.
        Install-Dependency $AsioSDKUrl "..\libs\ASIOSDK2"
    } else { Write-Log "Skipping ASIO SDK (JACK build detected)." }
}

# Setup environment variables and build tool paths
Function Initialize-Build-Environment {
    param([Parameter(Mandatory=$true)][string]$BuildArch)

    # Use native vswhere.exe to find VS2017+ installations with C++ build tools
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) { Throw "vswhere.exe not found. Visual Studio 2017 or above is required." }

    $VsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $VsVer = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion

    if ([string]::IsNullOrWhiteSpace($VsPath)) { Throw "Could not locate a Visual Studio installation with C++ build tools." }

    # Dynamically determine the correct Platform Toolset based on VS version to avoid hardcoded MSBuild failures
    if ($VsVer -match "^17\.") { Set-Item Env:VsPlatformToolset "v143" }
    elseif ($VsVer -match "^16\.") { Set-Item Env:VsPlatformToolset "v142" }
    else { Set-Item Env:VsPlatformToolset "v141" }

    $VcVars = if ($BuildArch -eq "x86_64") { "$VsPath\VC\Auxiliary\build\vcvars64.bat" } else { "$VsPath\VC\Auxiliary\build\vcvars32.bat" }

    Write-Log "Using VS environment: $VcVars (Toolset: $Env:VsPlatformToolset)"
    if (-not (Test-Path $VcVars)) { Throw "MSVC environment script ($BuildArch) not found at $VcVars" }

    # Import environment variables set by vcvarsXX.bat into current scope
    $EnvDump = [System.IO.Path]::GetTempFileName()
    Invoke-Native-Command "cmd" ("/c", "`"$VcVars`" && set > `"$EnvDump`"")
    Get-Content $EnvDump | Where-Object { $_ -match "^([^=]+)=(.*)$" } | ForEach-Object { Set-Item "Env:$($Matches[1])" $Matches[2] }
    Remove-Item $EnvDump -Force
}

# Resolve Qt path by falling back to Registry lookups if the default path is missing
Function Resolve-Qt-Path {
    param([Parameter(Mandatory=$true)][string]$DefaultPath)
    if (Test-Path $DefaultPath) { return $DefaultPath }
    Write-Log "Qt path '$DefaultPath' not found. Searching registry..." -Level DEBUG
    $QtVer = Split-Path $DefaultPath -Leaf
    $RegPaths = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*", "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*", "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*"
    foreach ($Reg in $RegPaths) {
        $Installs = Get-ItemProperty $Reg -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -match "Qt" -and $_.InstallLocation }
        foreach ($Inst in $Installs) {
            $TryPath = Join-Path $Inst.InstallLocation $QtVer
            if (Test-Path $TryPath) { Write-Log "Discovered Qt at: $TryPath"; return $TryPath }
        }
    }
    Write-Log "Could not locate Qt $QtVer in registry." -Level WARN; return $DefaultPath
}

# Setup Qt environment variables and build tool paths
Function Initialize-Qt-Build-Environment {
    param([Parameter(Mandatory=$true)][string]$QtInstallPath, [Parameter(Mandatory=$true)][string]$QtCompile)
    $QtBin = "$QtInstallPath\$QtCompile\bin"
    # Setup Qt executables paths for later calls
    Set-Item Env:QtQmakePath "$QtBin\qmake.exe"; Set-Item Env:QtWinDeployPath "$QtBin\windeployqt.exe"

    if (Get-Command "jom.exe" -ErrorAction SilentlyContinue) { Set-Item Env:QtJomPath "jom.exe" }
    else {
        $QtRoot = Split-Path $QtInstallPath -Parent
        $JomExes = "$QtRoot\Tools\QtCreator\bin\jom\jom.exe", "$QtRoot\Tools\jom\jom.exe", "C:\Qt\Tools\QtCreator\bin\jom\jom.exe", "D:\Qt\Tools\QtCreator\bin\jom\jom.exe"
        $FoundJom = $JomExes | Where-Object { Test-Path $_ } | Select-Object -First 1
        if ($FoundJom) { Write-Log "Discovered jom: $FoundJom"; Set-Item Env:QtJomPath $FoundJom } else { Set-Item Env:QtJomPath "" }
    }
    Write-Log "Using Qt binaries: $QtBin"
    if (-not (Test-Path $Env:QtQmakePath)) { Throw "Qt MSVC binaries not found at $QtBin" }
}

# Build Jamulus x86_64 and x86
Function Build-App {
    param([Parameter(Mandatory=$true)][string]$Cfg, [Parameter(Mandatory=$true)][string]$Arch)
    Write-Log "Building App ($Arch) config: $Cfg"

    $QmkCfg = "CONFIG+=$Cfg $Arch $BuildOption"
    if (-not $DebugMode) { $QmkCfg += " silent" }

    Invoke-Native-Command $Env:QtQmakePath ("$RootPath\$AppName.pro", $QmkCfg, "-o", "$BuildPath\Makefile")

    Set-Location $BuildPath

    # Add /NOLOGO to stop the Microsoft header from being generated
    $MkArgs = @("/NOLOGO", $Cfg)
    if (-not $DebugMode) { $MkArgs += "/S" }

    if ($Env:QtJomPath) {
        $Cores = [Math]::Max(1, [Math]::Floor([int]$Env:NUMBER_OF_PROCESSORS / 2))
        Write-Log "Building with jom /J $Cores (half of $Env:NUMBER_OF_PROCESSORS cores)"
        Invoke-Native-Command $Env:QtJomPath (@("/J", "$Cores") + $MkArgs)
    } else {
        Write-Log "Building with nmake (sequential)" -Level WARN
        Invoke-Native-Command "nmake" $MkArgs
    }

    Write-Log "Deploying Qt dependencies..."
    Invoke-Native-Command $Env:QtWinDeployPath ("--$Cfg", "--compiler-runtime", "--dir=$DeployPath\$Arch", "$BuildPath\$Cfg\$AppName.exe")
    Move-Item "$BuildPath\$Cfg\$AppName.exe" "$DeployPath\$Arch" -Force

    # Use the new switch to drop the 'Could Not Find' stderr stream entirely
    Invoke-Native-Command "nmake" (@("clean") + $MkArgs) -SuppressStdErr

    Set-Location $RootPath
}

# Build and deploy Jamulus 64bit and 32bit variants
function Build-App-Variants {
    Write-Log "Starting Application Builds" -Level STEP

    $ArchsToBuild = @()
    if (-not $Skip64Bit) { $ArchsToBuild += "x86_64" } else { Write-Log "Skipping 64-bit build (-Skip64Bit used)." -Level INFO }
    if (-not $Skip32Bit) { $ArchsToBuild += "x86" } else { Write-Log "Skipping 32-bit build (-Skip32Bit used)." -Level INFO }

    if ($ArchsToBuild.Count -eq 0) {
        Write-Log "All application builds skipped." -Level WARN
        return
    }

    foreach ($Arch in $ArchsToBuild) {
        Write-Log "Configuring environment for $Arch..."
        $OrigEnv = Get-ChildItem Env:

        try {
            $Path = if ($Arch -eq "x86") { $QtInstallPath32 } else { $QtInstallPath64 }
            $Comp = if ($Arch -eq "x86") { $QtCompile32 } else { $QtCompile64 }
            Initialize-Build-Environment $Arch
            Initialize-Qt-Build-Environment (Resolve-Qt-Path $Path) $Comp
            Build-App "release" $Arch
        } catch {
            Write-Log "Build failed for ${Arch}: $_" -Level WARN
        } finally {
            # Ensure the environment is reset before the next loop, even if this arch fails
            $OrigEnv | ForEach-Object { Set-Item "Env:$($_.Name)" $_.Value }
        }
    }
}

# Build Windows installer
Function Build-Installer {
    param([string]$BuildOption)
    Write-Log "Building Windows Installer" -Level STEP

    # --- SAFETY GATE: Do not build if no executables were generated ---
    $Has64 = Test-Path "$DeployPath\x86_64\$AppName.exe"
    $Has32 = Test-Path "$DeployPath\x86\$AppName.exe"

    if (-not $Has64 -and -not $Has32) {
        Write-Log "No built binaries found in $DeployPath. Skipping installer to avoid creating an empty package." -Level ERROR
        Throw "Installer build aborted: No application binaries found."
    }

    $Ver = (Get-Content "$RootPath\$AppName.pro" | Where-Object { $_ -match "^VERSION *= *(.*)$" } | ForEach-Object { $Matches[1] } | Select-Object -First 1)

    $NsisVerb = if ($DebugMode) { "/v4" } else { "/v2" }
    $NsisArgs = @($NsisVerb, "/DAPP_NAME=$AppName", "/DAPP_VERSION=$Ver", "/DROOT_PATH=$RootPath", "/DWINDOWS_PATH=$WindowsPath", "/DDEPLOY_PATH=$DeployPath")

    if ($BuildOption) { $NsisArgs += "/DBUILD_OPTION=$BuildOption" }
    Invoke-Native-Command "$RootPath\libs\NSIS\NSIS-source\makensis" (@($NsisArgs) + "$WindowsPath\installer.nsi")
}

# Build and copy NS-Process dll
Function Build-NSProcess {
    if (Test-Path "$RootPath\libs\NSIS\nsProcess.dll") { Write-Log "nsProcess.dll exists, skipping."; return }
    Write-Log "Building nsProcess.dll..." -Level STEP
    $OrigEnv = Get-ChildItem Env:; Initialize-Build-Environment "x86"

    # Force msbuild to retarget the solution dynamically to match installed toolchain
    Invoke-Native-Command "msbuild" ("$RootPath\libs\NSIS\nsProcess\nsProcess.sln", "/p:Configuration=Release UNICODE", "/p:Platform=Win32", "/p:PlatformToolset=$Env:VsPlatformToolset")

    Move-Item "$RootPath\libs\NSIS\nsProcess\Release\nsProcess.dll" "$RootPath\libs\NSIS\nsProcess.dll" -Force
    Remove-Item "$RootPath\libs\NSIS\nsProcess\Release\" -Force -Recurse
    $OrigEnv | ForEach-Object { Set-Item "Env:$($_.Name)" $_.Value }
}

try {
    Clean-Build-Environment; Install-Dependencies; Build-App-Variants
    Build-NSProcess; Build-Installer $BuildOption
    Write-Log "Build process completed successfully." -Level STEP
} catch {
    Write-Log "Pipeline failed: $_" -Level ERROR; exit 1
}
