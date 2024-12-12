#include "SeisISU.h"
#include "SeisOSegy.h"
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
        SeisOSegy *osgy = seis_osegy_new();
        if (!osgy)
                return 1;
        SeisSegyErr const *oerr = seis_osegy_get_error(osgy);
        SeisTrace *trc = NULL;
        seis_isu_open(isgy, argv[1]);
        if (ierr->code)
                goto error;
        char const *tmp_suffix = "_tmp_output_sgy";
        size_t size = strlen(tmp_suffix) + strlen(argv[1]) + 1;
        tmp_name = (char *)malloc(size);
        if (!tmp_name)
                goto error;
        strcpy(tmp_name, argv[1]);
        strcat(tmp_name, tmp_suffix);
        seis_osegy_open(osgy, tmp_name);
        if (oerr->code)
                goto error;
        while (!seis_isu_end_of_data(isgy)) {
                trc = seis_isu_read_trace(isgy);
                if (ierr->code)
                        goto error;
                seis_osegy_write_trace(osgy, trc);
                if (oerr->code)
                        goto error;
                seis_trace_unref(&trc);
        }
        seis_isu_unref(&isgy);
        seis_osegy_unref(&osgy);
        /*remove(tmp_name);*/
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
        seis_osegy_unref(&osgy);
        if (tmp_name)
                free(tmp_name);
        return 1;
}
