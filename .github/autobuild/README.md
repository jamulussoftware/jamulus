# Scripts for building Jamulus via GitHub Actions

The scripts in this folder are used by the Github autobuild actions in .github/workflows/autobuild.yml.
They are responsible for setting up the Github environment, building the binaries and passing the resulting artifacts back to the workflow.

The scripts may work outside of Github, but haven't been designed or tested for that use case.
See the various platform-specific build scripts in their own folders for standalone builds (e.g. windows/deploy_windows.ps1).
