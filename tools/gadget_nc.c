#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define KCOV_ENABLED

#ifdef KCOV_ENABLED
#include "kcov.h"
#endif

void server_loop()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("cannot create socket");
		exit(1);
	}
	struct sockaddr_in addr = {
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(7000),
		.sin_family = AF_INET,
	};
	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr))) {
		perror("cannot bind");
		exit(1);
	}
	fprintf(stdout, "binded\n");

	if (listen(fd, 1) == -1) {
		perror("cannot listen");
		exit(1);
	}

	char buf[256] = {};
	int client = -1;

	while ((client = accept(fd, NULL, NULL)) != -1) {
		fprintf(stdout, "Connected\n");
		int msg_len = -1;
		while ((msg_len = read(client, buf, sizeof(buf))) > 0) {
			write(client, buf, msg_len);
			write(STDOUT_FILENO, buf, msg_len);
		}
		break;
	}
}

int main(int argc, const char **argv)
{
	#ifdef KCOV_ENABLED
	kcov_init();
	kcov_reset();
	#endif

	// read(-1, NULL, 0);
	server_loop();
	// int x;
	// scanf("%d", &x);
	// sleep(2);

	#ifdef KCOV_ENABLED
	unsigned long n = kcov_tracked();
	printf("n = %lu\n", n);
	for (unsigned long i = 1; i <= n; ++i) {
		fprintf(stderr, "0x%lx\n", kcov_ctx.cover[i]);
	}
	kcov_free();
	#endif

	return 0;
}
