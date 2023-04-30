# Steps for generating Windows artifacts via Github Actions
# See README.md in this folder for details.
# See windows/deploy_windows.ps1 for standalone builds.

param(
    [Parameter(Mandatory=$true)]
    [string] $Stage = "",
    # Allow buildoption to be passed for jackonwindows build, leave empty for standard (ASIO) build:
    [string] $BuildOption = "",
    # unused, only required during refactoring as long as not all platforms have been updated:
    [string] $GithubWorkspace =""
)

# Fail early on all errors
$ErrorActionPreference = "Stop"

$QtDir = 'C:\Qt'
$ChocoCacheDir = 'C:\ChocoCache'
$DownloadCacheDir = 'C:\AutobuildCache'
# The following version pinnings are semi-automatically checked for
# updates. Verify .github/workflows/bump-dependencies.yaml when changing those manually:
$Qt32Version = "5.15.2"
$Qt64Version = "6.4.3"
$AqtinstallVersion = "3.1.5"
$JackVersion = "1.9.22"
$Msvc32Version = "win32_msvc2019"
$Msvc64Version = "win64_msvc2019_64"
$JomVersion = "1.1.2"

# Compose JACK download urls
$JackBaseUrl = "https://github.com/jackaudio/jack2-releases/releases/download/v${JackVersion}/jack2-win"
$Jack64Url = $JackBaseUrl + "64-v${JackVersion}.exe"
$Jack32Url = $JackBaseUrl + "32-v${JackVersion}.exe"

$JamulusVersion = $Env:JAMULUS_BUILD_VERSION
if ( $JamulusVersion -notmatch '^\d+\.\d+\.\d+.*' )
{
    throw "Environment variable JAMULUS_BUILD_VERSION has to be set to a valid version string"
}

# Download dependency to cache directory
Function Download-Dependency
{
    param(
        [Parameter(Mandatory=$true)]
        [string] $Uri,
        [Parameter(Mandatory=$true)]
        [string] $Name
    )


    # Restore dependency if cached copy already exists
    if (Test-Path -Path "$DownloadCacheDir\$Name" -PathType Leaf)
    {
        echo "Using ${DownloadCacheDir}\${Name} from previous run (actions/cache)"
        return
    }

    Invoke-WebRequest -Uri $Uri -OutFile "${DownloadCacheDir}\${Name}"

    if ( !$? )
    {
        throw "Download of $Name ($Uri) failed with exit code $LastExitCode"
    }
}

Function Install-Qt
{
    param(
        [string] $QtVersion,
        [string] $QtArch
    )
    $Args = (
        "--outputdir", "$QtDir",
        "windows",
        "desktop",
        "$QtVersion",
        "$QtArch",
        "--archives", "qtbase", "qttools", "qttranslations"
    )
    if ( $QtVersion -notmatch '5\.[0-9]+\.[0-9]+' )
    {
        # From Qt6 onwards, qtmultimedia is a module and cannot be installed
        # as an archive anymore.
        $Args += ("--modules")
    }
    $Args += ("qtmultimedia")
    aqt install-qt @Args
    if ( !$? )
    {
        echo "WARNING: Qt installation via first aqt run failed, re-starting with different base URL."
        aqt install-qt -b https://mirrors.ocf.berkeley.edu/qt/ @Args
        if ( !$? )
        {
            throw "Qt installation with args @Args failed with exit code $LastExitCode"
        }
    }
}

Function Ensure-Qt
{
    if ( Test-Path -Path $QtDir )
    {
        echo "Using Qt installation from previous run (actions/cache)"
        return
    }

    echo "Install Qt..."
    # Install Qt
    pip install "aqtinstall==$AqtinstallVersion"
    if ( !$? )
    {
        throw "pip install aqtinstall failed with exit code $LastExitCode"
    }

    echo "Get Qt 64 bit..."
    Install-Qt "${Qt64Version}" "${Msvc64Version}"

    echo "Get Qt 32 bit..."
    Install-Qt "${Qt32Version}" "${Msvc32Version}"
}

Function Ensure-jom
{
    choco install --no-progress -y jom --version "${JomVersion}"
}

Function Ensure-JACK
{
    if ( $BuildOption -ne "jackonwindows" )
    {
        return
    }

    # Set installer parameters for silent install

    $JACKInstallParms = "/SILENT", "/SUPPRESSMSGBOXES", "/NORESTART"

    # Create cache directory if it doesn't exist yet

    if (-not(Test-Path -Path "$DownloadCacheDir"))
    {
        New-Item -Path $DownloadCacheDir -ItemType "directory"
    }

    echo "Downloading 64 Bit and 32 Bit JACK installer (if needed)..."

    Download-Dependency -Uri $Jack64Url -Name "JACK64.exe"
    Download-Dependency -Uri $Jack32Url -Name "JACK32.exe"

    # Install JACK 64 Bit silently via installer

    echo "Installing JACK2 64-bit..."

    $JACKInstallPath = "${DownloadCacheDir}\JACK64.exe"

    & $JACKInstallPath $JACKInstallParms

    if ( !$? )
    {
        throw "64bit JACK installer failed with exit code $LastExitCode"
    }

    echo "64bit JACK installation completed successfully"

    echo "Installing JACK2 32-bit..."

    # Install JACK 32 Bit silently via installer

    $JACKInstallPath = "${DownloadCacheDir}\JACK32.exe"

    & $JACKInstallPath $JACKInstallParms

    if ( !$? )
    {
        throw "32bit JACK installer failed with exit code $LastExitCode"
    }

    echo "32bit JACK installation completed successfully"
}

Function Build-App-With-Installer
{
    echo "Build app and create installer..."
    $ExtraArgs = @()
    if ( $BuildOption -ne "" )
    {
        $ExtraArgs += ("-BuildOption", $BuildOption)
    }
    powershell ".\windows\deploy_windows.ps1" "C:\Qt\${Qt32Version}" "C:\Qt\${Qt64Version}" @ExtraArgs
    if ( !$? )
    {
        throw "deploy_windows.ps1 failed with exit code $LastExitCode"
    }
}

Function Pass-Artifact-to-Job
{
    # Add $BuildOption as artifact file name suffix. Shorten "jackonwindows" to just "jack":
    $ArtifactSuffix = switch -Regex ( $BuildOption )
    {
        "jackonwindows" { "_jack"; break }
        "^\S+$"         { "_${BuildOption}"; break }
        default         { "" }
    }

    $artifact = "jamulus_${JamulusVersion}_win${ArtifactSuffix}.exe"

    echo "Copying artifact to ${artifact}"
    move ".\deploy\Jamulus*installer-win.exe" ".\deploy\${artifact}"
    if ( !$? )
    {
        throw "move failed with exit code $LastExitCode"
    }
    echo "Setting Github step output name=artifact_1 to ${artifact}"
    echo "artifact_1=${artifact}" >> "$Env:GITHUB_OUTPUT"
}

switch ( $Stage )
{
    "setup"
    {
        choco config set cacheLocation $ChocoCacheDir
        Ensure-Qt
        Ensure-jom
        Ensure-JACK
    }
    "build"
    {
        Build-App-With-Installer
    }
    "get-artifacts"
    {
        Pass-Artifact-to-Job
    }
    default
    {
        throw "Unknown stage ${Stage}"
    }
}
