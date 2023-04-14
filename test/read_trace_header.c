#include "SeisISegy.h"
#include <SeisTrace.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	if (argc < 2)
		return 1;
	SeisISegy *sgy = seis_isegy_new();
	if (!sgy)
		return 1;
	SeisSegyErr const *err = seis_isegy_get_error(sgy);
	SeisTraceHeader *hdr = NULL;
	seis_isegy_open(sgy, argv[1]);
	if (err->code)
		goto error;
	hdr = seis_isegy_read_trace_header(sgy);
	if (err->code)
		goto error;
	seis_trace_header_unref(hdr);
	seis_isegy_unref(sgy);
	return 0;
error:
	if (hdr)
		seis_trace_header_unref(hdr);
	seis_isegy_unref(sgy);
	printf("%s\n", err->message);
	return 1;
}
