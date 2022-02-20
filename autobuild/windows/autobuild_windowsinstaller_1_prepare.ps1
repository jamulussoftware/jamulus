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

###################
###  PROCEDURE  ###
###################

$QtDir = 'C:\Qt'
$ChocoCacheDir = 'C:\ChocoCache'
$Qt32Version = "5.15.2"
$Qt64Version = "5.15.2"
$AqtinstallVersion = "2.0.5"
$JackVersion = "1.9.17"
$MsvcVersion = "2019"

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
    # intermediate solution if the main server is down: append e.g. " -b https://mirrors.ocf.berkeley.edu/qt/" to the "aqt"-line below
    aqt install-qt --outputdir "${QtDir}" windows desktop "${Qt64Version}" "win64_msvc${MsvcVersion}_64"
    if ( !$? )
    {
        throw "64bit Qt installation failed with exit code $LastExitCode"
    }

    echo "Get Qt 32 bit..."
    # intermediate solution if the main server is down: append e.g. " -b https://mirrors.ocf.berkeley.edu/qt/" to the "aqt"-line below
    aqt install-qt --outputdir "${QtDir}" windows desktop "${Qt32Version}" "win32_msvc${MsvcVersion}"
    if ( !$? )
    {
            throw "32bit Qt installation failed with exit code $LastExitCode"
    }
}


#################################
###  ONLY ADD JACK IF NEEDED  ###
#################################

if ($BuildOption -Eq "jackonwindows")
{
    choco config set cacheLocation $ChocoCacheDir

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
