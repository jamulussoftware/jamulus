param (
    # Replace default path with system Qt installation folder if necessary
    [string] $QtInstallPath32 = "C:\Qt\5.15.2",
    [string] $QtInstallPath64 = "C:\Qt\6.8.1",
    [string] $QtCompile32 = "msvc2019",
    [string] $QtCompile64 = "msvc2022_64",
    # Important:
    # - Do not update ASIO SDK without checking for license-related changes.
    # - Do not copy (parts of) the ASIO SDK into the Jamulus source tree without
    #   further consideration as it would make the license situation more complicated.
    #
    # The following version pinnings are semi-automatically checked for
    # updates. Verify .github/workflows/bump-dependencies.yaml when changing those manually:
    [string] $AsioSDKUrl = "https://download.steinberg.net/sdk_downloads/ASIO-SDK_2.3.4_2025-10-15.zip",
    [string] $NsisUrl = "https://downloads.sourceforge.net/project/nsis/NSIS%203/3.12/nsis-3.12.zip",
    [string] $BuildOption = "",
    # Toggles for debugging and targeted builds
    [switch] $DebugMode,
    [switch] $Skip64Bit,
    [switch] $Skip32Bit
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

Function Write-Log
{
    param(
        [Parameter(Mandatory=$true)]
        [AllowNull()]
        [AllowEmptyString()]
        [string] $Message,
        [ValidateSet("INFO","WARN","ERROR","STEP","DEBUG")]
        [string] $Level = "INFO"
    )

    if ($null -eq $Message) { $Message = "" }
    $Stamp = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")
    $Log = "[$Stamp] [$Level] $Message"

    switch ($Level) {
        "INFO"  { Write-Host $Log }
        "STEP"  { Write-Host "`n>>> [$Stamp] $Message" -ForegroundColor Cyan }
        "WARN"  { Write-Host $Log -ForegroundColor Yellow }
        "ERROR" { Write-Host $Log -ForegroundColor Red }
        "DEBUG" { if ($DebugMode) { Write-Host $Log -ForegroundColor DarkGray } }
    }
}

# Execute native command with errorlevel handling
Function Invoke-Native-Command
{
    param(
        [Parameter(Mandatory=$true)]
        [string] $Command,
        [string[]] $Arguments,
        [switch] $SuppressStdErr
    )

    Write-Log "Executing: $Command $($Arguments -join ' ')" "DEBUG"
    $Out = [Collections.Generic.List[string]]::new()

    $PrevEA = $ErrorActionPreference
    $ErrorActionPreference = "Continue"

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
    } finally {
        $ErrorActionPreference = $PrevEA
    }

    if ($LastExitCode) {
        $Err = "Native command $Command failed (Exit: $LastExitCode)"
        Write-Log $Err "ERROR"
        if (-not $DebugMode) {
            $Out.ForEach({ Write-Log $_ "ERROR" })
        }
        throw $Err
    }
}

# Cleanup existing build folders
Function Clean-Build-Environment
{
    Write-Log "Cleaning Build Environment..." "STEP"

    if (Test-Path -Path $BuildPath) { Remove-Item -Path $BuildPath -Recurse -Force }
    if (Test-Path -Path $DeployPath) { Remove-Item -Path $DeployPath -Recurse -Force }

    New-Item -Path $BuildPath -ItemType Directory | Out-Null
    New-Item -Path $DeployPath -ItemType Directory | Out-Null

    Write-Log "Build and Deploy directories initialized." "INFO"
}

# For sourceforge links we need to get the correct mirror (especially NSIS) Thanks: https://www.powershellmagazine.com/2013/01/29/pstip-retrieve-a-redirected-url/
Function Get-RedirectedUrl
{
    param(
        [Parameter(Mandatory=$true)]
        [string] $url
    )

    $sleep = 10
    $maxSleep = 80

    for ($i = 1; $i -le 10; $i++) {
        try {
            $req = [System.Net.WebRequest]::Create($url)
            $req.AllowAutoRedirect = $true
            $res = $req.GetResponse()
            $redirect = $res.ResponseUri.AbsoluteUri
            $res.Close()
            return $redirect
        } catch {
            if ($i -lt 10) {
                Write-Log "Fetch attempt $i/10 for $url failed, retrying in ${sleep}s" "WARN"
                Start-Sleep -Seconds $sleep
                $sleep = [Math]::Min($sleep * 2, $maxSleep)
                continue
            }
            Write-Log "All 10 fetch attempts for $url failed" "ERROR"
            throw
        }
    }
}

# Download and uncompress dependency in ZIP format
Function Install-Dependency
{
    param(
        [Parameter(Mandatory=$true)]
        [string] $Uri,
        [Parameter(Mandatory=$true)]
        [string] $Destination
    )

    if (Test-Path -Path "$WindowsPath\$Destination")
    {
        Write-Log "Using cached ${WindowsPath}\${Destination}" "INFO"
        return
    }

    $TempFile = [System.IO.Path]::GetTempFileName() + ".zip"

    # Create a unique empty directory to unpack into
    $TempDir = Join-Path ([System.IO.Path]::GetTempPath()) ([System.Guid]::NewGuid().ToString())
    New-Item -ItemType Directory -Path $TempDir | Out-Null

    if ($Uri -Match "downloads.sourceforge.net")
    {
        $Uri = Get-RedirectedUrl -url $Uri
    }

    Write-Log "Downloading $Uri..." "INFO"
    Invoke-WebRequest -Uri $Uri -OutFile $TempFile

    Write-Log "Extracting to $WindowsPath\$Destination..." "INFO"
    Expand-Archive -Path $TempFile -DestinationPath $TempDir -Force

    Move-Item -Path "$TempDir\*" -Destination "$WindowsPath\$Destination" -Force
    Remove-Item -Path $TempDir -Recurse -Force
    Remove-Item -Path $TempFile -Force
}

# Install ASIO SDK and NSIS Installer
Function Install-Dependencies
{
    Write-Log "Installing Dependencies..." "STEP"
    Install-Dependency -Uri $NsisUrl -Destination "..\libs\NSIS\NSIS-source"

    if ($BuildOption -Notmatch "jack") {
        # Don't download ASIO SDK on Jamulus JACK builds to save resources.
        Install-Dependency -Uri $AsioSDKUrl -Destination "..\libs\ASIOSDK2"
    } else {
        Write-Log "Skipping ASIO SDK (JACK build detected)." "INFO"
    }
}

# Setup environment variables and build tool paths
Function Initialize-Build-Environment
{
    param(
        [Parameter(Mandatory=$true)]
        [string] $BuildArch
    )

    # Use native vswhere.exe to find VS2017+ installations with C++ build tools
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -Path $vswhere))
    {
        Throw "vswhere.exe not found. Visual Studio 2017 or above is required."
    }

    $VsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $VsVer = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion

    if ([string]::IsNullOrWhiteSpace($VsPath))
    {
        Throw "Could not locate a Visual Studio installation with C++ build tools."
    }

    # Dynamically determine the correct Platform Toolset based on VS version to avoid hardcoded MSBuild failures
    if ($VsVer -match "^17\.") { Set-Item Env:VsPlatformToolset "v143" }
    elseif ($VsVer -match "^16\.") { Set-Item Env:VsPlatformToolset "v142" }
    else { Set-Item Env:VsPlatformToolset "v141" }

    if ($BuildArch -eq "x86_64")
    {
        $VcVarsBin = "$VsPath\VC\Auxiliary\build\vcvars64.bat"
    }
    else
    {
        $VcVarsBin = "$VsPath\VC\Auxiliary\build\vcvars32.bat"
    }

    Write-Log "Using VS environment: $VcVarsBin (Toolset: $Env:VsPlatformToolset)" "INFO"

    if (-not (Test-Path -Path $VcVarsBin))
    {
        Throw "MSVC environment script ($BuildArch) not found at $VcVarsBin"
    }

    # Import environment variables set by vcvarsXX.bat into current scope
    $EnvDump = [System.IO.Path]::GetTempFileName()
    Invoke-Native-Command -Command "cmd" -Arguments ("/c", "`"$VcVarsBin`" && set > `"$EnvDump`"")

    foreach ($_ in Get-Content -Path $EnvDump)
    {
        if ($_ -match "^([^=]+)=(.*)$")
        {
            Set-Item "Env:$($Matches[1])" $Matches[2]
        }
    }

    Remove-Item -Path $EnvDump -Force
}

# Resolve Qt path by falling back to Registry lookups if the default path is missing
Function Resolve-Qt-Path
{
    param(
        [Parameter(Mandatory=$true)]
        [string] $DefaultPath
    )

    if (Test-Path -Path $DefaultPath) { return $DefaultPath }

    Write-Log "Qt path '$DefaultPath' not found. Searching registry..." "DEBUG"
    $QtVer = Split-Path $DefaultPath -Leaf
    $RegPaths = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*",
                "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*",
                "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*"

    foreach ($Reg in $RegPaths)
    {
        $Installs = Get-ItemProperty $Reg -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -match "Qt" -and $_.InstallLocation }
        foreach ($Inst in $Installs)
        {
            $TryPath = Join-Path $Inst.InstallLocation $QtVer
            if (Test-Path -Path $TryPath)
            {
                Write-Log "Discovered Qt at: $TryPath" "INFO"
                return $TryPath
            }
        }
    }

    Write-Log "Could not locate Qt $QtVer in registry." "WARN"
    return $DefaultPath
}

# Setup Qt environment variables and build tool paths
Function Initialize-Qt-Build-Environment
{
    param(
        [Parameter(Mandatory=$true)]
        [string] $QtInstallPath,
        [Parameter(Mandatory=$true)]
        [string] $QtCompile
    )

    $QtMsvcSpecPath = "$QtInstallPath\$QtCompile\bin"

    # Setup Qt executables paths for later calls
    Set-Item Env:QtQmakePath "$QtMsvcSpecPath\qmake.exe"
    Set-Item Env:QtWinDeployPath "$QtMsvcSpecPath\windeployqt.exe"

    if (Get-Command "jom.exe" -ErrorAction SilentlyContinue)
    {
        Set-Item Env:QtJomPath "jom.exe"
    }
    else
    {
        $QtRoot = Split-Path $QtInstallPath -Parent
        $JomExes = "$QtRoot\Tools\QtCreator\bin\jom\jom.exe",
                   "$QtRoot\Tools\jom\jom.exe",
                   "C:\Qt\Tools\QtCreator\bin\jom\jom.exe",
                   "D:\Qt\Tools\QtCreator\bin\jom\jom.exe"

        $FoundJom = $JomExes | Where-Object { Test-Path -Path $_ } | Select-Object -First 1
        if ($FoundJom)
        {
            Write-Log "Discovered jom: $FoundJom" "INFO"
            Set-Item Env:QtJomPath $FoundJom
        }
        else
        {
            Set-Item Env:QtJomPath ""
        }
    }

    Write-Log "Using Qt binaries: $QtMsvcSpecPath" "INFO"

    if (-not (Test-Path -Path $Env:QtQmakePath))
    {
        Throw "Qt MSVC binaries not found at $QtMsvcSpecPath"
    }
}

# Build Jamulus x86_64 and x86
Function Build-App
{
    param(
        [Parameter(Mandatory=$true)]
        [string] $BuildConfig,
        [Parameter(Mandatory=$true)]
        [string] $BuildArch
    )

    Write-Log "Building App ($BuildArch) config: $BuildConfig" "INFO"

    $QmkCfg = "CONFIG+=$BuildConfig $BuildArch $BuildOption"
    if (-not $DebugMode) { $QmkCfg += " silent" }

    Invoke-Native-Command -Command "$Env:QtQmakePath" `
        -Arguments ("$RootPath\$AppName.pro", $QmkCfg, "-o", "$BuildPath\Makefile")

    Set-Location -Path $BuildPath

    # Add /NOLOGO to stop the Microsoft header from being generated
    $MkArgs = @("/NOLOGO", $BuildConfig)
    if (-not $DebugMode) { $MkArgs += "/S" }

    if ($Env:QtJomPath)
    {
        $Cores = [Math]::Max(1, [Math]::Floor([int]$Env:NUMBER_OF_PROCESSORS / 2))
        Write-Log "Building with jom /J $Cores (half of $Env:NUMBER_OF_PROCESSORS cores)" "INFO"
        Invoke-Native-Command -Command "$Env:QtJomPath" -Arguments (@("/J", "$Cores") + $MkArgs)
    }
    else
    {
        Write-Log "Building with nmake (sequential)" "WARN"
        Invoke-Native-Command -Command "nmake" -Arguments $MkArgs
    }

    Write-Log "Deploying Qt dependencies..." "INFO"
    Invoke-Native-Command -Command "$Env:QtWinDeployPath" `
        -Arguments ("--$BuildConfig", "--compiler-runtime", "--dir=$DeployPath\$BuildArch", "$BuildPath\$BuildConfig\$AppName.exe")

    Move-Item -Path "$BuildPath\$BuildConfig\$AppName.exe" -Destination "$DeployPath\$BuildArch" -Force

    # Use the new switch to drop the 'Could Not Find' stderr stream entirely
    Invoke-Native-Command -Command "nmake" -Arguments (@("clean") + $MkArgs) -SuppressStdErr

    Set-Location -Path $RootPath
}

# Build and deploy Jamulus 64bit and 32bit variants
function Build-App-Variants
{
    Write-Log "Starting Application Builds" "STEP"

    $ArchsToBuild = @()
    if (-not $Skip64Bit) { $ArchsToBuild += "x86_64" } else { Write-Log "Skipping 64-bit build (-Skip64Bit used)." "INFO" }
    if (-not $Skip32Bit) { $ArchsToBuild += "x86" } else { Write-Log "Skipping 32-bit build (-Skip32Bit used)." "INFO" }

    if ($ArchsToBuild.Count -eq 0)
    {
        Write-Log "All application builds skipped." "WARN"
        return
    }

    foreach ($Arch in $ArchsToBuild)
    {
        Write-Log "Configuring environment for $Arch..." "INFO"
        $OriginalEnv = Get-ChildItem Env:

        try {
            if ($Arch -eq "x86")
            {
                $Path = $QtInstallPath32
                $Comp = $QtCompile32
            }
            else
            {
                $Path = $QtInstallPath64
                $Comp = $QtCompile64
            }

            Initialize-Build-Environment -BuildArch $Arch
            Initialize-Qt-Build-Environment -QtInstallPath (Resolve-Qt-Path -DefaultPath $Path) -QtCompile $Comp
            Build-App -BuildConfig "release" -BuildArch $Arch
        } catch {
            Write-Log "Build failed for ${Arch}: $_" "WARN"
        } finally {
            # Ensure the environment is reset before the next loop, even if this arch fails
            $OriginalEnv | ForEach-Object { Set-Item "Env:$($_.Name)" $_.Value }
        }
    }
}

# Build Windows installer
Function Build-Installer
{
    param(
        [string] $BuildOption
    )

    Write-Log "Building Windows Installer" "STEP"

    # --- SAFETY GATE: Do not build if no executables were generated ---
    $Has64 = Test-Path -Path "$DeployPath\x86_64\$AppName.exe"
    $Has32 = Test-Path -Path "$DeployPath\x86\$AppName.exe"

    if (-not $Has64 -and -not $Has32)
    {
        Write-Log "No built binaries found in $DeployPath. Skipping installer to avoid creating an empty package." "ERROR"
        Throw "Installer build aborted: No application binaries found."
    }

    foreach ($_ in Get-Content -Path "$RootPath\$AppName.pro")
    {
        if ($_ -Match "^VERSION *= *(.*)$")
        {
            $AppVersion = $Matches[1]
            break
        }
    }

    $NsisVerb = if ($DebugMode) { "/v4" } else { "/v2" }
    $NsisArgs = @($NsisVerb, "/DAPP_NAME=$AppName", "/DAPP_VERSION=$AppVersion", `
        "/DROOT_PATH=$RootPath", "/DWINDOWS_PATH=$WindowsPath", "/DDEPLOY_PATH=$DeployPath")

    if ($BuildOption -ne "")
    {
        $NsisArgs += "/DBUILD_OPTION=$BuildOption"
    }

    Invoke-Native-Command -Command "$RootPath\libs\NSIS\NSIS-source\makensis" `
        -Arguments (@($NsisArgs) + "$WindowsPath\installer.nsi")
}

# Build and copy NS-Process dll
Function Build-NSProcess
{
    if (-not (Test-Path -Path "$RootPath\libs\NSIS\nsProcess.dll"))
    {
        Write-Log "Building nsProcess.dll..." "STEP"

        $OriginalEnv = Get-ChildItem Env:
        Initialize-Build-Environment -BuildArch "x86"

        # Force msbuild to retarget the solution dynamically to match installed toolchain
        Invoke-Native-Command -Command "msbuild" `
            -Arguments ("$RootPath\libs\NSIS\nsProcess\nsProcess.sln", '/p:Configuration="Release UNICODE"', `
            "/p:Platform=Win32", "/p:PlatformToolset=$Env:VsPlatformToolset")

        Move-Item -Path "$RootPath\libs\NSIS\nsProcess\Release\nsProcess.dll" -Destination "$RootPath\libs\NSIS\nsProcess.dll" -Force
        Remove-Item -Path "$RootPath\libs\NSIS\nsProcess\Release\" -Force -Recurse

        $OriginalEnv | ForEach-Object { Set-Item "Env:$($_.Name)" $_.Value }
    }
    else
    {
        Write-Log "nsProcess.dll exists, skipping." "INFO"
    }
}

try {
    Clean-Build-Environment
    Install-Dependencies
    Build-App-Variants
    Build-NSProcess
    Build-Installer -BuildOption $BuildOption
    Write-Log "Build process completed successfully." "STEP"
} catch {
    Write-Log "Pipeline failed: $_" "ERROR"
    exit 1
}
