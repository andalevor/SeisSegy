#include "SeisSegy.h"
#include <SeisTrace.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	if (argc < 2)
		return 1;
	SeisSegy_t sgy = seis_segy_new('r');
	if (!sgy)
		return 1;
	SeisSegyErr_t err = seis_segy_get_error(sgy);
	SeisTrace_t trc = NULL;
	seis_segy_open(sgy, argv[1]);
	if (err->code)
		goto error;
	trc = seis_segy_read_trace(sgy);
	if (err->code)
		goto error;
	seis_trace_unref(trc);
	seis_segy_unref(sgy);
	return 0;
error:
	if (trc)
		seis_trace_unref(trc);
	seis_segy_unref(sgy);
	printf("%s\n", err->message);
	return 1;
}
