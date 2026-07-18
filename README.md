# android-selinux-disabler

Standalone extraction of the SELinux-disabling stage from the [CVE-2026-43074](https://github.com/NebuSec/CyberMeowfia/tree/main/security-research/Ndays/Android-CVE-2026-43074) eventpoll UAF exploit. It races the `eventpoll` loop-depth check to write a zero into `selinux_state.enforcing`, switching the device to Permissive via a single-byte kernel write. This is the MAC-disabling primitive only — no privilege escalation.

> [!WARNING]
> Kernel exploit proof of concept. May crash the kernel, corrupt data, or leave the device inconsistent. Run only on devices you own or are authorized to test. Back up first.

## Origin

Trimmed from the full PoC by NebuSec ([source tree](https://github.com/NebuSec/CyberMeowfia/tree/main/security-research/Ndays/Android-CVE-2026-43074)). Upstream fixed the bug by deferring the eventpoll free to an RCU grace period ([commit `07712db8`](https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/commit/?id=07712db80857d5d09ae08f3df85a708ecfc3b61f)).

## Target

Pinned to one exact build — the enforcing byte is reached through a hardcoded KVA alias and the race depends on that kernel's slab layout:

- **Device:** Pixel 10 Pro / blazer
- **Kernel:** `6.6.118-android15-8-g53e6e091166e-ab15266607-4k`
- **Fingerprint:** `google/blazer/blazer:17/CP2A.260705.006/15641320:user/release-keys`
- **Alias:** `TARGET_SELINUX_ENFORCING_ALIAS = 0xffffff800236a2e0`

Re-targeting another build requires recomputing the alias (image offset of `selinux_state` + mapping base + load address) in [`src/target.h`](src/target.h).

## Build

Needs the Android NDK with the aarch64 clang toolchain. Defaults to `NDK_ROOT=/opt/android-ndk`, `API=35`:

```
make            # → build/disabler
```

Override as needed:

```
make API=35 NDK_ROOT=/path/to/android-ndk
```

## Run

Push `build/disabler` to the device and run as the `shell` user (uid 2000). SELinux must be Enforcing at start; the binary skips otherwise. On success:

```
SELINUX_AFTER Permissive
RESULT PASS selinux_zero attempt=N
```

Verify with `getenforce` → `Permissive`.

## Scope

The original exploit adds, on top of this primitive, a pipe-buffer `flags` redirect (DirtyPipe-style write into a read-only page-cache page) that patches `/system/bin/dumpstate` to spawn a root daemon. Those stages (`pipe.c`, `patch.c`, `su.c`, `payloads.S`, and the original `main.c` orchestration) are dropped here, and `late_refs.c` is reduced to its zero-byte-redirect path. Disabling SELinux alone does not grant root — it only removes MAC enforcement so the carrier payload can run unconfined.
