#ifndef SEIS_COMMON_SEGY_PRIVATE
#define SEIS_COMMON_SEGY_PRIVATE

#include "SeisCommonSegy.h"
#include "m-array.h"
#include "m-string.h"
#include "m-tuple.h"

ARRAY_DEF(str_arr, string_t)

TUPLE_DEF2(hdr_fmt, (name, string_t), (offset, int), (format, enum FORMAT))
#define M_OPL_hdr_fmt_t()                                                      \
        TUPLE_OPLIST(hdr_fmt, STRING_OPLIST, M_BASIC_OPLIST,                   \
                     M_ENUM_OPLIST(enum FORMAT, i32))
ARRAY_DEF(single_hdr_fmt, hdr_fmt_t)
#define M_OPL_single_hdr_fmt_t() ARRAY_OPLIST(single_hdr_fmt, M_OPL_hdr_fmt_t())
ARRAY_DEF(mult_hdr_fmt, single_hdr_fmt_t)
#define M_OPL_mult_hdr_fmt_t()                                                 \
        ARRAY_OPLIST(mult_hdr_fmt, M_OPL_single_hdr_fmt_t())

typedef struct SeisCommonSegyPrivate {
        struct SeisCommonSegy com;
        str_arr_t text_hdrs, end_stanzas;
        mult_hdr_fmt_t trc_hdr_map;
} SeisCommonSegyPrivate;

#endif
