#!/usr/bin/env python3
"""Renders test-results.xml (the junit-flavoured QtTest report
run-unit-tests.py's "run" subcommand writes) as a per-suite pass/fail table,
plus any failing test names/messages, appended to $GITHUB_STEP_SUMMARY.
Reporting only: always exits 0, so a missing or unparsable results file just
shows up in the summary instead of failing this step.
"""

import os
import sys
import xml.etree.ElementTree as ET

RESULTS_PATH = "test-results.xml"


def read_suites ( path ):
    root = ET.parse ( path ).getroot()
    return [ root ] if root.tag == "testsuite" else list ( root.findall ( "testsuite" ) )


def write_summary ( summary_path, lines ):
    if not summary_path:
        sys.stdout.writelines ( lines )
        return

    with open ( summary_path, "a", encoding="utf-8" ) as f:
        f.writelines ( lines )


def main():
    job_name = os.environ.get ( "JOB_NAME", "unit tests" )
    summary_path = os.environ.get ( "GITHUB_STEP_SUMMARY" )

    lines = [ "### JUnit results: {}\n\n".format ( job_name ) ]

    suites = None
    if not os.path.exists ( RESULTS_PATH ):
        lines.append ( "_No {} was produced (the build or test run likely failed before it could be written)._\n\n".format ( RESULTS_PATH ) )
    else:
        try:
            suites = read_suites ( RESULTS_PATH )
        except ET.ParseError as e:
            lines.append ( "_{} could not be parsed ({})._\n\n".format ( RESULTS_PATH, e ) )

    if suites is None:
        write_summary ( summary_path, lines )
        return

    lines.append ( "| Suite | Tests | Passed | Failed | Skipped | Time (s) |\n" )
    lines.append ( "|---|---|---|---|---|---|\n" )

    failures = []

    for suite in suites:
        tests = int ( suite.get ( "tests", 0 ) )
        failed = int ( suite.get ( "failures", 0 ) ) + int ( suite.get ( "errors", 0 ) )
        skipped = int ( suite.get ( "skipped", 0 ) )
        passed = tests - failed - skipped
        status = "PASS" if failed == 0 else "FAIL"

        lines.append (
            "| {} {} | {} | {} | {} | {} | {} |\n".format ( status, suite.get ( "name" ), tests, passed, failed, skipped, suite.get ( "time", "?" ) )
        )

        for testcase in suite.findall ( "testcase" ):
            node = testcase.find ( "failure" )
            if node is None:
                node = testcase.find ( "error" )
            if node is not None:
                failures.append ( ( testcase.get ( "name" ), ( node.get ( "message" ) or "" ).strip() ) )

    if failures:
        lines.append ( "\n<details><summary>Failed tests</summary>\n\n" )
        for name, message in failures:
            lines.append ( "- `{}`: {}\n".format ( name, message ) )
        lines.append ( "\n</details>\n" )

    lines.append ( "\n" )

    write_summary ( summary_path, lines )


if __name__ == "__main__":
    main()
