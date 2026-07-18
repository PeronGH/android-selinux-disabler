# android-selinux-disabler

Standalone extraction of the SELinux-disabling stage from the
CVE-2026-43074 eventpoll UAF exploit. It races the `eventpoll`
loop-depth check to write a zero into `selinux_state.enforcing`,
switching the device from Enforcing to Permissive via a single-byte
arbitrary-write kernel primitive. No privilege escalation is performed
here — this is only the MAC-disabling primitive.

> Warning
>
> This is a kernel exploit proof of concept. It may crash the kernel,
> corrupt data, or leave the device in an inconsistent state. Run it
> only on devices you own or are explicitly authorized to test. Back up
> first. Use at your own risk.

## Origin

Extracted and trimmed from the full CVE-2026-43074 proof of concept by
NebuSec:

<https://github.com/NebuSec/CyberMeowfia/tree/main/security-research/Ndays/Android-CVE-2026-43074>

The upstream bug was fixed by deferring the eventpoll free until an RCU
grace period (commit `07712db80857d5d09ae08f3df85a708ecfc3b61f`).

## Target

Pinned to one exact Pixel 10 Pro build because the SELinux enforcing
byte is reached through a hardcoded KVA alias and the race depends on
the slab layout of that kernel:

- Device: Pixel 10 Pro / blazer
- Kernel: `6.6.118-android15-8-g53e6e091166e-ab15266607-4k`
- Fingerprint: `google/blazer/blazer:17/CP2A.260705.006/15641320:user/release-keys`
- Alias: `TARGET_SELINUX_ENFORCING_ALIAS = 0xffffff800236a2e0`

Re-targeting requires recomputing the enforcing alias for the new
build (physical load offset + KASLR slide) in `src/target.h`.

## Build

Requires the Android NDK with the aarch64 clang toolchain. The
Makefile defaults to `NDK_ROOT = /opt/android-ndk` and `API = 35`:

```
make            # produces build/disabler
```

Override the API level or NDK location as needed:

```
make API=35 NDK_ROOT=/path/to/android-ndk
```

## Run

Push `build/disabler` to the device and run it as the `shell` user
(uid 2000). SELinux must be Enforcing at start; the binary skips
otherwise. On success it prints:

```
SELINUX_AFTER Permissive
RESULT PASS selinux_zero attempt=N
```

Confirm after running with `getenforce` (prints `Permissive`).

## What was dropped from the original

The full CVE-2026-43074 exploit adds, on top of this primitive, a
pipe-buffer `flags` redirect (DirtyPipe-style write into a read-only
page-cache page) used to patch `/system/bin/dumpstate`, which then
re-execs a root daemon. Those stages (`pipe.c`, `patch.c`, `su.c`,
`payloads.S`, and the orchestration in the original `main.c`) are not
included here, and the `late_refs.c` race helper was reduced to its
zero-byte-redirect path (the `late_refs_pipe` / pipe-`flags` branch and
its `PIPE_*` constants were removed). `target.h` keeps only the SELinux
enforcing alias; `common.h` keeps only the offsets/declarations the
zero path uses; `util.c` keeps only `selinux_enforcing`.

Disabling SELinux alone does not grant root — it only removes MAC
enforcement so that the later carrier-context payload can run
unconfined.
