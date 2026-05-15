Write-Output "Listing C:\SDKs\ASIOSDK\host (top levels)"
Get-ChildItem -LiteralPath 'C:\SDKs\ASIOSDK\host' -Force | Select-Object Name,FullName,Mode | Format-Table -AutoSize

Write-Output ""
Write-Output "Searching for .sln and .vcxproj files (recursive)"
$slns = Get-ChildItem -LiteralPath 'C:\SDKs\ASIOSDK\host' -Filter '*.sln' -Recurse -ErrorAction SilentlyContinue
$projs = Get-ChildItem -LiteralPath 'C:\SDKs\ASIOSDK\host' -Filter '*.vcxproj' -Recurse -ErrorAction SilentlyContinue

Write-Output ""
Write-Output ("Found SLN files: " + ($slns.Count))
if ($slns) { $slns | Select-Object -First 20 FullName | ForEach-Object { Write-Output $_.FullName } }

Write-Output ""
Write-Output ("Found VCXPROJ files: " + ($projs.Count))
if ($projs) { $projs | Select-Object -First 20 FullName | ForEach-Object { Write-Output $_.FullName } }
