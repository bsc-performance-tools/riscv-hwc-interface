#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <papi.h>

void main(int argc, char **argv)
{
	long long hwc_values[2] = {0, 0};

	int mypapiset = 0;
	PAPI_create_eventset(&mypapiset);
	PAPI_add_event(mypapiset, INST_RETIRED);
	PAPI_add_event(mypapiset, INST_CYCLES);

	PAPI_start(mypapiset);

	int i = 0;
	int a = 4;

	for (i=0; i < 10; i ++)
	{
		a += a;
	}

	PAPI_read(mypapiset, hwc_values);

	fprintf(stdout, "instret = %lld\n", hwc_values[0]);
	fprintf(stdout, "cycle = %lld\n", hwc_values[1]);

	PAPI_reset(mypapiset);

	for (i=0; i < 10; i ++)
	{
		a += a;
	}

	PAPI_accum(mypapiset, hwc_values);

	fprintf(stdout, "instret = %lld\n", hwc_values[0]);
	fprintf(stdout, "cycle = %lld\n", hwc_values[1]);
}
