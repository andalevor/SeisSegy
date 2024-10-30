#include "SeisISegy.h"
#include "SeisOSegyRev0.h"

int main(int argc, char *argv[])
{
	if (argc < 2)
		return 1;
	SeisISegy *isgy = seis_isegy_new();
	if (!isgy)
		return 1;
	SeisSegyErr const *ierr = seis_isegy_get_error(isgy);
	SeisOSegyRev0 *osgy = seis_osegy_rev0_new();
	if (!osgy)
		return 1;
	SeisSegyErr const *oerr = seis_osegy_rev0_get_error(osgy);
	SeisTrace *trc = NULL;
	seis_isegy_open(isgy, argv[1]);
	if (ierr->code)
		goto error;
	char const *text_header = seis_isegy_get_text_header(isgy, 0);
	SeisSegyBinHdr const *binary_header = seis_isegy_get_binary_header(isgy);
	seis_osegy_rev0_set_text_header(osgy, text_header);
	seis_osegy_rev0_set_binary_header(osgy, binary_header);
	seis_osegy_rev0_open(osgy, "tmp_rev0");
	if (oerr->code)
		goto error;
	while (!seis_isegy_end_of_data(isgy)) {
		trc = seis_isegy_read_trace(isgy);
		if (ierr->code)
			goto error;
		seis_osegy_rev0_write_trace(osgy, trc);
		if (oerr->code)
			goto error;
		seis_trace_unref(trc);
	}
	seis_isegy_unref(isgy);
	seis_osegy_rev0_unref(osgy);
	FILE *orig_file = fopen(argv[1], "rb");
	if (!orig_file)
		return 1;
	FILE *test_file = fopen("tmp_rev0", "rb");
	if (!test_file)
		return 1;
	int orig = 0, test = 0;
	size_t counter = 0;
	do {
		orig = fgetc(orig_file);
		test = fgetc(test_file);
		++counter;
		if (orig != test) {
			printf("Not equal: %zd\nOrig: %d, Test: %d\n", counter, orig, test);
			return 1;
		}
	} while (orig != EOF);
	remove("tmp_rev0");
	return 0;
error:
	if (trc)
		seis_trace_unref(trc);
	if (ierr->code)
		printf("%s\n", ierr->message);
	else
		printf("%s\n", oerr->message);
	seis_isegy_unref(isgy);
	seis_osegy_rev0_unref(osgy);
	return 1;
}
