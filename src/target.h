#pragma once

/*
 * Pixel 10 Pro / blazer, build CP2A.260705.006 /15641320
 *   fingerprint: google/blazer/blazer:17/CP2A.260705.006/15641320:user/release-keys
 *   kernel:      6.6.118-android15-8-g53e6e091166e-ab15266607-4k
 *
 * Pixel 10 hardware: PHYS_OFFSET == KERNEL_PHYS_LOAD == 0x80000000.
 * selinux_state.enforcing is reached through this alias; re-targeting a
 * different build/device requires recomputing it (see README).
 */
#define TARGET_SELINUX_ENFORCING_ALIAS UINT64_C(0xffffff800236a2e0)
