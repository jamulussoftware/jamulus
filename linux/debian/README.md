# Debian packaging

Author(s):
* The Jamulus Development Team

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see [<https://www.gnu.org/licenses/>](https://www.gnu.org/licenses/).

---

## Files in this folder

`linux/deploy_deb.sh` (run from the repository root) uses this directory to build two packages with `debuild`:

- **jamulus** — the desktop client/server (built with `CONFIG+=noupcasename`, which renames the target from `Jamulus` to `jamulus`).
- **jamulus-headless** — a server-only binary (`CONFIG+=headless serveronly` with `TARGET=jamulus-headless`, so `noupcasename` is not needed) with no GUI library dependencies, plus a systemd service.

Most files here are standard Debian packaging (`control`, `rules`, `copyright`, `*.install`, …). `changelog` is regenerated at build time from the top-level `ChangeLog` — don't edit it by hand.

## jamulus-headless.service

Installed to `lib/systemd/system` by the headless package. It runs `/usr/bin/jamulus-headless -s -n` as the unprivileged `jamulus` system user (created in `jamulus-headless.postinst`), restarts on failure, and raises CPU and I/O scheduling priority for real-time audio.

Start it with `sudo systemctl enable --now jamulus-headless`. To name, publish or otherwise configure the server, adjust the `ExecStart=` options — see https://jamulus.io/wiki/Running-a-Server#server-mode-related-options.
