#ifndef KCOV_UTILS
#define KCOV_UTILS

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/kcov.h>
#include <stdatomic.h>

#define KCOV_SUBSYSTEM_USB_GADGET (0x02ull << 56)

static const unsigned long COVER_SIZE = 64 << 10;
struct kcov_ctx_t {
	int fd;	
	unsigned long *cover;
	struct kcov_remote_arg *arg;
} kcov_ctx;

static void kcov_init()
{
	kcov_ctx.arg = (struct kcov_remote_arg*)calloc(1, sizeof(*kcov_ctx.arg) + 1 * sizeof(kcov_ctx.arg->handles[0]));
	if (kcov_ctx.arg == NULL) {
		perror("kcov remote arg alloc");
		exit(0);
	}
	
	kcov_ctx.fd = open("/sys/kernel/debug/kcov", O_RDWR);
	if (kcov_ctx.fd == -1) {
		perror("cannot open kcov");
		goto error;
	}

	if (ioctl(kcov_ctx.fd, KCOV_INIT_TRACE, COVER_SIZE)) {
		perror("ioctl trace");
		goto error;
	}

	kcov_ctx.cover = (unsigned long*)mmap(
		NULL,
		COVER_SIZE * sizeof(*kcov_ctx.cover),
		PROT_READ | PROT_WRITE,
		MAP_SHARED,
		kcov_ctx.fd,
		0
	);
	if ((void*)kcov_ctx.cover == MAP_FAILED) {
		perror("cover mmap failed");
		goto error;
	}

	kcov_ctx.arg->trace_mode = KCOV_TRACE_PC; // KCOV_TRACE_PC;
	kcov_ctx.arg->area_size = COVER_SIZE;
	kcov_ctx.arg->common_handle = getpid();// kcov_remote_handle(KCOV_SUBSYSTEM_COMMON, 0x42);
	// kcov_ctx.arg->common_handle = kcov_remote_handle(KCOV_SUBSYSTEM_COMMON, getpid());
	// kcov_ctx.arg->common_handle = kcov_remote_handle(KCOV_SUBSYSTEM_COMMON, 1);
	kcov_ctx.arg->num_handles = 1;
	// kcov_ctx.arg->handles[0] = 0x42;
	kcov_ctx.arg->handles[0] = kcov_remote_handle(KCOV_SUBSYSTEM_USB, 0x42);
	// kcov_ctx.arg->handles[1] = kcov_remote_handle(KCOV_SUBSYSTEM_USB, 2);
	// kcov_ctx.arg->handles[2] = kcov_remote_handle(KCOV_SUBSYSTEM_USB, 3);

	#if 1
	if (ioctl(kcov_ctx.fd, KCOV_REMOTE_ENABLE, kcov_ctx.arg)) {
		perror("kcov remote enable");
		goto error;
	}
	#else

	if (ioctl(kcov_ctx.fd, KCOV_ENABLE, 0)) {
		perror("ioctl enable");
		goto error;
	}
	#endif

	free(kcov_ctx.arg);
	return;

	error:
	free(kcov_ctx.arg);
	exit(0);
}

static void kcov_reset()
{
	__atomic_store_n(&kcov_ctx.cover[0], 0, __ATOMIC_RELAXED);
}
static unsigned long kcov_tracked()
{
	return __atomic_load_n(&kcov_ctx.cover[0], __ATOMIC_RELAXED);
}

static void kcov_free()
{
	if (ioctl(kcov_ctx.fd, KCOV_DISABLE, 0)) {
		perror("ioctl disable");
		exit(1);
	}
	if (munmap(kcov_ctx.cover, COVER_SIZE * sizeof(*kcov_ctx.cover))) {
		perror("munmap");
		exit(1);
	}
	close(kcov_ctx.fd);
}

static void kcov_dump_adresses()
{
	unsigned long n = kcov_tracked();
	for (unsigned long i = 1; i <= n; ++i) {
		fprintf(stderr, "0x%lx\n", kcov_ctx.cover[i]);
	}
}

#endif
