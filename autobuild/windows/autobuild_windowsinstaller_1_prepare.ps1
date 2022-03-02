# Powershell

# autobuild_1_prepare: set up environment, install Qt & dependencies


####################
###  PARAMETERS  ###
####################

param(
    # Allow buildoption to be passed for jackonwindows build, leave empty for standard (ASIO) build
    [string] $BuildOption = ""
)

# Fail early on all errors
$ErrorActionPreference = "Stop"

Function Install-Qt {
    param(
        [string] $QtVersion,
        [string] $QtArch,
        [string] $InstallDir
    )
    $Args = ("--outputdir", "$InstallDir", "windows", "desktop", "$QtVersion", "$QtArch")
    aqt install-qt @Args
    if ( !$? )
    {
        echo "WARNING: Qt installation via first aqt run failed, re-starting with different base URL."
        aqt install-qt @Args -b https://mirrors.ocf.berkeley.edu/qt/
        if ( !$? )
        {
            throw "Qt installation with args @Arguments failed with exit code $LastExitCode"
        }
    }
}

###################
###  PROCEDURE  ###
###################

$QtDir = 'C:\Qt'
$ChocoCacheDir = 'C:\ChocoCache'
$Qt32Version = "5.15.2"
$Qt64Version = "5.15.2"
$AqtinstallVersion = "2.0.5"
$JackVersion = "1.9.17"
$Msvc32Version = "win32_msvc2019"
$Msvc64Version = "win64_msvc2019_64"
$JomVersion = "1.1.2"

if ( Test-Path -Path $QtDir )
{
    echo "Using Qt installation from previous run (actions/cache)"
}
else
{
    echo "Install Qt..."
    # Install Qt
    pip install "aqtinstall==$AqtinstallVersion"
    if ( !$? )
    {
        throw "pip install aqtinstall failed with exit code $LastExitCode"
    }

    echo "Get Qt 64 bit..."
    Install-Qt ${Qt64Version} ${Msvc64Version} ${QtDir}

    echo "Get Qt 32 bit..."
    Install-Qt ${Qt32Version} ${Msvc32Version} ${QtDir}
}

choco config set cacheLocation $ChocoCacheDir
choco install --no-progress -y jom --version "${JomVersion}"


#################################
###  ONLY ADD JACK IF NEEDED  ###
#################################

if ($BuildOption -Eq "jackonwindows")
{
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
