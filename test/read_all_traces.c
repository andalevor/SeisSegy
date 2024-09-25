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
	SeisTrace *trc = NULL;
	seis_isegy_open(sgy, argv[1]);
	if (err->code)
		goto error;
	long long counter = 0;
	while (!seis_isegy_end_of_data(sgy)) {
		trc = seis_isegy_read_trace(sgy);
		if (err->code)
			goto error;
		long long const *num = seis_trace_header_get_int(
			seis_trace_get_header_const(trc), "TRC_SEQ_LINE");
		if (!num)
			goto error;
		if (*num != ++counter)
			goto error;
		seis_trace_unref(trc);
	}
	seis_isegy_unref(sgy);
	return 0;
error:
	if (trc)
		seis_trace_unref(trc);
	printf("%s\n", err->message);
	seis_isegy_unref(sgy);
	return 1;
}
