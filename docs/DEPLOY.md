### Copyright (c) 2026

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

# Deploying a Jamulus Server

[COMPILING.md](../COMPILING.md) ends when the binary exists. This document covers the step after that: putting a self-built headless server binary on a production host and verifying it actually runs. Most self-inflicted server outages happen in this step, and every rule below corresponds to a real-world failure.

For configuring and operating a server (registration, recording, welcome message, etc.), see the [Server manual](https://jamulus.io/wiki/Running-a-Server).

## Build for the target, not for the build machine

- **CPU architecture must match.** A binary built on x86-64 will not run on an ARM host (and vice versa) — the service crash-loops with `Exec format error`. Before copying, compare `file ./Jamulus` on the build machine with `uname -m` on the target.
- **Build on the oldest OS release you deploy to.** Binaries depend on the glibc/libstdc++ of the build machine. A binary built on Ubuntu 24.04 fails on 22.04 with `GLIBCXX_3.4.32 not found`, while a 22.04 build runs fine on 24.04 and later. Newer hosts run older binaries; the reverse never holds.
- **Check shared libraries after every copy.** `ldd /path/to/jamulus | grep "not found"` must print nothing. This catches a missing Qt runtime package before systemd shows you a crash loop. The minimal headless runtime needs the Qt core, network, concurrent and xml libraries (see [COMPILING.md](../COMPILING.md)).
- **Low-memory hosts:** on machines with ≤ 1 GB RAM, build with `make -j1`, or build on a bigger machine of the same OS/architecture and copy the binary. If you must add temporary swap to survive a build, remove it afterwards — see below.

## Run under systemd

Use the unit shipped in [`linux/debian/jamulus-headless.service`](../linux/debian/jamulus-headless.service) as your starting point — it already encodes hard-won defaults (dedicated `jamulus` user, `Nice=-20`, real-time I/O scheduling, `MemorySwapMax=0`, `Restart=on-failure`). Manage the server only through `systemctl`; never kill the process by PID.

## Host tuning

- **Enlarge the UDP receive buffer.** Under load, default kernel buffers drop packets, which musicians hear as dropouts. Set in `/etc/sysctl.d/99-jamulus.conf`:

  ```
  net.core.rmem_max=4194304
  net.core.rmem_default=4194304
  ```

- **No swap on a live server.** Swapping causes latency spikes audible to everyone connected. Keep swap off (the shipped systemd unit sets `MemorySwapMax=0` for the service; better still, don't enable swap on the host at all).
- **Don't compete with the audio process.** Never compile, or run other CPU-heavy work, on a host while musicians are connected — CPU contention causes dropouts just like network loss does.

## Firewall

- Inbound UDP on the server port (default 22224) must be open. Remember that cloud providers filter *in front of* the host (AWS security groups, OCI security lists) in addition to any host firewall, and some images run `firewalld` by default — you may need to open the port in two places.
- If you enable JSON-RPC (`--jsonrpcport`), never expose that TCP port to the internet. Bind it to localhost or firewall it to specific trusted addresses, and always use `--jsonrpcsecretfile`.
- Corollary: a TCP probe of a firewalled port from outside proves nothing about the service. Verify RPC from an allowed host, not from the internet.

## Verify every deploy

A deploy is finished when all of these pass on the target host — not when the file lands:

```bash
file /usr/bin/jamulus-headless          # architecture matches `uname -m`
ldd /usr/bin/jamulus-headless | grep "not found"   # no output
/usr/bin/jamulus-headless --version     # the version you just built
systemctl status jamulus-headless       # active (running)
```

Then check the restart counter is stable (`systemctl show -p NRestarts jamulus-headless`, again a minute later — a crash loop can look "active" in a single snapshot), and finally do an end-to-end test: if the server is registered with a directory, confirm it appears in the listing, and connect a client to confirm audio passes.

When a change "isn't working" on a server, check the binary's timestamp against the commit you think it contains *before* debugging — a stale binary explains most such mysteries.
