#==================================================================================================
# Visual Studio Tool Script by pgScorpio
#==================================================================================================
Add-Type -AssemblyName 'PresentationFramework'
#==================================================================================================
# WARNING: 
#   The vcx project files in this folder are heavily modified to make them movable.
#   DO NOT REGENERATE VCX FILES FROM Qt!
#   When the Jamulus pro file is changed, the vcx files need (often manual) editing too!
#
#   In Visual Studio NEVER use ABSOLUTE PATHS but always use the appropriate variables.
#   In Visual Studio NEVER use PERSONAL PATHS but always use appropriate environment variables. 
#   
#   Do NOT directly open the JamulusWin.vcxproj or JamulusWin.sln files by double-clicking 
#   or by opening them from Visual Studio IDE !
#
#   ALWAYS start Visual studio using this script from Powershell using./JamulusWin.ps1 --startvs
#==================================================================================================

# General Env names in Visual Studio (Though there ARE several exceptions!!):
#   $(*Dir)  Ends with \ or /         Undefined variable defaults to the root of the current drive
#   $(*Path) Does NOT end with \ or / Undefined variable defaults to the current folder            (But $(*Path) is also used for filenames!)


#==================================================================================================
# Modify this section to reflect your Qt and Visual Studio installation.
#==================================================================================================

$QtBase      = "C:\Qt\5.15.2"
$QtMSVC      = "msvc2019"

$Jamulus_Version="3.8.2"
#For development versions uncomment next line:
$Jamulus_Version =$Jamulus_Version + 'dev-'+$(git describe --match=xxxxxxxxxxxxxxxxxxxx --always --abbrev --dirty)

#==================================================================================================
# Fixed settings:
#==================================================================================================

$QtSrcPath   = "$("$QtBase\$QtMSVC")_64"
$QtBinPath   = "$QtSrcPath\bin"

#QtSrcDir and QtBinDir will be set accordingly with added \

#==================================================================================================
# configuration for this script
#==================================================================================================
$BuildType = ""

$InfoForeground    = "Green"
$InfoBackground    = "DarkBlue"

$WarningForeground = "Yellow"
$WarningBackground = "Black"

$ErrorForeground   = "Red"
$ErrorBackground   = "Black"

$PromptForeground  = "Yellow"
$PromptBackground  = "DarkBlue"


#==================================================================================================
# Get project name and location
#==================================================================================================

# Scriptname should match the vcx Project Name!
$vcxproject_name   = [io.path]::GetFileNameWithoutExtension( $MyInvocation.MyCommand.Name )
$vcxproject_file   = "$vcxproject_name.vcxproj"

# The full folder path in which the vcxproj should be located (Same as this script location)
$ProjectFolderPath = [io.path]::GetDirectoryName( $MyInvocation.MyCommand.Path )


#==================================================================================================
# Script tools:
#==================================================================================================

function info( $text ) 
{
    Write-Host $text  -ForegroundColor $InfoForeground -BackgroundColor $InfoBackground
}

function warning( $text ) 
{

    Write-Host $text  -ForegroundColor $WarningForeground -BackgroundColor $WarningBackground
}

function error( $text ) 
{
    Write-Host $text  -ForegroundColor $ErrorForeground -BackgroundColor $ErrorBackground
}


## safe getEnv: return empty string if var does not exist...
function getEnv( [string] $var )
{
    if ( Test-Path "Env:$($var)" )
    {
        return (Get-Item -Path Env:$($var)).Value
    }
    else
    {
        return ""
    }
}

## verbose setEnv: reports changes...
function setEnv( [string] $var, [string] $value )
{
    if ( $(getEnv( $var )) -ne $value )
    {
        Set-Item -Path Env:$var -Value $value
        Write-Host "$var set to '$(getEnv($var))' !" -ForegroundColor $InfoForeground -BackgroundColor $InfoBackground
    }
}

## safe getEnv: return empty string if var does not exist...
function AddToPath( [string]$addPath )
{
    if ( Test-Path $addPath )
    {
        $arrPath = $Env:Path -split ';'
        
        $found = $false
        foreach( $dir in $arrPath)
        {
            if ($dir -eq $addPath)
            {
                $found = $true;
            }
        }
        
        if ( -not ($found) )
        {
            $last = ($arrPath.length - 1)
            if ( $arrPath[$last] -eq '' )
            {
                $arrPath[$last] = $addPath
            }
            else
            {
                $arrPath += $addPath
            }

            info "Adding $addPath to PATH"
        }
        else
        {
            # Already in path!
            return
        }
        
        $Env:Path = $arrPath -join ';'
        
        return
    } 
    else 
    {
        error ( @(
                  "Dir $addPath does not exist!  "
                  "So it was NOT addded to Path !"
                 ) -join "`n" )

        
        return
    }
}


#=============================================================================
# 'User Defined' functions:
#=============================================================================

function CheckEnvironment()
{
    if ( -not ( Test-Path -Path ".\$vcxproject_file" ) )
    {
        error "Error: This script must be run from the folder containing the $vcxproject_file file!"
        exit 1;
    }

    Write-Host "Checking environment for $vcxproject_name...."



    #BuildDir should NOT be located under the Jamulus repo folder!
    #so we set BuildDir to a build folder on the same level as the repo folder...
    $BuildRoot         = [io.path]::GetFullPath( "$ProjectFolderPath\..\build" )
    # Use the same repo/project folder structure as the current repo
    # The folder name in which the vcxproj is located
    $ProjectFolderName = [io.path]::GetFileName( $ProjectFolderPath )
    $BuildPath         = "$BuildRoot\$ProjectFolderName\$vcxproject_name"
    #BuildDir will be set accordingly with added \


    AddToPath $QtBinPath


    #=========================================================================
    # Add user defined paths here:
    #=========================================================================
    # Use: AddToPath "NEWPATH"

    #=========================================================================


    setEnv 'VCProjectPath'  $PWD
    setEnv 'VCProjectDir'   "$PWD\"

    setEnv 'BuildPath'      $BuildPath
    setEnv 'BuildDir'       "$BuildPath\"

    setEnv 'QtBase'         $QtBase     
    setEnv 'QtSrcPath'      $QtSrcPath      
    setEnv 'QtSrcDir'       "$QtSrcPath\"
    setEnv 'QtBinPath'      $QtBinPath
    setEnv 'QtBinDir'       "$QtBinPath\"
    setEnv 'QMAKESPEC'      $QMAKESPEC  

    setEnv 'Jamulus_Version' $Jamulus_Version

    #=========================================================================
    # Add user defined environment variables here:
    #=========================================================================
    # Use: setEnv "NAME" "VALUE"

    #=========================================================================


    Write-Host "Environment set"
}


function PreBuild
{
    # Custom Pre-Build step called from VS. $BuildType should be debug | release

    # Save the current build environment to a textfile in the current build folder.
    cmd /c "set > $Env:BuildDir$BuildType\build-env.txt"

    # Dirty Fix for: warning MSB8064: Custom build for item "..\build\JamulusWin\release\moc_predefs.h.cbt" succeeded, 
    # but specified dependency "f:\repos\build\jamuluswin\release\moc_predefs.h.cbt" does not exist. 
    # This may cause incremental build to work incorrectly

    $moc_predefs = "$Env:BuildDir$BuildType\moc\moc_predefs.h"

    if ( -not ( Test-Path -Path "$moc_predefs" ) )
    {
        #remove cbt file after a clean
        if ( Test-Path -Path "$moc_predefs.cbt" )
        {
            echo "Prebuils: Deleting $moc_predefs.cbt..."
            Remove-Item "$moc_predefs.cbt"
        }
    }
}

function PreLink
{
    # Custom Pre-Link step called from VS. $BuildType should be debug | release


    # Dirty Fix for: warning MSB8064: Custom build for item "..\build\JamulusWin\release\moc_predefs.h.cbt" succeeded, 
    # but specified dependency "f:\repos\build\jamuluswin\release\moc_predefs.h.cbt" does not exist. 
    # This may cause incremental build to work incorrectly

    $moc_predefs = "$Env:BuildDir$BuildType\moc\moc_predefs.h"

    if ( ( Test-Path -Path "$moc_predefs" ) -and (-not ( Test-Path -Path "$moc_predefs.cbt" )) )
    {
        echo "Prelink: Generating $moc_predefs.cbt..."

        "Generated by $($vcxproject_name).ps1 --prebuild $BuildType" > "$moc_predefs.cbt"
    }
}

function PostBuild
{
    # Custom Post-Build step called from VS. $args[1] should be debug | release
    # WARNING: This step is ALSO called by Visual Studio BEFORE a re-build !
}



#=============================================================================
#=============================================================================

function help
{
    echo ""
    echo "=================================================================================================="
    echo "$($vcxproject_name).ps1: (toolset script by psScorpio for Visual Studio project $vcxproject_name.)"
    echo "=================================================================================================="
    echo ""
    echo "parameters: *)"
    echo "--setenv    Just set the environment for the project as defined in this script."
    echo "--startvs   Set the environment and open $vcxproject_name.vcxproj."
    echo ""
    echo "Also handles Custom build events for VS build with parameters:"
    echo "--prebuild  debug | release"
    echo "--prelink   debug | release"
    echo "--postbuild debug | release"
    echo ""
    echo "*) $($vcxproject_name).ps1 must be run from the folder containing the $vcxproject_name project files."
    echo "   Do NOT open the project in Visual studio without setting the environment first!"
    echo ""
    echo "=================================================================================================="
}

if ( $args.count -ge 1 ) 
{
    $option = $args[0]

    if ( $args.count -ge 2 )
    {
        $BuildType = $args[1]
    }

    if ( $option -eq "--setenv" )
    {
        CheckEnvironment
        exit 0
    }

    if ( $option -eq "--startvs" )
    {
        CheckEnvironment
        Invoke-Expression -Command ".\$vcxproject_file"
        exit
    }

    if ( $option -eq "--prebuild" )
    {
        echo "Custom Pre-Build step for $vcxproject_name $BuildType"

        if ( $args.count -ge 2 )
        {
            PreBuild
            exit 0
        }

        echo "Custom Pre-Build step missing BuildType parameter !"
        exit 1;
    }
    
    if ( $option -eq "--prelink" )
    {
        echo "Custom Pre-Link step for $vcxproject_name $BuildType"

        if ( $args.count -ge 2 )
        {
            PreLink
            exit 0;
        }

        echo "Custom Pre-Link step missing BuildType parameter !"
        exit 1
    }

    if ( $option -eq "--postbuild" )
    {
        echo "Custom Post-Build step for $vcxproject_name $BuildType"

        if ( $args.count -ge 2 )
        {
            PostBuild
            exit 0;
        }

        echo "Custom Post-Build step missing BuildType parameter !"
        exit 1
    }
    
    if ( $option -eq "--help" )
    {
        Help
        exit 0;
    }

    error "Invalid parameter $option"
    help
    exit 1;
 }

 help
 exit 1;
 