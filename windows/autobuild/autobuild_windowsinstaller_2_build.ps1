# Powershell

# autobuild_2_build: actual build process


####################
###  PARAMETERS  ###
####################

# Get the source path via parameter
param (
    [string] $jamulus_project_path = $Env:jamulus_project_path
)
# Sanity check of parameters
if (("$jamulus_project_path" -eq $null) -or ("$jamulus_project_path" -eq "")) {
    throw "expecting ""jamulus_project_path"" as parameter or ENV"
} elseif (!(Test-Path -Path $jamulus_project_path)) {
    throw "non.existing jamulus_project_path: $jamulus_project_path"
} else {
    echo "jamulus_project_path is valid: $jamulus_project_path"
}


###################
###  PROCEDURE  ###
###################

echo "Build installer..."
# Build the installer
powershell "$jamulus_project_path\windows\deploy_windows.ps1" "C:\Qt\5.15.2"
