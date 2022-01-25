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

echo "Install Qt..."
# Install Qt
pip install aqtinstall
if ( !$? )
{
		throw "pip install aqtinstall failed with exit code $LastExitCode"
}

echo "Get Qt 64 bit..."
# intermediate solution if the main server is down: append e.g. " -b https://mirrors.ocf.berkeley.edu/qt/" to the "aqt"-line below
aqt install-qt --outputdir C:\Qt windows desktop 5.15.2 win64_msvc2019_64
if ( !$? )
{
		throw "64bit Qt installation failed with exit code $LastExitCode"
}

echo "Get Qt 32 bit..."
# intermediate solution if the main server is down: append e.g. " -b https://mirrors.ocf.berkeley.edu/qt/" to the "aqt"-line below
aqt install-qt --outputdir C:\Qt windows desktop 5.15.2 win32_msvc2019
if ( !$? )
{
		throw "32bit Qt installation failed with exit code $LastExitCode"
}


#################################
###  ONLY ADD JACK IF NEEDED  ###
#################################

if ($BuildOption -Eq "jackonwindows")
{
    echo "Install JACK2 64-bit..."
    # Install JACK2 64-bit
    choco install --no-progress -y jack
    if ( !$? )
    {
        throw "64bit jack installation failed with exit code $LastExitCode"
    }

    echo "Install JACK2 32-bit..."
    # Install JACK2 32-bit (need to force choco install as it detects 64 bits as installed)
    choco install --no-progress -y -f --forcex86 jack
    if ( !$? )
    {
        throw "32bit jack installation failed with exit code $LastExitCode"
    }
}
