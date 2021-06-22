# Powershell

# autobuild_1_prepare: set up environment, install Qt & dependencies


####################
###  PARAMETERS  ###
####################

param(
    # Allow buildoption to be passed for jackonwindows build, leave empty for standard (ASIO) build
    [string] $BuildOption = ""
)


###################
###  PROCEDURE  ###
###################

echo "Install Qt..."
# Install Qt
pip install aqtinstall
echo "Get Qt 64 bit..."
# intermediate solution if the main server is down: append e.g. " -b https://mirrors.ocf.berkeley.edu/qt/" to the "aqt"-line below
aqt install --outputdir C:\Qt 5.15.2 windows desktop win64_msvc2019_64

echo "Get Qt 32 bit..."
# intermediate solution if the main server is down: append e.g. " -b https://mirrors.ocf.berkeley.edu/qt/" to the "aqt"-line below
aqt install --outputdir C:\Qt 5.15.2 windows desktop win32_msvc2019


#################################
###  ONLY ADD JACK IF NEEDED  ###
#################################

if ($BuildOption -Eq "jackonwindows")
{
    echo "Install JACK2 64-bit..."
    # Install JACK2 64-bit
    choco install --no-progress -y jack

    echo "Install JACK2 32-bit..."
    # Install JACK2 32-bit (need to force choco install as it detects 64 bits as installed)
    choco install --no-progress -y -f --forcex86 jack
}

