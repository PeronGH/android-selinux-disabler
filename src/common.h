#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

enum {
	EXPLOIT_PAGE_SIZE = 4096,
	CORE = 0,
};

int selinux_enforcing(void);

void known_page_prepare(unsigned char *data);
uint64_t known_page_acquire(void);
void known_page_reclaim(int carrier_sv[2]);
unsigned char *known_page_data(void);

int late_refs_zero(uint64_t target_byte);
