JamulusWin.* files for builing the windows version using visual studio 2019 (by pgScorpio)

#==================================================================================================
# WARNING: 
#   The vcx project files in this folder are heavily modified to make them movable.
#   DO NOT REGENERATE VCX FILES FROM Qt!
#   When the Jamulus pro file is changed, the vcx files need (often manual) editing too!
#
#   In Visual Studio NEVER use ABSOLUTE PATHS but always use the appropriate variables.
#   In Visual Studio NEVER use PERSONAL PATHS but always use appropriate environment variables. 
#   
#   Do NOT directly open the JamulusWin.vcxproj or JamulusWin.sln files by double-clicking !
#   ALWAYS start Visual studio using this script from Powershell using./JamulusWin.ps1 --startvs
#==================================================================================================


JamulusWin.ps1 is a PowerShell script to setup the environment needed for builing with Visual Studio.
JamulusWin.ps1 also handles all Custom Build Steps during building of the project. (Modify when needed.)

Run ./JamulusWin.ps1 without parameters for command options info.

You should check the variables defined in JamulusWin.ps1 to reflect your Qt installation 

You can modify the 'User Defined functions' in JamulusWin.ps1 if needed.

