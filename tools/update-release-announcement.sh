#!/bin/bash -eu
python3 -m venv /tmp/release-annoucement.venv
source /tmp/release-annoucement.venv/bin/activate
pip install ollama
python tools/release-announcement.py "$@"
