#include "SeisCommonSegy.h"
#include "SeisEncodings.h"
#include "SeisISegy.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void print_text_header(char const *str);

int main(int argc, char *argv[]) {
        if (argc < 2)
                return 1;
        SeisISegy *sgy = seis_isegy_new();
        if (!sgy)
                return 1;
        SeisSegyErr const *err = seis_isegy_get_error(sgy);
        seis_isegy_open(sgy, argv[1]);
        if (err->code)
                goto error;
        char const *hdr = seis_isegy_get_text_header(sgy, 0);
        size_t len = strlen(hdr);
        char *mhdr = (char *)malloc(len + 1);
        strncpy(mhdr, hdr, len + 1);
        ebcdic_to_ascii(mhdr);
        printf("Orig:\n");
        print_text_header(hdr);
        printf("\nConverted:\n");
        print_text_header(mhdr);
        free(mhdr);
        seis_isegy_unref(&sgy);
        return 0;
error:
        seis_isegy_unref(&sgy);
        return 1;
}

void print_text_header(char const *str) {
        for (size_t i = 0; i < TEXT_HEADER_SIZE; ++i) {
                printf("%c", str[i]);
                if (!((i + 1) % 80))
                        printf("\n");
        }
}
