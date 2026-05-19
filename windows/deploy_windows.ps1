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
    [switch] $DebugMode
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
    param(
        [Parameter(Position=0)]
        [ValidateSet("INFO","WARN","ERROR","STEP","DEBUG","NATIVE","NATIVE-ERR")]
        [string] $Level = "INFO",

        [Parameter(Position=1, Mandatory=$true)]
        [AllowNull()]
        [AllowEmptyString()]
        [string] $Message
    )

    if ($null -eq $Message) { $Message = "" }
    $Stamp = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")
    $Log = "[$Stamp] [$Level] $Message"

    switch ($Level) {
        "INFO"       { Write-Host $Log }
        "STEP"       { Write-Host "`n>>> [$Stamp] $Message" -ForegroundColor Cyan }
        "WARN"       { Write-Host $Log -ForegroundColor Yellow }
        "ERROR"      { Write-Host $Log -ForegroundColor Red }
        "DEBUG"      { if ($DebugMode) { Write-Host $Log -ForegroundColor DarkGray } }
        "NATIVE"     { Write-Host "    $Message" -ForegroundColor DarkGray }
        "NATIVE-ERR" { Write-Host "    $Message" -ForegroundColor DarkYellow }
    }
}

# Execute native command with errorlevel handling
Function Invoke-Native-Command {
    param(
        [string] $Command,
        [string[]] $Arguments,
        [switch] $SuppressStdErr
    )

    Write-Log "DEBUG" "Executing: $Command $($Arguments -join ' ')"
    $Out = [Collections.Generic.List[string]]::new()

    $PrevEA = $ErrorActionPreference
    $ErrorActionPreference = "Continue"

    try {
        & "$Command" @Arguments 2>&1 | ForEach-Object {
            $IsErr = $_ -is [Management.Automation.ErrorRecord]
            if ($IsErr -and $SuppressStdErr) { return }

            $Line = if ($IsErr) { $_.TargetObject -as [string] } else { $_.ToString() }
            if ([string]::IsNullOrWhiteSpace($Line)) { return }

            $Out.Add($Line)

            if ($IsErr) {
                Write-Log "NATIVE-ERR" $Line
            } else {
                Write-Log "NATIVE" $Line
            }
        }
    } finally {
        $ErrorActionPreference = $PrevEA
    }

    if ($LastExitCode -Ne 0) {
        $Err = "Native command $Command returned with exit code $LastExitCode"
        Write-Log "ERROR" $Err
        if (-not $DebugMode) {
            $Out.ForEach({ Write-Log "ERROR" $_ })
        }
        Throw $Err
    }
}

# Cleanup existing build folders
Function Clean-Build-Environment
{
    if (Test-Path -Path $BuildPath) { Remove-Item -Path $BuildPath -Recurse -Force }
    if (Test-Path -Path $DeployPath) { Remove-Item -Path $DeployPath -Recurse -Force }

    New-Item -Path $BuildPath -ItemType Directory
    New-Item -Path $DeployPath -ItemType Directory
}

# For sourceforge links we need to get the correct mirror (especially NISIS) Thanks: https://www.powershellmagazine.com/2013/01/29/pstip-retrieve-a-redirected-url/
Function Get-RedirectedUrl {
    param(
        [Parameter(Mandatory=$true)]
        [string] $url
    )

    $numAttempts = 10
    $sleepTime = 10
    $maxSleepTime = 80
    for ($attempt = 1; $attempt -le $numAttempts; $attempt++) {
        try {
            $request = [System.Net.WebRequest]::Create($url)
            $request.AllowAutoRedirect=$true
            $response=$request.GetResponse()
            $response.ResponseUri.AbsoluteUri
            $response.Close()
            return
        } catch {
            if ($attempt -lt $numAttempts) {
                Write-Log "WARN" "Caught error: $_"
                Write-Log "WARN" "Get-RedirectedUrl: Fetch attempt #${attempt}/${numAttempts} for $url failed, trying again in ${sleepTime}s"
                Start-Sleep -Seconds $sleepTime
                $sleepTime = [Math]::Min($sleepTime * 2, $maxSleepTime)
                continue
            }
            Write-Log "ERROR" "Get-RedirectedUrl: All ${numAttempts} fetch attempts for $url failed, failing whole call"
            throw
        }
    }
}

function Initialize-Module-Here ($m) { # see https://stackoverflow.com/a/51692402

    # If module is imported say that and do nothing
    if (Get-Module | Where-Object {$_.Name -eq $m}) {
        Write-Log "INFO" "Module $m is already imported."
    }
    else {

        # If module is not imported, but available on disk then import
        if (Get-Module -ListAvailable | Where-Object {$_.Name -eq $m}) {
            Import-Module $m
        }
        else {

            # If module is not imported, not available on disk, but is in online gallery then install and import
            if (Find-Module -Name $m | Where-Object {$_.Name -eq $m}) {
                Install-Module -Name $m -Force -Verbose -Scope CurrentUser
                Import-Module $m
            }
            else {

                # If module is not imported, not available and not in online gallery then abort
                Write-Log "ERROR" "Module $m not imported, not available and not in online gallery, exiting."
                EXIT 1
            }
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
        Write-Log "INFO" "Using ${WindowsPath}\${Destination} from previous run (e.g. actions/cache)"
        return
    }

    $TempFileName = [System.IO.Path]::GetTempFileName() + ".zip"
    $TempPath = [System.IO.Path]::GetTempPath()
    $TempGuid = [System.Guid]::NewGuid()
    # Create a unique empty directory to unpack into
    $TempDir = (Join-Path $TempPath $TempGuid)
    New-Item -ItemType Directory -Path $TempDir

    if ($Uri -Match "downloads.sourceforge.net")
    {
      $Uri = Get-RedirectedUrl -URL $Uri
    }

    Write-Log "INFO" "Downloading $Uri..."
    Invoke-WebRequest -Uri $Uri -OutFile $TempFileName
    Write-Log "DEBUG" "Saved to: $TempFileName"
    Expand-Archive -Path $TempFileName -DestinationPath $TempDir -Force
    Write-Log "DEBUG" "Extracting to: $WindowsPath\$Destination"

    # Because we unpacked into a new directory, we can use * for the directory in the archive,
    # so that we do not need to know the directory name the archive was packed from.
    Move-Item -Path "$TempDir\*" -Destination "$WindowsPath\$Destination" -Force
    Remove-Item -Path $TempDir -Recurse -Force
    Remove-Item -Path $TempFileName -Force
}

# Install VSSetup (Visual Studio detection), ASIO SDK and NSIS Installer
Function Install-Dependencies
{
    Write-Log "STEP" "Installing Dependencies..."
    if (-not (Get-PackageProvider -Name nuget).Name -eq "nuget") {
      Install-PackageProvider -Name "Nuget" -Scope CurrentUser -Force
    }
    Initialize-Module-Here -m "VSSetup"
    Install-Dependency -Uri $NsisUrl -Destination "..\libs\NSIS\NSIS-source"

    if ($BuildOption -Notmatch "jack") {
        # Don't download ASIO SDK on Jamulus JACK builds to save
        # resources and to be extra-sure license-wise.
        Install-Dependency -Uri $AsioSDKUrl -Destination "..\libs\ASIOSDK2"
    } else {
        Write-Log "INFO" "Skipping ASIO SDK (JACK build detected)."
    }
}

# Setup environment variables and build tool paths
Function Initialize-Build-Environment
{
    param(
        [Parameter(Mandatory=$true)]
        [string] $BuildArch
    )

    # Look for Visual Studio/Build Tools 2017 or later (version 15.0 or above)
    $VsInstallPath = Get-VSSetupInstance | `
        Select-VSSetupInstance -Product "*" -Version "15.0" -Latest | `
        Select-Object -ExpandProperty "InstallationPath"

    if ($VsInstallPath -Eq "") { $VsInstallPath = "<N/A>" }

    if ($BuildArch -Eq "x86_64")
    {
        $VcVarsBin = "$VsInstallPath\VC\Auxiliary\build\vcvars64.bat"
    }
    else
    {
        $VcVarsBin = "$VsInstallPath\VC\Auxiliary\build\vcvars32.bat"
    }

    ""
    "**********************************************************************"
    "Using Visual Studio/Build Tools environment settings located at"
    $VcVarsBin
    "**********************************************************************"
    ""

    if (-Not (Test-Path -Path $VcVarsBin))
    {
        Throw "Microsoft Visual Studio ($BuildArch variant) is not installed. " + `
            "Please install Visual Studio 2017 or above it before running this script."
    }

    # Import environment variables set by vcvarsXX.bat into current scope
    $EnvDump = [System.IO.Path]::GetTempFileName()
    Invoke-Native-Command -Command "cmd" `
        -Arguments ("/c", "`"$VcVarsBin`" && set > `"$EnvDump`"")

    foreach ($_ in Get-Content -Path $EnvDump)
    {
        if ($_ -Match "^([^=]+)=(.*)$")
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

    Write-Log "DEBUG" "Qt path '$DefaultPath' not found. Searching registry..."
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
                Write-Log "INFO" "Discovered Qt at: $TryPath"
                return $TryPath
            }
        }
    }

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

    # Check if jom.exe (Qt's clone of nmake that supports parallel execution) is
    # available in the system PATH. This speeds up the build process significantly.
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
            Set-Item Env:QtJomPath $FoundJom
        }
        else
        {
            Set-Item Env:QtJomPath ""
        }
    }

    "**********************************************************************"
    "Using Qt binaries for Visual C++ located at"
    $QtMsvcSpecPath
    "**********************************************************************"
    ""

    if (-Not (Test-Path -Path $Env:QtQmakePath))
    {
        Throw "The Qt binaries for Microsoft Visual C++ 2017 or above could not be located at $QtMsvcSpecPath. " + `
            "Please install Qt with support for MSVC 2017 or above before running this script," + `
            "then call this script with the Qt install location, for example C:\Qt\5.15.2"
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

    $QmkCfg = "CONFIG+=$BuildConfig $BuildArch $BuildOption"

    Invoke-Native-Command -Command "$Env:QtQmakePath" `
        -Arguments ("$RootPath\$AppName.pro", $QmkCfg, "-o", "$BuildPath\Makefile")

    Set-Location -Path $BuildPath

    $MkArgs = @("/NOLOGO", $BuildConfig)

    if ($Env:QtJomPath)
    {
        $Cores = [Math]::Max(1, [Math]::Floor([int]$Env:NUMBER_OF_PROCESSORS / 2))
        Write-Log "INFO" "Building with jom /J $Cores (half of $Env:NUMBER_OF_PROCESSORS cores)"
        Invoke-Native-Command -Command "$Env:QtJomPath" -Arguments (@("/J", "$Cores") + $MkArgs)
    }
    else
    {
        Write-Log "INFO" "Building with nmake (install Qt jom if you want parallel builds)"
        Invoke-Native-Command -Command "nmake" -Arguments $MkArgs
    }

    Invoke-Native-Command -Command "$Env:QtWinDeployPath" `
        -Arguments ("--$BuildConfig", "--compiler-runtime", "--dir=$DeployPath\$BuildArch",
        "$BuildPath\$BuildConfig\$AppName.exe")

    Move-Item -Path "$BuildPath\$BuildConfig\$AppName.exe" -Destination "$DeployPath\$BuildArch" -Force

    $CleanArgs = @("clean", "/NOLOGO")
    Invoke-Native-Command -Command "nmake" -Arguments $CleanArgs -SuppressStdErr

    Set-Location -Path $RootPath
}

# Build and deploy Jamulus 64bit and 32bit variants
function Build-App-Variants
{
    foreach ($_ in ("x86_64", "x86"))
    {
        $OriginalEnv = Get-ChildItem Env:
        if ($_ -eq "x86")
        {
            Initialize-Build-Environment -BuildArch $_
            Initialize-Qt-Build-Environment -QtInstallPath (Resolve-Qt-Path -DefaultPath $QtInstallPath32) -QtCompile $QtCompile32
        }
        else
        {
            Initialize-Build-Environment -BuildArch $_
            Initialize-Qt-Build-Environment -QtInstallPath (Resolve-Qt-Path -DefaultPath $QtInstallPath64) -QtCompile $QtCompile64
        }
        Build-App -BuildConfig "release" -BuildArch $_
        $OriginalEnv | % { Set-Item "Env:$($_.Name)" $_.Value }
    }
}

# Build Windows installer
Function Build-Installer
{
    param(
        [string] $BuildOption
    )

    # --- SAFETY GATE: Do not build if no executables were generated ---
    $Has64 = Test-Path -Path "$DeployPath\x86_64\$AppName.exe"
    $Has32 = Test-Path -Path "$DeployPath\x86\$AppName.exe"

    if (-not $Has64 -and -not $Has32) {
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

    if ($BuildOption -ne "")
    {
        Invoke-Native-Command -Command "$RootPath\libs\NSIS\NSIS-source\makensis" `
            -Arguments ($NsisVerb, "/DAPP_NAME=$AppName", "/DAPP_VERSION=$AppVersion", `
            "/DROOT_PATH=$RootPath", "/DWINDOWS_PATH=$WindowsPath", "/DDEPLOY_PATH=$DeployPath", `
            "/DBUILD_OPTION=$BuildOption", `
            "$WindowsPath\installer.nsi")
    }
    else
    {
        Invoke-Native-Command -Command "$RootPath\libs\NSIS\NSIS-source\makensis" `
            -Arguments ($NsisVerb, "/DAPP_NAME=$AppName", "/DAPP_VERSION=$AppVersion", `
            "/DROOT_PATH=$RootPath", "/DWINDOWS_PATH=$WindowsPath", "/DDEPLOY_PATH=$DeployPath", `
            "$WindowsPath\installer.nsi")
    }
}

# Build and copy NS-Process dll
Function Build-NSProcess
{
    if (!(Test-Path -path "$RootPath\libs\NSIS\nsProcess.dll")) {

        Write-Log "INFO" "Building nsProcess..."

        $OriginalEnv = Get-ChildItem Env:
        Initialize-Build-Environment -BuildArch "x86"

        Invoke-Native-Command -Command "msbuild" `
            -Arguments ("$RootPath\libs\NSIS\nsProcess\nsProcess.sln", '/p:Configuration="Release UNICODE"', `
            "/p:Platform=Win32")

        Move-Item -Path "$RootPath\libs\NSIS\nsProcess\Release\nsProcess.dll" -Destination "$RootPath\libs\NSIS\nsProcess.dll" -Force
        Remove-Item -Path "$RootPath\libs\NSIS\nsProcess\Release\" -Force -Recurse
        $OriginalEnv | % { Set-Item "Env:$($_.Name)" $_.Value }
    }
}

Clean-Build-Environment
Install-Dependencies
Build-App-Variants
Build-NSProcess
Build-Installer -BuildOption $BuildOption
Write-Log "STEP" "Build process completed successfully."
