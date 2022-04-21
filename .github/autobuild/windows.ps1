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
$Qt32Version = "5.15.2"
$Qt64Version = "5.15.2"
$AqtinstallVersion = "2.1.0"
$JackVersion = "1.9.17"
$Msvc32Version = "win32_msvc2019"
$Msvc64Version = "win64_msvc2019_64"
$JomVersion = "1.1.2"

$JamulusVersion = $Env:JAMULUS_BUILD_VERSION
if ( $JamulusVersion -notmatch '^\d+\.\d+\.\d+.*' )
{
    throw "Environment variable JAMULUS_BUILD_VERSION has to be set to a valid version string"
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

    echo "Install JACK2 64-bit..."
    # Install JACK2 64-bit
    choco install --no-progress -y jack --version "${JackVersion}"
    if ( !$? )
    {
        throw "64bit jack installation failed with exit code $LastExitCode"
    }

    echo "Install JACK2 32-bit..."
    # Install JACK2 32-bit (need to force choco install as it detects 64 bits as installed)
    choco install --no-progress -y -f --forcex86 jack --version "${JackVersion}"
    if ( !$? )
    {
        throw "32bit jack installation failed with exit code $LastExitCode"
    }
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
    echo "Setting Github step output name=artifact_1::${artifact}"
    echo "::set-output name=artifact_1::${artifact}"
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
