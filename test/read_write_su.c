#include "SeisISU.h"
#include "SeisOSU.h"
#include <SeisTrace.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
        char *tmp_name = NULL;
        if (argc < 2)
                return 1;
        SeisISU *isgy = seis_isu_new();
        if (!isgy)
                return 1;
        SeisSegyErr const *ierr = seis_isu_get_error(isgy);
        SeisOSU *osgy = seis_osu_new();
        if (!osgy)
                return 1;
        SeisSegyErr const *oerr = seis_osu_get_error(osgy);
        SeisTrace *trc = NULL;
        seis_isu_open(isgy, argv[1]);
        if (ierr->code)
                goto error;
        char const *tmp_suffix = "_tmp_output_su";
        size_t size = strlen(tmp_suffix) + strlen(argv[1]) + 1;
        tmp_name = (char *)malloc(size);
        if (!tmp_name)
                goto error;
        strcpy(tmp_name, argv[1]);
        strcat(tmp_name, tmp_suffix);
        seis_osu_open(osgy, tmp_name);
        if (oerr->code)
                goto error;
        while (!seis_isu_end_of_data(isgy)) {
                trc = seis_isu_read_trace(isgy);
                if (ierr->code)
                        goto error;
                seis_osu_write_trace(osgy, trc);
                if (oerr->code)
                        goto error;
                seis_trace_unref(&trc);
        }
        seis_isu_unref(&isgy);
        seis_osu_unref(&osgy);
        FILE *orig_file = fopen(argv[1], "rb");
        if (!orig_file)
                goto error;
        FILE *test_file = fopen(tmp_name, "rb");
        if (!test_file)
                goto error;
        int orig = 0, test = 0;
        size_t counter = 0;
        do {
                orig = fgetc(orig_file);
                test = fgetc(test_file);
                ++counter;
                if (orig != test) {
                        printf("Not equal: %zd\nOrig: %d, Test: %d\n", counter,
                               orig, test);
                        goto error;
                }
        } while (orig != EOF);
        fclose(orig_file);
        fclose(test_file);
        remove(tmp_name);
        free(tmp_name);
        return 0;
error:
        seis_trace_unref(&trc);
        if (isgy && osgy) {
                if (ierr->code)
                        printf("%s\n", ierr->message);
                else
                        printf("%s\n", oerr->message);
        }
        seis_isu_unref(&isgy);
        seis_osu_unref(&osgy);
        if (tmp_name)
                free(tmp_name);
        return 1;
}
