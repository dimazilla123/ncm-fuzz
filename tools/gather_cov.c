#include "kcov.h"

int main(int argc, char **argv)
{
	kcov_init();
	kcov_reset();

	int x;
	scanf("%d", &x);

	kcov_dump_adresses();
	kcov_free();
	return 0;
}
