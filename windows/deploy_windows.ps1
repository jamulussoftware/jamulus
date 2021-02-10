param(
    # Replace default path with system Qt installation folder if necessary
    [string] $QtInstallPath = "C:\Qt\5.12.3",
    [string] $QtCompile32 = "msvc2019",
    [string] $QtCompile64 = "msvc2019_64",
    [string] $AsioSDKName = "ASIOSDK2.3.2",
    [string] $AsioSDKUrl = "https://www.steinberg.net/sdk_downloads/ASIOSDK2.3.2.zip",
    [string] $NsisName = "nsis-3.06.1",
    [string] $NsisUrl = "https://downloads.sourceforge.net/project/nsis/NSIS%203/3.06.1/nsis-3.06.1.zip"
)

# change directory to the directory above (if needed)
Set-Location -Path "$PSScriptRoot\..\"

# Global constants
$RootPath = "$PWD"
$BuildPath = "$RootPath\build"
$DeployPath = "$RootPath\deploy"
$WindowsPath ="$RootPath\windows"
$AppName = "Jamulus"

# Stop at all errors
$ErrorActionPreference = "Stop"


# Execute native command with errorlevel handling
Function Invoke-Native-Command {
    Param(
        [string] $Command,
        [string[]] $Arguments
    )

    & "$Command" @Arguments

    if ($LastExitCode -Ne 0)
    {
        Throw "Native command $Command returned with exit code $LastExitCode"
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

    Param (
        [Parameter(Mandatory=$true)]
        [String]$URL
    )

    $request = [System.Net.WebRequest]::Create($url)
    $request.AllowAutoRedirect=$false
    $response=$request.GetResponse()

    if ($response.StatusCode -eq "Found")
    {
        $response.GetResponseHeader("Location")
    }
}

function Initialize-Module-Here ($m) { # see https://stackoverflow.com/a/51692402

    # If module is imported say that and do nothing
    if (Get-Module | Where-Object {$_.Name -eq $m}) {
        Write-Output "Module $m is already imported."
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
                Write-Output "Module $m not imported, not available and not in online gallery, exiting."
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
        [string] $Name,
        [Parameter(Mandatory=$true)]
        [string] $Destination
    )

    if (Test-Path -Path "$WindowsPath\$Destination") { return }

    $TempFileName = [System.IO.Path]::GetTempFileName() + ".zip"
    $TempDir = [System.IO.Path]::GetTempPath()

    if ($Uri -Match "downloads.sourceforge.net")
    {
      $Uri = Get-RedirectedUrl -URL $Uri
    }

    Invoke-WebRequest -Uri $Uri -OutFile $TempFileName
    echo $TempFileName
    Expand-Archive -Path $TempFileName -DestinationPath $TempDir -Force
    echo $WindowsPath\$Destination
    Move-Item -Path "$TempDir\$Name" -Destination "$WindowsPath\$Destination" -Force
    Remove-Item -Path $TempFileName -Force
}

# Install VSSetup (Visual Studio detection), ASIO SDK and NSIS Installer
Function Install-Dependencies
{
    if (-not (Get-PackageProvider -Name nuget).Name -eq "nuget") {
      Install-PackageProvider -Name "Nuget" -Scope CurrentUser -Force
    }
    Initialize-Module-Here -m "VSSetup"
    Install-Dependency -Uri $AsioSDKUrl `
        -Name $AsioSDKName -Destination "ASIOSDK2"
    Install-Dependency -Uri $NsisUrl `
        -Name $NsisName -Destination "NSIS"
}

# Setup environment variables and build tool paths
Function Initialize-Build-Environment
{
    param(
        [Parameter(Mandatory=$true)]
        [string] $QtInstallPath,
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
        $QtMsvcSpecPath = "$QtInstallPath\$QtCompile64\bin"
    }
    else
    {
        $VcVarsBin = "$VsInstallPath\VC\Auxiliary\build\vcvars32.bat"
        $QtMsvcSpecPath = "$QtInstallPath\$QtCompile32\bin"
    }

    # Setup Qt executables paths for later calls
    Set-Item Env:QtQmakePath "$QtMsvcSpecPath\qmake.exe"
    Set-Item Env:QtWinDeployPath "$QtMsvcSpecPath\windeployqt.exe"

    ""
    "**********************************************************************"
    "Using Visual Studio/Build Tools environment settings located at"
    $VcVarsBin
    "**********************************************************************"
    ""
    "**********************************************************************"
    "Using Qt binaries for Visual C++ located at"
    $QtMsvcSpecPath
    "**********************************************************************"
    ""

    if (-Not (Test-Path -Path $VcVarsBin))
    {
        Throw "Microsoft Visual Studio ($BuildArch variant) is not installed. " + `
            "Please install Visual Studio 2017 or above it before running this script."
    }

    if (-Not (Test-Path -Path $Env:QtQmakePath))
    {
        Throw "The Qt binaries for Microsoft Visual C++ 2017 (msvc2017) could not be located. " + `
            "Please install Qt with support for MSVC 2017 before running this script," + `
            "then call this script with the Qt install location, for example C:\Qt\5.12.3"
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

# Build Jamulus x86_64 and x86
Function Build-App
{
    param(
        [Parameter(Mandatory=$true)]
        [string] $BuildConfig,
        [Parameter(Mandatory=$true)]
        [string] $BuildArch
    )

    Invoke-Native-Command -Command "$Env:QtQmakePath" `
        -Arguments ("$RootPath\$AppName.pro", "CONFIG+=$BuildConfig $BuildArch", `
        "-o", "$BuildPath\Makefile")

    Set-Location -Path $BuildPath
    Invoke-Native-Command -Command "nmake" -Arguments ("$BuildConfig")
    Invoke-Native-Command -Command "$Env:QtWinDeployPath" `
        -Arguments ("--$BuildConfig", "--compiler-runtime", "--dir=$DeployPath\$BuildArch",
        "$BuildPath\$BuildConfig\$AppName.exe")

    Move-Item -Path "$BuildPath\$BuildConfig\$AppName.exe" -Destination "$DeployPath\$BuildArch" -Force
    Invoke-Native-Command -Command "nmake" -Arguments ("clean")
    Set-Location -Path $RootPath
}

# Build and deploy Jamulus 64bit and 32bit variants
function Build-App-Variants
{
    param(
        [Parameter(Mandatory=$true)]
        [string] $QtInstallPath
    )

    foreach ($_ in ("x86_64", "x86"))
    {
        $OriginalEnv = Get-ChildItem Env:
        Initialize-Build-Environment -QtInstallPath $QtInstallPath -BuildArch $_
        Build-App -BuildConfig "release" -BuildArch $_
        $OriginalEnv | % { Set-Item "Env:$($_.Name)" $_.Value }
    }
}

# Build Windows installer
Function Build-Installer
{
    foreach ($_ in Get-Content -Path "$RootPath\$AppName.pro")
    {
        if ($_ -Match "^VERSION *= *(.*)$")
        {
            $AppVersion = $Matches[1]
            break
        }
    }

    Invoke-Native-Command -Command "$WindowsPath\NSIS\makensis" `
        -Arguments ("/v4", "/DAPP_NAME=$AppName", "/DAPP_VERSION=$AppVersion", `
        "/DROOT_PATH=$RootPath", "/DWINDOWS_PATH=$WindowsPath", "/DDEPLOY_PATH=$DeployPath", `
        "$WindowsPath\installer.nsi")
}

# Build and copy NS-Process dll
Function Build-NSProcess
{
    param(
        [Parameter(Mandatory=$true)]
        [string] $QtInstallPath
    )
    if (!(Test-Path -path "$WindowsPath\nsProcess.dll")) {

        echo "Building nsProcess..."

        $OriginalEnv = Get-ChildItem Env:
        Initialize-Build-Environment -QtInstallPath $QtInstallPath -BuildArch "x86"
    
        Invoke-Native-Command -Command "msbuild" `
            -Arguments ("$WindowsPath\nsProcess\nsProcess.sln", '/p:Configuration="Release UNICODE"', `
            "/p:Platform=Win32")
   
        Move-Item -Path "$WindowsPath\nsProcess\Release\nsProcess.dll" -Destination "$WindowsPath\nsProcess.dll" -Force
        Remove-Item -Path "$WindowsPath\nsProcess\Release\" -Force -Recurse
        $OriginalEnv | % { Set-Item "Env:$($_.Name)" $_.Value }
    }
}

Clean-Build-Environment
Install-Dependencies
Build-App-Variants -QtInstallPath $QtInstallPath
Build-NSProcess -QtInstallPath $QtInstallPath
Build-Installer
