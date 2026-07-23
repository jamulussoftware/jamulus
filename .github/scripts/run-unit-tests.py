#!/usr/bin/env python3
"""Cross-platform build+run driver for the protocol unit test suite.

Usage:
  python3 run-unit-tests.py build
    qmake + make/nmake, out-of-tree in build-test/. On Windows this also
    bootstraps the MSVC environment first (see apply_msvc_environment()):
    build and run are separate GitHub Actions steps (each a fresh shell), so
    that bootstrap has to happen again here rather than once in the workflow.

  python3 run-unit-tests.py run
    Runs the built binary, writes test-output.txt and test-results.xml,
    prints the text report, then gates on test-results.xml: exits nonzero
    unless it reports at least one test and zero failures/errors. The
    binary's own exit code is ignored (see cmd_run()).

Reads from the environment (set by the workflow, from the job's matrix):
  QMAKE_BIN         -- qmake executable name (default "qmake")
  QMAKE_EXTRA_ARGS  -- extra qmake command line arguments, shell-quoted as one
                       string (e.g. QMAKE_CXXFLAGS+="--coverage"); split with
                       shlex before passing to qmake
"""

import os
import platform
import re
import shlex
import subprocess
import sys
import xml.etree.ElementTree as ET

BUILD_DIR = "build-test"
RESULTS_PATH = "test-results.xml"
OUTPUT_PATH = "test-output.txt"


def qmake_extra_args():
    return shlex.split ( os.environ.get ( "QMAKE_EXTRA_ARGS", "" ) )


def run ( cmd, **kwargs ):
    print ( "+ " + " ".join ( cmd ) )
    subprocess.run ( cmd, check=True, **kwargs )


def msvc_environment():
    """Returns the environment variables vcvarsall.bat x64 adds/changes, by
    diffing `cmd /c set` before and after calling it -- the same env-diffing
    technique windows/deploy_windows.ps1's Initialize-Build-Environment uses
    for the same purpose."""
    vswhere = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    vs_path = subprocess.run (
        [
            vswhere,
            "-latest",
            "-prerelease",
            "-products",
            "*",
            "-requires",
            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "-property",
            "installationPath",
        ],
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    vcvarsall = os.path.join ( vs_path, "VC", "Auxiliary", "Build", "vcvarsall.bat" )

    # subprocess.run ( ..., shell=True ) on Windows already runs the string
    # through "%COMSPEC% /c <string>" itself, so cmd is passed as raw command
    # text here, not prefixed with a literal "cmd /c" -- doing that doubles up
    # the cmd.exe nesting and silently swallows vcvarsall.bat's effect on the
    # "after" snapshot (before == after, so no variables get applied).
    def snapshot ( cmd ):
        result = subprocess.run ( cmd, shell=True, capture_output=True, text=True )
        if result.returncode != 0:
            sys.exit ( "command failed (exit {}): {}\nstdout:\n{}\nstderr:\n{}".format ( result.returncode, cmd, result.stdout, result.stderr ) )

        env = {}
        for line in result.stdout.splitlines():
            m = re.match ( r"^([^=]+)=(.*)$", line )
            if m:
                env[m.group ( 1 )] = m.group ( 2 )
        return env

    before = snapshot ( "set" )
    after = snapshot ( 'call "{}" x64 >nul && set'.format ( vcvarsall ) )

    return { name: value for name, value in after.items() if before.get ( name ) != value }


def apply_msvc_environment():
    for name, value in msvc_environment().items():
        os.environ[name] = value

    print ( "VCToolsInstallDir={}".format ( os.environ.get ( "VCToolsInstallDir", "" ) ) )


def cmd_build():
    os.makedirs ( BUILD_DIR, exist_ok=True )
    qmake_bin = os.environ.get ( "QMAKE_BIN", "qmake" )

    if platform.system() == "Windows":
        apply_msvc_environment()
        run (
            [ qmake_bin, "..\\src\\test\\test.pro", "CONFIG-=debug_and_release", "CONFIG+=release", "DESTDIR=." ],
            cwd=BUILD_DIR,
        )
        run ( [ "nmake" ], cwd=BUILD_DIR )
    else:
        run ( [ qmake_bin, "../src/test/test.pro" ] + qmake_extra_args(), cwd=BUILD_DIR )
        run ( [ "make", "-j{}".format ( os.cpu_count() or 1 ) ], cwd=BUILD_DIR )


def test_binary_path():
    name = "jamulus-test.exe" if platform.system() == "Windows" else "jamulus-test"
    return os.path.join ( BUILD_DIR, name )


def pick_junit_format ( binary ):
    # QtTest's junit-flavoured logger is called "xunitxml" on Qt 5.15 and was
    # renamed "junitxml" on Qt 6.x -- ask the binary itself via -help instead
    # of hardcoding it by Qt version, so this keeps working if the name
    # changes again.
    help_text = subprocess.run ( [ binary, "-help" ], capture_output=True, text=True ).stdout
    return "junitxml" if "junitxml" in help_text else "xunitxml"


def read_suites ( path ):
    root = ET.parse ( path ).getroot()
    return [ root ] if root.tag == "testsuite" else list ( root.findall ( "testsuite" ) )


def gate_on_results():
    if not os.path.exists ( RESULTS_PATH ):
        return "gate failed: no {} was produced".format ( RESULTS_PATH )

    try:
        suites = read_suites ( RESULTS_PATH )
    except ET.ParseError as e:
        return "gate failed: {} could not be parsed ({})".format ( RESULTS_PATH, e )

    total_tests = sum ( int ( suite.get ( "tests", 0 ) ) for suite in suites )
    total_failed = sum ( int ( suite.get ( "failures", 0 ) ) + int ( suite.get ( "errors", 0 ) ) for suite in suites )

    if total_tests == 0:
        return "gate failed: {} reported 0 tests".format ( RESULTS_PATH )

    if total_failed != 0:
        return "gate failed: {} of {} tests failed".format ( total_failed, total_tests )

    print ( "gate passed: {} tests, 0 failures".format ( total_tests ) )
    return None


def cmd_run():
    binary = test_binary_path()
    junit_format = pick_junit_format ( binary )
    print ( "Using QtTest JUnit logger format: " + junit_format )

    # remove stale reports so the gate below can only ever see this run's
    # output, even if the binary crashes before writing anything
    for path in [ RESULTS_PATH, OUTPUT_PATH ]:
        if os.path.exists ( path ):
            os.remove ( path )

    # "-o file,txt" not "-o -,txt": on windows-latest runners this binary's
    # stdout comes back 0 bytes regardless of capture method (suspected: Qt's
    # console detection on the inherited handle); QtTest's own file writer
    # works there. Using it on Unix too avoids a platform branch.
    subprocess.run ( [ binary, "-o", OUTPUT_PATH + ",txt", "-o", RESULTS_PATH + "," + junit_format ] )

    if os.path.exists ( OUTPUT_PATH ):
        with open ( OUTPUT_PATH, encoding="utf-8", errors="replace" ) as f:
            sys.stdout.write ( f.read() )

    # The binary's own exit code is unreliable on Windows runners, so it's
    # ignored on every platform; test-results.xml is the sole source of truth.
    error = gate_on_results()
    if error:
        sys.exit ( error )


def main():
    if len ( sys.argv ) != 2 or sys.argv[1] not in ( "build", "run" ):
        sys.exit ( "usage: run-unit-tests.py <build|run>" )

    if sys.argv[1] == "build":
        cmd_build()
    else:
        cmd_run()


if __name__ == "__main__":
    main()
