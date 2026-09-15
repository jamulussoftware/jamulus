# The following version pinnings are semi-automatically checked for updates.
# Verify .github/workflows/bump-dependencies.yml when changing those manually:

# Values are consumed by .github/autobuild/windows.ps1, windows/deploy_windows.ps1 and the dependency cache key.
$Qt32Version = "5.15.2"
$Qt64Version = "6.10.2"
$QtCompile32 = "msvc2019"
$QtCompile64 = "msvc2022"
$AqtinstallVersion = "3.3.0"
$JackVersion = "1.9.22"
$JomVersion = "1.1.2"

# Important:
# - Do not update ASIO SDK without checking for license-related changes.
# - Do not copy (parts of) the ASIO SDK into the Jamulus source tree without
#   further consideration as it would make the license situation more complicated.
$AsioSDKVersion = "ASIO-SDK_2.3.4_2025-10-15"

$NsisVersion = "3.12"
