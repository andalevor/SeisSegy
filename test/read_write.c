#include "SeisISegy.h"
#include "SeisOSegy.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
        char *tmp_name = NULL;
        if (argc < 2)
                return 1;
        SeisISegy *isgy = seis_isegy_new();
        if (!isgy)
                return 1;
        SeisSegyErr const *ierr = seis_isegy_get_error(isgy);
        SeisOSegy *osgy = seis_osegy_new();
        if (!osgy)
                return 1;
        SeisSegyErr const *oerr = seis_osegy_get_error(osgy);
        SeisTrace *trc = NULL;
        seis_isegy_open(isgy, argv[1]);
        if (ierr->code)
                goto error;
        char const *text_header = seis_isegy_get_text_header(isgy, 0);
        SeisSegyBinHdr const *binary_header =
            seis_isegy_get_binary_header(isgy);
        seis_osegy_set_text_header(osgy, text_header);
        seis_osegy_set_binary_header(osgy, binary_header);
        char const *tmp_suffix = "_tmp_output_segy";
        size_t size = strlen(tmp_suffix) + strlen(argv[1]) + 1;
        tmp_name = (char *)malloc(size);
        strcpy(tmp_name, argv[1]);
        strcat(tmp_name, tmp_suffix);
        seis_osegy_open(osgy, tmp_name);
        if (oerr->code)
                goto error;
        while (!seis_isegy_end_of_data(isgy)) {
                trc = seis_isegy_read_trace(isgy);
                if (ierr->code)
                        goto error;
                seis_osegy_write_trace(osgy, trc);
                if (oerr->code)
                        goto error;
                seis_trace_unref(trc);
        }
        seis_isegy_unref(isgy);
        seis_osegy_unref(osgy);
        FILE *orig_file = fopen(argv[1], "rb");
        if (!orig_file)
                return 1;
        FILE *test_file = fopen(tmp_name, "rb");
        if (!test_file)
                return 1;
        int orig = 0, test = 0;
        size_t counter = 0;
        do {
                orig = fgetc(orig_file);
                test = fgetc(test_file);
                ++counter;
                if (orig != test) {
                        printf("Not equal: %zd\nOrig: %d, Test: %d\n", counter,
                               orig, test);
                        return 1;
                }
        } while (orig != EOF);
        fclose(orig_file);
        fclose(test_file);
        remove(tmp_name);
        free(tmp_name);
        return 0;
error:
        if (trc)
                seis_trace_unref(trc);
        if (ierr->code)
                printf("%s\n", ierr->message);
        else
                printf("%s\n", oerr->message);
        seis_isegy_unref(isgy);
        seis_osegy_unref(osgy);
        free(tmp_name);
        return 1;
}
