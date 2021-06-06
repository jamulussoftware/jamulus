# Powershell

# autobuild_1_prepare: set up environment, install Qt & dependencies


###################
###  PROCEDURE  ###
###################

echo "Install Qt..."
# Install Qt
pip install aqtinstall
echo "Get Qt 64 bit..."
# intermediate solution if the main server is down: append e.g. " -b https://mirrors.ocf.berkeley.edu/qt/" to the "aqt"-line below
aqt install --outputdir C:\Qt 6.1.0 windows desktop win64_msvc2019_64

echo "Get Qt 32 bit..."
# intermediate solution if the main server is down: append e.g. " -b https://mirrors.ocf.berkeley.edu/qt/" to the "aqt"-line below
aqt install --outputdir C:\Qt 6.1.0 windows desktop win32_msvc2019
