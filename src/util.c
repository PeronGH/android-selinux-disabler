#include "common.h"

#include <fcntl.h>
#include <unistd.h>

int selinux_enforcing(void)
{
	char value;
	int fd = open("/sys/fs/selinux/enforce", O_RDONLY | O_CLOEXEC);

	if (fd < 0)
		return -1;
	if (read(fd, &value, 1) != 1) {
		close(fd);
		return -1;
	}
	close(fd);
	return value == '1';
}
