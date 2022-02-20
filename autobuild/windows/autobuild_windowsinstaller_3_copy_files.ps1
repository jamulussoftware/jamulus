# Powershell

# autobuild_3_copy_files: copy the built files to deploy folder

####################
###  PARAMETERS  ###
####################

# Get the source path via parameter and set BuildOption to empty when not provided
param (
    [string] $jamulus_project_path = $Env:jamulus_project_path,
    [string] $jamulus_buildversionstring = $Env:jamulus_buildversionstring,
    [string] $BuildOption = ""
)

# Fail early on all errors
$ErrorActionPreference = "Stop"

# Sanity check of parameters
if (("$jamulus_project_path" -eq $null) -or ("$jamulus_project_path" -eq "")) {
    throw "expecting ""jamulus_project_path"" as parameter or ENV"
} elseif (!(Test-Path -Path "$jamulus_project_path")) {
    throw "non.existing jamulus_project_path: $jamulus_project_path"
} else {
    echo "jamulus_project_path is valid: $jamulus_project_path"
}
if (($jamulus_buildversionstring -eq $null) -or ($jamulus_buildversionstring -eq "")) {
    echo "expecting ""jamulus_buildversionstring"" as parameter or ENV"
    echo "using ""NoVersion"" as jamulus_buildversionstring for filenames"
    $jamulus_buildversionstring = "NoVersion"
}

# Amend BuildOption when needed, only needed when the name build option in the artifact needs to be different
switch ($BuildOption)
{
    "jackonwindows"   {$BuildOption = "jack"; break}
}


###################
###  PROCEDURE  ###
###################

# Copy the file
echo "copy"
if ($BuildOption -ne "")
{
    $artifact_deploy_filename = "jamulus_${Env:jamulus_buildversionstring}_win_${BuildOption}.exe"
}
else
{
    $artifact_deploy_filename = "jamulus_${Env:jamulus_buildversionstring}_win.exe"
}

echo "copying deploy file to $artifact_deploy_filename"
cp "$jamulus_project_path\deploy\Jamulus*installer-win.exe" "$jamulus_project_path\deploy\$artifact_deploy_filename"
if ( !$? )
{
		throw "cp failed with exit code $LastExitCode";
}


Function github_output_value
{
    param (
        [Parameter(Mandatory=$true)]
        [string] $name,
        [Parameter(Mandatory=$true)]
        [string] $value
    )

    echo "github_output_value() $name = $value"
    echo "::set-output name=$name::$value"
}


github_output_value -name "artifact_1" -value "$artifact_deploy_filename"
