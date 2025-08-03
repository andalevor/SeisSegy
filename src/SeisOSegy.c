#include "SeisOSegy.h"
#include "SeisCommonSegy.h"
#include "SeisCommonSegyPrivate.h"
#include "SeisEncodings.h"
#include "SeisOSU.h"
#include "TRY.h"
#include "m-string.h"
#include <SeisTrace.h>
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(x) (void)(x)

static SeisSegyErrCode assign_raw_writers(SeisOSegy *sgy);
static SeisSegyErrCode assign_sample_writer(SeisOSegy *sgy);
static SeisSegyErrCode assign_bytes_per_sample(SeisOSegy *sgy);
static SeisSegyErrCode write_text_header(SeisOSegy *sgy);
static SeisSegyErrCode write_bin_header(SeisOSegy *sgy);
static SeisSegyErrCode write_ext_text_headers(SeisOSegy *sgy);
static SeisSegyErrCode write_trailer_stanzas(SeisOSegy *sgy);
static SeisSegyErrCode write_trace_header(SeisOSegy *sgy,
                                          SeisTraceHeader const *hdr);
static SeisSegyErrCode write_trace_samples_fix(SeisOSegy *sgy,
                                               SeisTrace const *t);
static SeisSegyErrCode write_trace_samples_var(SeisOSegy *sgy,
                                               SeisTrace const *t);
static void fill_buf_with_fmt_arr(SeisOSegy *sgy, single_hdr_fmt_t *arr,
                                  SeisTraceHeader const *hdr);
static SeisSegyErrCode write_to_file(SeisOSegy *sgy, char const *buf,
                                     size_t num);

struct SeisOSegy {
        SeisCommonSegy *com;
        void (*write_u8)(char **buf, uint8_t);
        void (*write_i8)(char **buf, int8_t);
        void (*write_u16)(char **buf, uint16_t);
        void (*write_i16)(char **buf, int16_t);
        void (*write_u24)(char **buf, uint32_t);
        void (*write_i24)(char **buf, int32_t);
        void (*write_u32)(char **buf, uint32_t);
        void (*write_i32)(char **buf, int32_t);
        void (*write_u64)(char **buf, uint64_t);
        void (*write_i64)(char **buf, int64_t);
        void (*write_sample)(SeisOSegy *sgy, char **buf, double);
        void (*write_ibm_float)(SeisOSegy *sgy, char **buf, double);
        void (*write_IEEE_float)(SeisOSegy *sgy, char **buf, double);
        void (*write_IEEE_double)(SeisOSegy *sgy, char **buf, double);
        SeisSegyErrCode (*write_add_trc_hdrs)(SeisOSegy *sgy,
                                              SeisTraceHeader const *hdr);
        SeisSegyErrCode (*write_trace_samples)(SeisOSegy *sgy,
                                               SeisTrace const *t);
        SeisSegyErrCode (*write_trace)(SeisOSegy *sgy, SeisTrace const *trc);
        int update_bin_header;
        int rc;
};

struct SeisOSU {
        SeisOSegy *sgy;
        int rc;
};

static void write_i8(char **buf, int8_t val);
static void write_u8(char **buf, uint8_t val);
static void write_i16(char **buf, int16_t val);
static void write_u16(char **buf, uint16_t val);
static void write_i24(char **buf, int32_t val);
static void write_u24(char **buf, uint32_t val);
static void write_i32(char **buf, int32_t val);
static void write_u32(char **buf, uint32_t val);
static void write_i64(char **buf, int64_t val);
static void write_u64(char **buf, uint64_t val);
static void write_i16_sw(char **buf, int16_t val);
static void write_u16_sw(char **buf, uint16_t val);
static void write_i24_sw(char **buf, int32_t val);
static void write_u24_sw(char **buf, uint32_t val);
static void write_i32_sw(char **buf, int32_t val);
static void write_u32_sw(char **buf, uint32_t val);
static void write_i64_sw(char **buf, int64_t val);
static void write_u64_sw(char **buf, uint64_t val);
static void write_IBM_float(SeisOSegy *sgy, char **buf, double val);
static void write_IEEE_float(SeisOSegy *sgy, char **buf, double val);
static void write_IEEE_double(SeisOSegy *sgy, char **buf, double val);
static void write_IEEE_float_native(SeisOSegy *sgy, char **buf, double val);
static void write_IEEE_double_native(SeisOSegy *sgy, char **buf, double val);
static void dbl_to_i8(SeisOSegy *sgy, char **buf, double val);
static void dbl_to_u8(SeisOSegy *sgy, char **buf, double val);
static void dbl_to_i16(SeisOSegy *sgy, char **buf, double val);
static void dbl_to_u16(SeisOSegy *sgy, char **buf, double val);
static void dbl_to_i24(SeisOSegy *sgy, char **buf, double val);
static void dbl_to_u24(SeisOSegy *sgy, char **buf, double val);
static void dbl_to_i32(SeisOSegy *sgy, char **buf, double val);
static void dbl_to_u32(SeisOSegy *sgy, char **buf, double val);
static void dbl_to_i64(SeisOSegy *sgy, char **buf, double val);
static void dbl_to_u64(SeisOSegy *sgy, char **buf, double val);

SeisOSegy *seis_osegy_new(void) {
        SeisOSegy *sgy = (SeisOSegy *)malloc(sizeof(struct SeisOSegy));
        if (!sgy)
                goto error;
        sgy->com = seis_common_segy_new();
        sgy->rc = 1;
        return sgy;
error:
        return NULL;
}

SeisOSegy *seis_osegy_ref(SeisOSegy *sgy) {
        ++sgy->rc;
        return sgy;
}

void seis_osegy_unref(SeisOSegy **sgy) {
        if (*sgy)
                if (!--(*sgy)->rc) {
                        if (!(*sgy)->com->err.code) {
                                SeisCommonSegy *com = (*sgy)->com;
                                if (com->bin_hdr.SEGY_rev_major_ver > 1)
                                        write_trailer_stanzas(*sgy);
                                /* update samp_per_tr value for variable
                                 * trace length SEGYs */
                                if ((*sgy)->update_bin_header) {
                                        com->bin_hdr.samp_per_tr =
                                            com->bin_hdr.ext_samp_per_tr =
                                                com->samp_per_tr;
                                        fseek(com->file, TEXT_HEADER_SIZE,
                                              SEEK_SET);
                                        write_bin_header(*sgy);
                                }
                        }
                        seis_common_segy_unref(&(*sgy)->com);
                        free(*sgy);
                        *sgy = NULL;
                }
}

SeisSegyErr const *seis_osegy_get_error(SeisOSegy const *sgy) {
        return &sgy->com->err;
}

void seis_osegy_set_text_header(SeisOSegy *sgy, char const *hdr) {
        assert(strlen(hdr) == TEXT_HEADER_SIZE);
        SeisCommonSegy *com = sgy->com;
        seis_common_segy_add_text_header(com, hdr);
}

void seis_osegy_set_binary_header(SeisOSegy *sgy, SeisSegyBinHdr const *bh) {
        sgy->com->bin_hdr = *bh;
}

SeisSegyErrCode seis_osegy_open(SeisOSegy *sgy, char const *file_name) {
        SeisCommonSegy *com = sgy->com;
        com->file = fopen(file_name, "wb");
        if (!com->file) {
                com->err.code = SEIS_SEGY_ERR_FILE_OPEN;
                com->err.message = "can't open file for writing";
                goto error;
        }
        if (!com->bin_hdr.format_code)
                com->bin_hdr.format_code = 1;
        TRY(write_text_header(sgy));
        TRY(assign_raw_writers(sgy));
        TRY(assign_bytes_per_sample(sgy));
        TRY(assign_sample_writer(sgy));
        TRY(write_bin_header(sgy));
        com->samp_per_tr = com->bin_hdr.ext_samp_per_tr
                               ? com->bin_hdr.ext_samp_per_tr
                               : com->bin_hdr.samp_per_tr;
        if (com->samp_per_tr)
                com->samp_buf =
                    (char *)malloc(com->samp_per_tr * com->bytes_per_sample);
        if (com->samp_per_tr && !com->samp_buf) {
                com->err.code = SEIS_SEGY_ERR_NO_MEM;
                com->err.message = "can't allocate memory in osegy open";
                goto error;
        }
        if ((com->bin_hdr.fixed_tr_length ||
             !com->bin_hdr.SEGY_rev_major_ver) &&
            com->samp_per_tr) {
                sgy->write_trace_samples = write_trace_samples_fix;
                sgy->update_bin_header = 0;
        } else {
                sgy->write_trace_samples = write_trace_samples_var;
                sgy->update_bin_header = 1;
        }
        if (com->bin_hdr.SEGY_rev_major_ver)
                TRY(write_ext_text_headers(sgy));
error:
        return com->err.code;
}

SeisSegyErrCode seis_osegy_remap_trace_header(SeisOSegy *sgy,
                                              char const *hdr_name, int hdr_num,
                                              int offset, enum FORMAT fmt) {
        return seis_common_remap_trace_header(sgy->com, hdr_name, hdr_num,
                                              offset, fmt);
}

void seis_osegy_add_trailer_stanza(SeisOSegy *sgy, char *buf) {
        SeisCommonSegy *com = sgy->com;
        assert(strlen(buf) == TEXT_HEADER_SIZE);
        seis_common_segy_add_stanza(com, buf);
}

void seis_osegy_add_ext_text_header(SeisOSegy *sgy, char *hdr) {
        SeisCommonSegy *com = sgy->com;
        assert(strlen(hdr) == TEXT_HEADER_SIZE);
        if (!seis_common_segy_get_text_headers_num(com)) {
                char const *hdr;
                switch (com->bin_hdr.SEGY_rev_major_ver) {
                case 0:
                default:
                        hdr = seis_segy_default_text_header_rev0;
                        break;
                case 1:
                        hdr = seis_segy_default_text_header_rev1;
                        break;
                case 2:
                        hdr = seis_segy_default_text_header_rev2;
                        break;
                }
                seis_common_segy_add_text_header(com, hdr);
        }
        seis_common_segy_add_text_header(com, hdr);
}

SeisSegyErrCode seis_osegy_write_trace(SeisOSegy *sgy, SeisTrace const *trc) {
        SeisSegyErr const *err = seis_osegy_get_error(sgy);
        TRY(write_trace_header(sgy, seis_trace_get_header_const(trc)));
        return sgy->write_trace_samples(sgy, trc);
error:
        return err->code;
}

SeisOSU *seis_osu_new(void) {
        SeisOSU *su = (SeisOSU *)malloc(sizeof(struct SeisOSU));
        if (!su)
                goto error;
        su->sgy = seis_osegy_new();
        su->rc = 1;
        su->sgy->update_bin_header = 0;
        return su;
error:
        return NULL;
}

SeisOSU *seis_osu_ref(SeisOSU *su) {
        ++su->rc;
        return su;
}

void seis_osu_unref(SeisOSU **su) {
        if (*su)
                if (!--(*su)->rc) {
                        seis_osegy_unref(&(*su)->sgy);
                        free(*su);
                        *su = NULL;
                }
}

SeisSegyErr const *seis_osu_get_error(SeisOSU const *su) {
        return &su->sgy->com->err;
}

SeisSegyErrCode seis_osu_remap_trace_header(SeisOSU *su, char const *hdr_name,
                                            int offset, enum FORMAT fmt) {
        return seis_osegy_remap_trace_header(su->sgy, hdr_name, 1, offset, fmt);
}

SeisSegyErrCode seis_osu_open(SeisOSU *su, char const *file_name) {
        SeisOSegy *sgy = su->sgy;
        SeisCommonSegy *com = sgy->com;
        com->file = fopen(file_name, "wb");
        if (!com->file) {
                com->err.code = SEIS_SEGY_ERR_FILE_OPEN;
                com->err.message = "can't open file for writing";
                goto error;
        }
#ifndef SU_BIG_ENDIAN
        com->bin_hdr.endianness = 0x01020304;
#endif
        TRY(assign_raw_writers(sgy));
        com->bytes_per_sample = 4;
        com->bin_hdr.format_code = 1;
        sgy->write_sample = write_IEEE_float_native;
        com->samp_per_tr = 0;
        com->samp_buf = NULL;
        sgy->write_trace_samples = write_trace_samples_var;
        sgy->update_bin_header = 0;
error:
        return com->err.code;
}

SeisSegyErrCode seis_osu_write_trace(SeisOSU *su, SeisTrace const *trc) {
        SeisSegyErr const *err = seis_osu_get_error(su);
        TRY(write_trace_header(su->sgy, seis_trace_get_header_const(trc)));
        return su->sgy->write_trace_samples(su->sgy, trc);
error:
        return err->code;
}

SeisSegyErrCode write_to_file(SeisOSegy *sgy, char const *buf, size_t num) {
        SeisCommonSegy *com = sgy->com;
        size_t written = fwrite(buf, 1, num, com->file);
        if (written != num) {
                com->err.code = SEIS_SEGY_ERR_FILE_WRITE;
                com->err.message = "written less bytes than should";
        }
        return com->err.code;
}

SeisSegyErrCode assign_raw_writers(SeisOSegy *sgy) {
        sgy->write_i8 = write_i8;
        sgy->write_u8 = write_u8;
        switch (sgy->com->bin_hdr.endianness) {
        case 0x01020304:
                sgy->write_i16 = write_i16;
                sgy->write_u16 = write_u16;
                sgy->write_i24 = write_i24;
                sgy->write_u24 = write_u24;
                sgy->write_i32 = write_i32;
                sgy->write_u32 = write_u32;
                sgy->write_i64 = write_i64;
                sgy->write_u64 = write_u64;
                break;
        case 0:
        case 0x04030201:
                sgy->write_i16 = write_i16_sw;
                sgy->write_u16 = write_u16_sw;
                sgy->write_i24 = write_i24_sw;
                sgy->write_u24 = write_u24_sw;
                sgy->write_i32 = write_i32_sw;
                sgy->write_u32 = write_u32_sw;
                sgy->write_i64 = write_i64_sw;
                sgy->write_u64 = write_u64_sw;
                break;
        default:
                sgy->com->err.code = SEIS_SEGY_ERR_UNKNOWN_ENDIANNESS;
                sgy->com->err.message = "unsupported endianness";
                break;
        }
        if (FLT_RADIX == 2 && DBL_MANT_DIG == 53) {
                sgy->write_IEEE_float = write_IEEE_float_native;
                sgy->write_IEEE_double = write_IEEE_double_native;
        } else {
                sgy->write_IEEE_float = write_IEEE_float;
                sgy->write_IEEE_double = write_IEEE_double;
        }
        sgy->write_ibm_float = write_IBM_float;
        return sgy->com->err.code;
}

SeisSegyErrCode assign_sample_writer(SeisOSegy *sgy) {
        switch (sgy->com->bin_hdr.format_code) {
        case 1:
                sgy->write_sample = write_IBM_float;
                break;
        case 2:
                sgy->write_sample = dbl_to_i32;
                break;
        case 3:
                sgy->write_sample = dbl_to_i16;
                break;
        case 5:
                sgy->write_sample = sgy->write_IEEE_float;
                break;
        case 6:
                sgy->write_sample = sgy->write_IEEE_double;
                break;
        case 7:
                sgy->write_sample = dbl_to_i24;
                break;
        case 8:
                sgy->write_sample = dbl_to_i8;
                break;
        case 9:
                sgy->write_sample = dbl_to_i64;
                break;
        case 10:
                sgy->write_sample = dbl_to_u32;
                break;
        case 11:
                sgy->write_sample = dbl_to_u16;
                break;
        case 12:
                sgy->write_sample = dbl_to_u64;
                break;
        case 15:
                sgy->write_sample = dbl_to_u24;
                break;
        case 16:
                sgy->write_sample = dbl_to_u8;
                break;
        default:
                sgy->com->err.code = SEIS_SEGY_ERR_UNSUPPORTED_FORMAT;
                sgy->com->err.message = "unknown format code in binary header";
        }
        return sgy->com->err.code;
}

SeisSegyErrCode assign_bytes_per_sample(SeisOSegy *sgy) {
        switch (sgy->com->bin_hdr.format_code) {
        case 1:
        case 2:
        case 4:
        case 5:
        case 10:
                sgy->com->bytes_per_sample = 4;
                break;
        case 3:
        case 11:
                sgy->com->bytes_per_sample = 2;
                break;
        case 6:
        case 9:
        case 12:
                sgy->com->bytes_per_sample = 8;
                break;
        case 7:
        case 15:
                sgy->com->bytes_per_sample = 3;
                break;
        case 8:
        case 16:
                sgy->com->bytes_per_sample = 1;
                break;
        default:
                sgy->com->err.code = SEIS_SEGY_ERR_UNSUPPORTED_FORMAT;
                sgy->com->err.message = "unknown format code in binary header";
        }
        return sgy->com->err.code;
}

SeisSegyErrCode write_text_header(SeisOSegy *sgy) {
        SeisCommonSegy *com = sgy->com;
        size_t hdr_num = seis_common_segy_get_text_headers_num(com);
        char const *hdr;
        char *buf = NULL;
        if (!hdr_num) {
                buf = (char *)malloc(TEXT_HEADER_SIZE + 1);
                if (!buf) {
                        com->err.code = SEIS_SEGY_ERR_NO_MEM;
                        com->err.message = "no memory for text header";
                        goto error;
                }
                char const *def_hdr;
                switch (com->bin_hdr.SEGY_rev_major_ver) {
                case 0:
                default:
                        def_hdr = seis_segy_default_text_header_rev0;
                        break;
                case 1:
                        def_hdr = seis_segy_default_text_header_rev1;
                        break;
                case 2:
                        def_hdr = seis_segy_default_text_header_rev2;
                        break;
                }
                memcpy(buf, def_hdr, TEXT_HEADER_SIZE);
                buf[TEXT_HEADER_SIZE] = 0;
                ascii_to_ebcdic(buf);
                hdr = buf;
        } else {
                hdr = seis_common_segy_get_text_header(com, 0);
        }
        write_to_file(sgy, hdr, TEXT_HEADER_SIZE);
        if (buf)
                free(buf);
        return com->err.code;
error:
        if (buf)
                free(buf);
        return com->err.code;
}

SeisSegyErrCode write_bin_header(SeisOSegy *sgy) {
        SeisCommonSegy *com = sgy->com;
        char *const bin_buf = (char *)malloc(BIN_HEADER_SIZE);
        if (!bin_buf) {
                com->err.code = SEIS_SEGY_ERR_NO_MEM;
                com->err.message = "no memory for bin header";
        }
        memset(bin_buf, 0, BIN_HEADER_SIZE);
        char *ptr = bin_buf;
        sgy->write_i32(&ptr, com->bin_hdr.job_id);
        sgy->write_i32(&ptr, com->bin_hdr.line_num);
        sgy->write_i32(&ptr, com->bin_hdr.reel_num);
        sgy->write_i16(&ptr, com->bin_hdr.tr_per_ens);
        sgy->write_i16(&ptr, com->bin_hdr.aux_per_ens);
        sgy->write_i16(&ptr, com->bin_hdr.samp_int);
        sgy->write_i16(&ptr, com->bin_hdr.samp_int_orig);
        sgy->write_i16(&ptr, com->bin_hdr.samp_per_tr);
        sgy->write_i16(&ptr, com->bin_hdr.samp_per_tr_orig);
        sgy->write_i16(&ptr, com->bin_hdr.format_code);
        sgy->write_i16(&ptr, com->bin_hdr.ens_fold);
        sgy->write_i16(&ptr, com->bin_hdr.sort_code);
        sgy->write_i16(&ptr, com->bin_hdr.vert_sum_code);
        sgy->write_i16(&ptr, com->bin_hdr.sw_freq_at_start);
        sgy->write_i16(&ptr, com->bin_hdr.sw_freq_at_end);
        sgy->write_i16(&ptr, com->bin_hdr.sw_length);
        sgy->write_i16(&ptr, com->bin_hdr.sw_type_code);
        sgy->write_i16(&ptr, com->bin_hdr.sw_ch_tr_num);
        sgy->write_i16(&ptr, com->bin_hdr.taper_at_start);
        sgy->write_i16(&ptr, com->bin_hdr.taper_at_end);
        sgy->write_i16(&ptr, com->bin_hdr.taper_type);
        sgy->write_i16(&ptr, com->bin_hdr.corr_traces);
        sgy->write_i16(&ptr, com->bin_hdr.bin_gain_recov);
        sgy->write_i16(&ptr, com->bin_hdr.amp_recov_meth);
        sgy->write_i16(&ptr, com->bin_hdr.measure_system);
        sgy->write_i16(&ptr, com->bin_hdr.impulse_sig_pol);
        sgy->write_i16(&ptr, com->bin_hdr.vib_pol_code);
        sgy->write_i32(&ptr, com->bin_hdr.ext_tr_per_ens);
        sgy->write_i32(&ptr, com->bin_hdr.ext_aux_per_ens);
        sgy->write_i32(&ptr, com->bin_hdr.ext_samp_per_tr);
        sgy->write_IEEE_double(sgy, &ptr, com->bin_hdr.ext_samp_int);
        sgy->write_IEEE_double(sgy, &ptr, com->bin_hdr.ext_samp_int_orig);
        sgy->write_i32(&ptr, com->bin_hdr.ext_samp_per_tr_orig);
        sgy->write_i32(&ptr, com->bin_hdr.ext_ens_fold);
        sgy->write_i32(&ptr, com->bin_hdr.endianness);
        ptr += 200;
        sgy->write_u8(&ptr, com->bin_hdr.SEGY_rev_major_ver);
        sgy->write_u8(&ptr, com->bin_hdr.SEGY_rev_minor_ver);
        sgy->write_i16(&ptr, com->bin_hdr.fixed_tr_length);
        sgy->write_i16(&ptr, com->bin_hdr.ext_text_headers_num);
        sgy->write_i32(&ptr, com->bin_hdr.max_num_add_tr_headers);
        sgy->write_i16(&ptr, com->bin_hdr.time_basis_code);
        sgy->write_u64(&ptr, com->bin_hdr.num_of_tr_in_file);
        sgy->write_u64(&ptr, com->bin_hdr.byte_off_of_first_tr);
        sgy->write_i32(&ptr, com->bin_hdr.num_of_trailer_stanza);
        fseek(com->file, TEXT_HEADER_SIZE, SEEK_SET);
        write_to_file(sgy, bin_buf, BIN_HEADER_SIZE);
        free(bin_buf);
        return com->err.code;
}

SeisSegyErrCode write_ext_text_headers(SeisOSegy *sgy) {
        SeisCommonSegy *com = sgy->com;
        size_t num = seis_common_segy_get_text_headers_num(com);
        for (size_t i = 1; i < num; ++i) {
                char const *hdr = seis_common_segy_get_text_header(com, i);
                write_to_file(sgy, hdr, TEXT_HEADER_SIZE);
        }
        return com->err.code;
}

SeisSegyErrCode write_trailer_stanzas(SeisOSegy *sgy) {
        SeisCommonSegy *com = sgy->com;
        size_t num = seis_common_segy_get_stanzas_num(com);
        for (size_t i = 1; i < num; ++i) {
                char const *hdr = seis_common_segy_get_stanza(com, i);
                write_to_file(sgy, hdr, TEXT_HEADER_SIZE);
        }
        return com->err.code;
}

void fill_buf_with_fmt_arr(SeisOSegy *sgy, single_hdr_fmt_t *arr,
                           SeisTraceHeader const *hdr) {
        char *ptr;
        char const *tmp;
        SeisTraceHeaderValue v;
        long long const *i;
        double const *d;
        SeisCommonSegy *com = sgy->com;
        memset(com->hdr_buf, 0, TRACE_HEADER_SIZE);
        for
                M_EACH(item, *arr, M_OPL_single_hdr_fmt_t()) {
                        ptr = com->hdr_buf + (*item)->offset;
                        switch ((*item)->format) {
                        case i8:
                                v = seis_trace_header_get(
                                    hdr, string_get_cstr((*item)->name));
                                i = seis_trace_header_value_get_int(v);
                                sgy->write_i8(&ptr, i ? *i : 0);
                                break;
                        case u8:
                                v = seis_trace_header_get(
                                    hdr, string_get_cstr((*item)->name));
                                i = seis_trace_header_value_get_int(v);
                                sgy->write_u8(&ptr, i ? *i : 0);
                                break;
                        case i16:
                                v = seis_trace_header_get(
                                    hdr, string_get_cstr((*item)->name));
                                i = seis_trace_header_value_get_int(v);
                                sgy->write_i16(&ptr, i ? *i : 0);
                                break;
                        case u16:
                                v = seis_trace_header_get(
                                    hdr, string_get_cstr((*item)->name));
                                i = seis_trace_header_value_get_int(v);
                                sgy->write_u16(&ptr, i ? *i : 0);
                                break;
                        case i32:
                                v = seis_trace_header_get(
                                    hdr, string_get_cstr((*item)->name));
                                i = seis_trace_header_value_get_int(v);
                                sgy->write_i32(&ptr, i ? *i : 0);
                                break;
                        case u32:
                                v = seis_trace_header_get(
                                    hdr, string_get_cstr((*item)->name));
                                i = seis_trace_header_value_get_int(v);
                                sgy->write_u32(&ptr, i ? *i : 0);
                                break;
                        case i64:
                                v = seis_trace_header_get(
                                    hdr, string_get_cstr((*item)->name));
                                i = seis_trace_header_value_get_int(v);
                                sgy->write_i64(&ptr, i ? *i : 0);
                                break;
                        case u64:
                                v = seis_trace_header_get(
                                    hdr, string_get_cstr((*item)->name));
                                i = seis_trace_header_value_get_int(v);
                                sgy->write_u64(&ptr, i ? *i : 0);
                                break;
                        case f32:
                                v = seis_trace_header_get(
                                    hdr, string_get_cstr((*item)->name));
                                d = seis_trace_header_value_get_real(v);
                                sgy->write_IEEE_float(sgy, &ptr, d ? *d : 0);
                                break;
                        case f64:
                                v = seis_trace_header_get(
                                    hdr, string_get_cstr((*item)->name));
                                d = seis_trace_header_value_get_real(v);
                                sgy->write_IEEE_double(sgy, &ptr, d ? *d : 0);
                                break;
                        case b64:
                                tmp = string_get_cstr((*item)->name);
                                size_t size = strlen(tmp);
                                size = size > 8 ? 8 : size;
                                memcpy(ptr, tmp, size);
                                break;
                        }
                }
}

SeisSegyErrCode write_trace_header(SeisOSegy *sgy, SeisTraceHeader const *hdr) {
        SeisCommonSegy *com = sgy->com;
        SeisCommonSegyPrivate *priv = (SeisCommonSegyPrivate *)com;
        fill_buf_with_fmt_arr(sgy, mult_hdr_fmt_get(priv->trc_hdr_map, 0), hdr);
        TRY(write_to_file(sgy, com->hdr_buf, TRACE_HEADER_SIZE));
        if (com->bin_hdr.max_num_add_tr_headers) {
                SeisTraceHeaderValue v =
                    seis_trace_header_get(hdr, "ADD_TRC_HDR_NUM");
                long long const *add_hdr_num =
                    seis_trace_header_value_get_int(v);
                int to_write;
                if (!*add_hdr_num)
                        to_write = com->bin_hdr.max_num_add_tr_headers;
                else
                        to_write = *add_hdr_num;
                for (int i = 1; i < 1 + to_write; ++i) {
                        fill_buf_with_fmt_arr(
                            sgy, mult_hdr_fmt_get(priv->trc_hdr_map, i), hdr);
                        TRY(write_to_file(sgy, com->hdr_buf,
                                          TRACE_HEADER_SIZE));
                }
        }
error:
        return com->err.code;
}

SeisSegyErrCode write_trace_samples_fix(SeisOSegy *sgy, SeisTrace const *t) {
        SeisCommonSegy *com = sgy->com;
        char *buf = com->samp_buf;
        double const *samples = seis_trace_get_samples_const(t);
        long long const samp_num = seis_trace_get_samples_num(t);
        for (long long i = 0; i < samp_num; ++i)
                sgy->write_sample(sgy, &buf, samples[i]);
        write_to_file(sgy, com->samp_buf, com->bytes_per_sample * samp_num);
        return com->err.code;
}

SeisSegyErrCode write_trace_samples_var(SeisOSegy *sgy, SeisTrace const *t) {
        SeisCommonSegy *com = sgy->com;
        long long const samp_num = seis_trace_get_samples_num(t);
        if (samp_num > com->samp_per_tr) {
                long long bytes_num = samp_num * com->bytes_per_sample;
                com->samp_buf = realloc(com->samp_buf, bytes_num);
                if (!com->samp_buf) {
                        com->err.code = SEIS_SEGY_ERR_NO_MEM;
                        com->err.message =
                            "can't get memory to write variable trace";
                        goto error;
                }
                com->samp_per_tr = samp_num;
        }
        return write_trace_samples_fix(sgy, t);
error:
        return com->err.code;
}

void write_i8(char **buf, int8_t val) {
        memcpy(*buf, &val, sizeof(int8_t));
        ++*buf;
}

void write_u8(char **buf, uint8_t val) {
        memcpy(*buf, &val, sizeof(uint8_t));
        ++*buf;
}

void write_i16(char **buf, int16_t val) {
        memcpy(*buf, &val, sizeof(int16_t));
        *buf += sizeof(int16_t);
}

void write_u16(char **buf, uint16_t val) {
        memcpy(*buf, &val, sizeof(uint16_t));
        *buf += sizeof(uint16_t);
}

void write_i24(char **buf, int32_t val) {
        uint16_t tmp16 = (uint32_t)val & 0xff;
        memcpy(*buf, &tmp16, sizeof(int16_t));
        *buf += sizeof(int16_t);
        uint8_t tmp8 = ((uint32_t)val & 0x0f00) >> 16;
        memcpy(*buf, &tmp8, sizeof(int8_t));
        ++*buf;
}

void write_u24(char **buf, uint32_t val) {
        uint16_t tmp16 = val & 0xff;
        memcpy(*buf, &tmp16, sizeof(uint16_t));
        *buf += sizeof(uint16_t);
        uint8_t tmp8 = (val & 0x0f00) >> 16;
        memcpy(*buf, &tmp8, sizeof(uint8_t));
        ++*buf;
}

void write_i32(char **buf, int32_t val) {
        memcpy(*buf, &val, sizeof(int32_t));
        *buf += sizeof(int32_t);
}

void write_u32(char **buf, uint32_t val) {
        memcpy(*buf, &val, sizeof(uint32_t));
        *buf += sizeof(uint32_t);
}

void write_i64(char **buf, int64_t val) {
        memcpy(*buf, &val, sizeof(int64_t));
        *buf += sizeof(int64_t);
}

void write_u64(char **buf, uint64_t val) {
        memcpy(*buf, &val, sizeof(uint64_t));
        *buf += sizeof(uint64_t);
}

void write_i16_sw(char **buf, int16_t val) {
        uint16_t tmp = val;
        val = (tmp & 0xff) << 8 | (tmp & 0xff00) >> 8;
        memcpy(*buf, &val, sizeof(int16_t));
        *buf += sizeof(int16_t);
}

void write_u16_sw(char **buf, uint16_t val) {
        uint16_t tmp = val;
        val = (tmp & 0xff) << 8 | (tmp & 0xff00) >> 8;
        memcpy(*buf, &val, sizeof(uint16_t));
        *buf += sizeof(uint16_t);
}

void write_i24_sw(char **buf, int32_t val) {
        uint8_t tmp8 = ((uint32_t)val & 0x0f00) >> 16;
        memcpy(*buf, &tmp8, sizeof(int8_t));
        ++*buf;
        uint16_t tmp16 = (uint32_t)val & 0xff;
        tmp16 = (tmp16 & 0xf) << 8 | (tmp16 & 0xf0) >> 8;
        memcpy(*buf, &tmp16, sizeof(int16_t));
        *buf += sizeof(int16_t);
}

void write_u24_sw(char **buf, uint32_t val) {
        uint8_t tmp8 = (val & 0x0f00) >> 16;
        memcpy(*buf, &tmp8, sizeof(int8_t));
        ++*buf;
        uint16_t tmp16 = val & 0xff;
        tmp16 = (tmp16 & 0xf) << 8 | (tmp16 & 0xf0) >> 8;
        memcpy(*buf, &tmp16, sizeof(uint16_t));
        *buf += sizeof(uint16_t);
}

void write_i32_sw(char **buf, int32_t val) {
        uint32_t tmp = val;
        val = (tmp & 0xff) << 24 | (tmp & 0xff000000) >> 24 |
              (tmp & 0xff00) << 8 | (tmp & 0xff0000) >> 8;
        memcpy(*buf, &val, sizeof(int32_t));
        *buf += sizeof(int32_t);
}

void write_u32_sw(char **buf, uint32_t val) {
        uint32_t tmp = val;
        val = (tmp & 0xff) << 24 | (tmp & 0xff000000) >> 24 |
              (tmp & 0xff00) << 8 | (tmp & 0xff0000) >> 8;
        memcpy(*buf, &val, sizeof(uint32_t));
        *buf += sizeof(uint32_t);
}

void write_i64_sw(char **buf, int64_t val) {
        uint64_t tmp = val;
        val = (tmp & 0xff) << 56 | (tmp & 0xff00000000000000) >> 56 |
              (tmp & 0xff00) << 40 | (tmp & 0xff000000000000) >> 40 |
              (tmp & 0xff0000) << 24 | (tmp & 0xff0000000000) >> 24 |
              (tmp & 0xff000000) << 8 | (tmp & 0xff00000000) >> 8;
        memcpy(*buf, &val, sizeof(int64_t));
        *buf += sizeof(int64_t);
}

void write_u64_sw(char **buf, uint64_t val) {
        uint64_t tmp = val;
        val = (tmp & 0xff) << 56 | (tmp & 0xff00000000000000) >> 56 |
              (tmp & 0xff00) << 40 | (tmp & 0xff000000000000) >> 40 |
              (tmp & 0xff0000) << 24 | (tmp & 0xff0000000000) >> 24 |
              (tmp & 0xff000000) << 8 | (tmp & 0xff00000000) >> 8;
        memcpy(*buf, &val, sizeof(uint64_t));
        *buf += sizeof(uint64_t);
}

void write_IBM_float(SeisOSegy *sgy, char **buf, double val) {
        uint32_t sign = val < 0 ? 1 : 0;
        double abs_val = fabs(val);
        uint32_t exp = (uint32_t)(log(abs_val) / log(2) / 4 + 1 + 64) & 0x7f;
        uint32_t fraction = abs_val / pow(16, (int)exp - 64) * pow(2, 24);
        uint32_t result = sign << 31 | exp << 24 | (fraction & 0x00ffffff);
        sgy->write_u32(buf, result);
}

void write_IEEE_float(SeisOSegy *sgy, char **buf, double val) {
        uint32_t sign = val < 0 ? 1 : 0;
        double abs_val = fabs(val);
        uint32_t exp = (uint32_t)(log(abs_val) / log(2) + 1 + 127) & 0xff;
        uint32_t fraction =
            abs_val / pow(2, (int32_t)exp - 127) * pow(2, 23) - 1;
        uint32_t result = sign << 31 | exp << 23 | (fraction & 0x007fffff);
        sgy->write_u32(buf, result);
}

void write_IEEE_double(SeisOSegy *sgy, char **buf, double val) {
        uint64_t sign = val < 0 ? 1 : 0;
        double abs_val = fabs(val);
        uint64_t exp = (uint64_t)(log(abs_val) / log(2) + 1 + 1023) & 0x7ff;
        uint64_t fraction =
            abs_val / pow(2, (int32_t)exp - 1023) * pow(2, 52 - 1);
        uint64_t result = sign << 63 | exp << 52 | (fraction & 0xfffffffffffff);
        sgy->write_u64(buf, result);
}

void write_IEEE_float_native(SeisOSegy *sgy, char **buf, double val) {
        float flt = val;
        uint32_t tmp;
        memcpy(&tmp, &flt, sizeof(float));
        sgy->write_u32(buf, tmp);
}

void write_IEEE_double_native(SeisOSegy *sgy, char **buf, double val) {
        uint64_t tmp;
        memcpy(&tmp, &val, sizeof(double));
        sgy->write_u64(buf, tmp);
}

void dbl_to_i8(SeisOSegy *sgy, char **buf, double val) {
        sgy->write_i8(buf, (int8_t)val);
}

void dbl_to_u8(SeisOSegy *sgy, char **buf, double val) {
        sgy->write_u8(buf, (uint8_t)val);
}

void dbl_to_i16(SeisOSegy *sgy, char **buf, double val) {
        sgy->write_i16(buf, (int16_t)val);
}

void dbl_to_u16(SeisOSegy *sgy, char **buf, double val) {
        sgy->write_u16(buf, (uint16_t)val);
}

void dbl_to_i24(SeisOSegy *sgy, char **buf, double val) {
        sgy->write_i24(buf, (int32_t)val);
}

void dbl_to_u24(SeisOSegy *sgy, char **buf, double val) {
        sgy->write_u24(buf, (uint32_t)val);
}

void dbl_to_i32(SeisOSegy *sgy, char **buf, double val) {
        sgy->write_i32(buf, (int32_t)val);
}

void dbl_to_u32(SeisOSegy *sgy, char **buf, double val) {
        sgy->write_u32(buf, (uint32_t)val);
}

void dbl_to_i64(SeisOSegy *sgy, char **buf, double val) {
        sgy->write_i64(buf, (int64_t)val);
}

void dbl_to_u64(SeisOSegy *sgy, char **buf, double val) {
        sgy->write_u64(buf, (uint64_t)val);
}
