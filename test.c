#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <papi.h>

void main(int argc, char **argv)
{
	long long hwc_values[2] = {0, 0};

	PAPI_start(0);

	int i = 0;
	int a = 4;

	for (i=0; i < 10; i ++)
	{
		a += a;
	}

	PAPI_read(0, hwc_values);

	fprintf(stdout, "instret = %lld\n", hwc_values[0]);
	fprintf(stdout, "cycle = %lld\n", hwc_values[1]);

	PAPI_reset(0);

	for (i=0; i < 10; i ++)
	{
		a += a;
	}

	PAPI_accum(0, hwc_values);

	fprintf(stdout, "instret = %lld\n", hwc_values[0]);
	fprintf(stdout, "cycle = %lld\n", hwc_values[1]);
}
