#include "SeisISegy.h"
#include "SeisCommonSegy.h"
#include "SeisCommonSegyPrivate.h"
#include "SeisISU.h"
#include "TRY.h"
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

struct SeisISegy {
        SeisCommonSegy *com;
        long curr_pos, first_trace_pos, end_of_data;
        int8_t (*read_i8)(char const **buf);
        uint8_t (*read_u8)(char const **buf);
        int16_t (*read_i16)(char const **buf);
        uint16_t (*read_u16)(char const **buf);
        int32_t (*read_i24)(char const **buf);
        uint32_t (*read_u24)(char const **buf);
        int32_t (*read_i32)(char const **buf);
        uint32_t (*read_u32)(char const **buf);
        int64_t (*read_i64)(char const **buf);
        uint64_t (*read_u64)(char const **buf);
        double (*dbl_from_IEEE_float)(SeisISegy *sgy, char const **buf);
        double (*dbl_from_IEEE_double)(SeisISegy *sgy, char const **buf);
        double (*read_sample)(SeisISegy *sgy, char const **buf);
        SeisSegyErrCode (*read_trc_smpls)(SeisISegy *sgy, SeisTraceHeader *hdr,
                                          SeisTrace **trc);
        SeisSegyErrCode (*skip_trc_smpls)(SeisISegy *sgy, SeisTraceHeader *hdr);
        int rc;
};

struct SeisISU {
        SeisISegy *sgy;
        int rc;
};

static SeisSegyErrCode fill_from_file(SeisISegy *sgy, char *buf, size_t num);
static void file_skip_bytes(SeisISegy *sgy, size_t num);
static SeisSegyErrCode
read_text_header(SeisISegy *sgy,
                 void (*add_func)(SeisCommonSegy *, char const *), int num);
static SeisSegyErrCode read_bin_header(SeisISegy *sgy);
static SeisSegyErrCode assign_raw_readers(SeisISegy *sgy);
static SeisSegyErrCode assign_sample_reader(SeisISegy *sgy);
static SeisSegyErrCode assign_bytes_per_sample(SeisISegy *sgy);
static SeisSegyErrCode read_ext_text_headers(SeisISegy *sgy);
static SeisSegyErrCode read_trailer_stanzas(SeisISegy *sgy);
static SeisSegyErrCode read_trc_smpls_fix(SeisISegy *sgy, SeisTraceHeader *hdr,
                                          SeisTrace **trc);
static SeisSegyErrCode read_trc_smpls_var(SeisISegy *sgy, SeisTraceHeader *hdr,
                                          SeisTrace **trc);
static SeisSegyErrCode read_trc_hdr(SeisISegy *sgy, SeisTraceHeader *hdr);
static SeisSegyErrCode skip_trc_smpls_fix(SeisISegy *sgy, SeisTraceHeader *hdr);
static SeisSegyErrCode skip_trc_smpls_var(SeisISegy *sgy, SeisTraceHeader *hdr);
static void fill_hdr_from_fmt_arr(SeisISegy *sgy, single_hdr_fmt_t *arr,
                                  SeisTraceHeader *hdr);

static int8_t read_i8(char const **buf);
static uint8_t read_u8(char const **buf);
static int16_t read_i16(char const **buf);
static uint16_t read_u16(char const **buf);
static int32_t read_i24(char const **buf);
static uint32_t read_u24(char const **buf);
static int32_t read_i32(char const **buf);
static uint32_t read_u32(char const **buf);
static int64_t read_i64(char const **buf);
static uint64_t read_u64(char const **buf);
static int16_t read_i16_sw(char const **buf);
static uint16_t read_u16_sw(char const **buf);
static int32_t read_i24_sw(char const **buf);
static uint32_t read_u24_sw(char const **buf);
static int32_t read_i32_sw(char const **buf);
static uint32_t read_u32_sw(char const **buf);
static int64_t read_i64_sw(char const **buf);
static uint64_t read_u64_sw(char const **buf);
static double dbl_from_IBM_float(SeisISegy *sgy, char const **buf);
static double dbl_from_IEEE_float(SeisISegy *sgy, char const **buf);
static double dbl_from_IEEE_double(SeisISegy *sgy, char const **buf);
static double dbl_from_IEEE_float_native(SeisISegy *sgy, char const **buf);
static double dbl_from_IEEE_double_native(SeisISegy *sgy, char const **buf);
static double dbl_from_i8(SeisISegy *sgy, char const **buf);
static double dbl_from_u8(SeisISegy *sgy, char const **buf);
static double dbl_from_i16(SeisISegy *sgy, char const **buf);
static double dbl_from_u16(SeisISegy *sgy, char const **buf);
static double dbl_from_i24(SeisISegy *sgy, char const **buf);
static double dbl_from_u24(SeisISegy *sgy, char const **buf);
static double dbl_from_i32(SeisISegy *sgy, char const **buf);
static double dbl_from_u32(SeisISegy *sgy, char const **buf);
static double dbl_from_i64(SeisISegy *sgy, char const **buf);
static double dbl_from_u64(SeisISegy *sgy, char const **buf);

SeisISegy *seis_isegy_new(void) {
        SeisISegy *sgy = (SeisISegy *)malloc(sizeof(struct SeisISegy));
        if (!sgy)
                goto error;
        sgy->com = seis_common_segy_new();
        sgy->rc = 1;
        return sgy;
error:
        return NULL;
}

SeisISegy *seis_isegy_ref(SeisISegy *sgy) {
        ++sgy->rc;
        return sgy;
}

void seis_isegy_unref(SeisISegy **sgy) {
        if (*sgy)
                if (--(*sgy)->rc == 0) {
                        seis_common_segy_unref(&(*sgy)->com);
                        free(*sgy);
                        *sgy = NULL;
                }
}

bool seis_isegy_end_of_data(SeisISegy const *sgy) {
        return sgy->end_of_data == sgy->curr_pos;
}

SeisSegyErr const *seis_isegy_get_error(SeisISegy const *sgy) {
        return &sgy->com->err;
}

SeisSegyErrCode seis_isegy_open(SeisISegy *sgy, char const *file_name) {
        SeisCommonSegy *com = sgy->com;
        assert(!com->file); /* open func must be called only once */
        com->file = fopen(file_name, "r");
        if (!com->file) {
                com->err.code = SEIS_SEGY_ERR_FILE_OPEN;
                com->err.message = "file open error";
                goto error;
        }
        TRY(read_text_header(sgy, seis_common_segy_add_text_header, 1));
        TRY(read_bin_header(sgy));
        TRY(assign_sample_reader(sgy));
        TRY(assign_bytes_per_sample(sgy));
        TRY(read_ext_text_headers(sgy));
        if (com->bin_hdr.byte_off_of_first_tr) {
                sgy->first_trace_pos = com->bin_hdr.byte_off_of_first_tr;
                fseek(com->file, sgy->first_trace_pos, SEEK_SET);
        } else {
                sgy->first_trace_pos = ftell(com->file);
        }
        com->samp_per_tr = com->bin_hdr.ext_samp_per_tr
                               ? com->bin_hdr.ext_samp_per_tr
                               : com->bin_hdr.samp_per_tr;
        TRY(read_trailer_stanzas(sgy));
        fseek(com->file, sgy->first_trace_pos, SEEK_SET);
        sgy->curr_pos = sgy->first_trace_pos;
        com->samp_buf =
            (char *)malloc(com->samp_per_tr * com->bytes_per_sample);
        if (com->bin_hdr.fixed_tr_length || !com->bin_hdr.SEGY_rev_major_ver) {
                sgy->read_trc_smpls = read_trc_smpls_fix;
                sgy->skip_trc_smpls = skip_trc_smpls_fix;
        } else {
                sgy->read_trc_smpls = read_trc_smpls_var;
                sgy->skip_trc_smpls = skip_trc_smpls_var;
        }
error:
        return com->err.code;
}

SeisTrace *seis_isegy_read_trace(SeisISegy *sgy) {
        SeisTraceHeader *hdr = seis_trace_header_new();
        if (!hdr) {
                sgy->com->err.code = SEIS_SEGY_ERR_NO_MEM;
                sgy->com->err.message = "can't get memory at trace reading";
                goto error;
        }
        TRY(read_trc_hdr(sgy, hdr));
        SeisTrace *trc;
        TRY(sgy->read_trc_smpls(sgy, hdr, &trc));
        return trc;
error:
        seis_trace_header_unref(&hdr);
        return NULL;
}

SeisTraceHeader *seis_isegy_read_trace_header(SeisISegy *sgy) {
        SeisTraceHeader *hdr = seis_trace_header_new();
        if (!hdr) {
                sgy->com->err.code = SEIS_SEGY_ERR_NO_MEM;
                sgy->com->err.message =
                    "can't get memory at trace header reading";
                goto error;
        }
        TRY(read_trc_hdr(sgy, hdr));
        sgy->skip_trc_smpls(sgy, hdr);
        return hdr;
error:
        seis_trace_header_unref(&hdr);
        return NULL;
}

SeisSegyBinHdr const *seis_isegy_get_binary_header(SeisISegy const *sgy) {
        return &sgy->com->bin_hdr;
}

SeisSegyErrCode seis_isegy_remap_trace_header(SeisISegy *sgy,
                                              char const *hdr_name, int hdr_num,
                                              int offset, enum FORMAT fmt) {
        return seis_common_remap_trace_header(sgy->com, hdr_name, hdr_num,
                                              offset, fmt);
}

size_t seis_isegy_get_text_headers_num(SeisISegy const *sgy) {
        return seis_common_segy_get_text_headers_num(sgy->com);
}

char const *seis_isegy_get_text_header(SeisISegy const *sgy, size_t idx) {
        return seis_common_segy_get_text_header(sgy->com, idx);
}

void seis_isegy_rewind(SeisISegy *sgy) {
        fseek(sgy->com->file, sgy->first_trace_pos, SEEK_SET);
        sgy->curr_pos = sgy->first_trace_pos;
}

size_t seis_isegy_get_offset(SeisISegy *sgy) { return ftell(sgy->com->file); }

void seis_isegy_set_offset(SeisISegy *sgy, size_t offset) {
        fseek(sgy->com->file, offset, SEEK_SET);
}

SeisISU *seis_isu_new(void) {
        SeisISU *su = (SeisISU *)malloc(sizeof(struct SeisISU));
        if (!su)
                goto error;
        su->sgy = seis_isegy_new();
        su->rc = 1;
        return su;
error:
        return NULL;
}

SeisISU *seis_isu_ref(SeisISU *su) {
        ++su->rc;
        return su;
}

void seis_isu_unref(SeisISU **su) {
        if (*su)
                if (--(*su)->rc == 0) {
                        seis_isegy_unref(&(*su)->sgy);
                        free(*su);
                        *su = NULL;
                }
}

SeisSegyErrCode seis_isu_open(SeisISU *su, char const *file_name) {
        SeisISegy *sgy = su->sgy;
        SeisCommonSegy *com = sgy->com;
        assert(!com->file); /* open func must be called only once */
        com->file = fopen(file_name, "r");
        if (!com->file) {
                com->err.code = SEIS_SEGY_ERR_FILE_OPEN;
                com->err.message = "file open error";
                goto error;
        }
#ifndef SU_BIG_ENDIAN
        com->bin_hdr.endianness = 0x01020304;
#endif
        TRY(assign_raw_readers(sgy));
        com->bin_hdr.format_code = 5;
        sgy->read_sample = dbl_from_IEEE_float_native;
        com->bytes_per_sample = 4;
        sgy->first_trace_pos = ftell(com->file);
        seis_isegy_remap_trace_header(sgy, "SAMP_NUM", 1, 115, u16);
        seis_isegy_remap_trace_header(sgy, "SAMP_INT", 1, 117, u16);
        SeisTraceHeader *hdr = seis_trace_header_new();
        if (!hdr) {
                com->err.code = SEIS_SEGY_ERR_NO_MEM;
                com->err.message = "can't get memory at trace reading";
                goto error;
        }
        TRY(read_trc_hdr(sgy, hdr));
        long long const *samp_num = seis_trace_header_get_int(hdr, "SAMP_NUM");
        com->samp_per_tr = *samp_num;
        if (!samp_num) {
                com->err.code = SEIS_SEGY_ERR_BROKEN_FILE;
                com->err.message =
                    "variable trace length and zero samples number";
                goto error;
        }
        fseek(com->file, 0, SEEK_END);
        sgy->end_of_data = ftell(com->file);
        fseek(com->file, sgy->first_trace_pos, SEEK_SET);
        sgy->curr_pos = sgy->first_trace_pos;
        com->samp_buf =
            (char *)malloc(com->samp_per_tr * com->bytes_per_sample);
        sgy->read_trc_smpls = read_trc_smpls_fix;
        sgy->skip_trc_smpls = skip_trc_smpls_fix;
        seis_trace_header_unref(&hdr);
error:
        seis_trace_header_unref(&hdr);
        return com->err.code;
}

bool seis_isu_end_of_data(SeisISU const *su) {
        return su->sgy->end_of_data == su->sgy->curr_pos;
}

SeisSegyErr const *seis_isu_get_error(SeisISU const *su) {
        return &su->sgy->com->err;
}

void seis_isu_rewind(SeisISU *su) {
        fseek(su->sgy->com->file, su->sgy->first_trace_pos, SEEK_SET);
        su->sgy->curr_pos = su->sgy->first_trace_pos;
}

SeisTrace *seis_isu_read_trace(SeisISU *su) {
        SeisTraceHeader *hdr = seis_trace_header_new();
        if (!hdr) {
                su->sgy->com->err.code = SEIS_SEGY_ERR_NO_MEM;
                su->sgy->com->err.message = "can't get memory at trace reading";
                goto error;
        }
        TRY(read_trc_hdr(su->sgy, hdr));
        SeisTrace *trc;
        TRY(su->sgy->read_trc_smpls(su->sgy, hdr, &trc));
        return trc;
error:
        seis_trace_header_unref(&hdr);
        return NULL;
}

SeisTraceHeader *seis_isu_read_trace_header(SeisISU *su) {
        SeisTraceHeader *hdr = seis_trace_header_new();
        if (!hdr) {
                su->sgy->com->err.code = SEIS_SEGY_ERR_NO_MEM;
                su->sgy->com->err.message = "can't get memory at trace reading";
                goto error;
        }
        TRY(read_trc_hdr(su->sgy, hdr));
        su->sgy->skip_trc_smpls(su->sgy, hdr);
        return hdr;
error:
        seis_trace_header_unref(&hdr);
        return NULL;
}

SeisSegyErrCode seis_isu_remap_trace_header(SeisISU *su, char const *hdr_name,
                                            int offset, enum FORMAT fmt) {
        return seis_isegy_remap_trace_header(su->sgy, hdr_name, 1, offset, fmt);
}

SeisSegyErrCode fill_from_file(SeisISegy *sgy, char *buf, size_t num) {
        SeisCommonSegy *com = sgy->com;
        size_t read = fread(buf, 1, num, com->file);
        if (read != num) {
                com->err.code = SEIS_SEGY_ERR_FILE_READ;
                com->err.message = "read less bytes than should";
        }
        sgy->curr_pos = ftell(com->file);
        return com->err.code;
}

void file_skip_bytes(SeisISegy *sgy, size_t num) {
        fseek(sgy->com->file, num, SEEK_CUR);
        sgy->curr_pos = ftell(sgy->com->file);
}

SeisSegyErrCode
read_text_header(SeisISegy *sgy,
                 void (*add_func)(SeisCommonSegy *, char const *), int num) {
        SeisCommonSegy *com = sgy->com;
        char *text_buf = (char *)malloc(TEXT_HEADER_SIZE + 1);
        text_buf[TEXT_HEADER_SIZE] = '\0';
        if (!text_buf) {
                com->err.code = SEIS_SEGY_ERR_NO_MEM;
                com->err.message = "no memory for text header buf";
                goto error;
        }
        for (int i = 0; i < num; ++i) {
                TRY(fill_from_file(sgy, text_buf, TEXT_HEADER_SIZE));
                add_func(sgy->com, text_buf);
        }
        free(text_buf);
        return com->err.code;
error:
        if (text_buf)
                free(text_buf);
        return com->err.code;
}

SeisSegyErrCode read_bin_header(SeisISegy *sgy) {
        SeisCommonSegy *com = sgy->com;
        char *const bin_buf = (char *)malloc(BIN_HEADER_SIZE);
        if (!bin_buf) {
                com->err.code = SEIS_SEGY_ERR_NO_MEM;
                com->err.message = "no memory for bin header";
        }
        TRY(fill_from_file(sgy, bin_buf, BIN_HEADER_SIZE));
        memcpy(&com->bin_hdr.endianness, bin_buf + 96, sizeof(int32_t));
        TRY(assign_raw_readers(sgy));
        char const *ptr = bin_buf;
        com->bin_hdr.job_id = sgy->read_i32(&ptr);
        com->bin_hdr.line_num = sgy->read_i32(&ptr);
        com->bin_hdr.reel_num = sgy->read_i32(&ptr);
        com->bin_hdr.tr_per_ens = sgy->read_i16(&ptr);
        com->bin_hdr.aux_per_ens = sgy->read_i16(&ptr);
        com->bin_hdr.samp_int = sgy->read_i16(&ptr);
        com->bin_hdr.samp_int_orig = sgy->read_i16(&ptr);
        com->bin_hdr.samp_per_tr = sgy->read_i16(&ptr);
        com->bin_hdr.samp_per_tr_orig = sgy->read_i16(&ptr);
        com->bin_hdr.format_code = sgy->read_i16(&ptr);
        com->bin_hdr.ens_fold = sgy->read_i16(&ptr);
        com->bin_hdr.sort_code = sgy->read_i16(&ptr);
        com->bin_hdr.vert_sum_code = sgy->read_i16(&ptr);
        com->bin_hdr.sw_freq_at_start = sgy->read_i16(&ptr);
        com->bin_hdr.sw_freq_at_end = sgy->read_i16(&ptr);
        com->bin_hdr.sw_length = sgy->read_i16(&ptr);
        com->bin_hdr.sw_type_code = sgy->read_i16(&ptr);
        com->bin_hdr.sw_ch_tr_num = sgy->read_i16(&ptr);
        com->bin_hdr.taper_at_start = sgy->read_i16(&ptr);
        com->bin_hdr.taper_at_end = sgy->read_i16(&ptr);
        com->bin_hdr.taper_type = sgy->read_i16(&ptr);
        com->bin_hdr.corr_traces = sgy->read_i16(&ptr);
        com->bin_hdr.bin_gain_recov = sgy->read_i16(&ptr);
        com->bin_hdr.amp_recov_meth = sgy->read_i16(&ptr);
        com->bin_hdr.measure_system = sgy->read_i16(&ptr);
        com->bin_hdr.impulse_sig_pol = sgy->read_i16(&ptr);
        com->bin_hdr.vib_pol_code = sgy->read_i16(&ptr);
        com->bin_hdr.ext_tr_per_ens = sgy->read_i32(&ptr);
        com->bin_hdr.ext_aux_per_ens = sgy->read_i32(&ptr);
        com->bin_hdr.ext_samp_per_tr = sgy->read_i32(&ptr);
        com->bin_hdr.ext_samp_int = sgy->dbl_from_IEEE_double(sgy, &ptr);
        com->bin_hdr.ext_samp_int_orig = sgy->dbl_from_IEEE_double(sgy, &ptr);
        com->bin_hdr.ext_samp_per_tr_orig = sgy->read_i32(&ptr);
        com->bin_hdr.ext_ens_fold = sgy->read_i32(&ptr);
        ptr += 204; // skip unassigned fields and endianness
        com->bin_hdr.SEGY_rev_major_ver = sgy->read_u8(&ptr);
        com->bin_hdr.SEGY_rev_minor_ver = sgy->read_u8(&ptr);
        com->bin_hdr.fixed_tr_length = sgy->read_i16(&ptr);
        com->bin_hdr.ext_text_headers_num = sgy->read_i16(&ptr);
        com->bin_hdr.max_num_add_tr_headers = sgy->read_i32(&ptr);
        com->bin_hdr.time_basis_code = sgy->read_i16(&ptr);
        com->bin_hdr.num_of_tr_in_file = sgy->read_i64(&ptr);
        com->bin_hdr.byte_off_of_first_tr = sgy->read_u64(&ptr);
        com->bin_hdr.num_of_trailer_stanza = sgy->read_i32(&ptr);
error:
        free(bin_buf);
        return com->err.code;
}

SeisSegyErrCode assign_raw_readers(SeisISegy *sgy) {
        SeisCommonSegy *com = sgy->com;
        sgy->read_i8 = read_i8;
        sgy->read_u8 = read_u8;
        switch (com->bin_hdr.endianness) {
        case 0x01020304:
                sgy->read_i16 = read_i16;
                sgy->read_u16 = read_u16;
                sgy->read_i24 = read_i24;
                sgy->read_u24 = read_u24;
                sgy->read_i32 = read_i32;
                sgy->read_u32 = read_u32;
                sgy->read_i64 = read_i64;
                sgy->read_u64 = read_u64;
                break;
        case 0:
        case 0x04030201:
                sgy->read_i16 = read_i16_sw;
                sgy->read_u16 = read_u16_sw;
                sgy->read_i24 = read_i24_sw;
                sgy->read_u24 = read_u24_sw;
                sgy->read_i32 = read_i32_sw;
                sgy->read_u32 = read_u32_sw;
                sgy->read_i64 = read_i64_sw;
                sgy->read_u64 = read_u64_sw;
                break;
        default:
                com->err.code = SEIS_SEGY_ERR_UNKNOWN_ENDIANNESS;
                com->err.message = "unsupported endianness";
        }
        if (FLT_RADIX == 2 && DBL_MANT_DIG == 53) {
                sgy->dbl_from_IEEE_float = dbl_from_IEEE_float_native;
                sgy->dbl_from_IEEE_double = dbl_from_IEEE_double_native;
        } else {
                sgy->dbl_from_IEEE_float = dbl_from_IEEE_float;
                sgy->dbl_from_IEEE_double = dbl_from_IEEE_double;
        }
        return com->err.code;
}

SeisSegyErrCode assign_sample_reader(SeisISegy *sgy) {
        SeisCommonSegy *com = sgy->com;
        switch (com->bin_hdr.format_code) {
        case 1:
                sgy->read_sample = dbl_from_IBM_float;
                break;
        case 2:
                sgy->read_sample = dbl_from_i32;
                break;
        case 3:
                sgy->read_sample = dbl_from_i16;
                break;
        case 5:
                sgy->read_sample = sgy->dbl_from_IEEE_float;
                break;
        case 6:
                sgy->read_sample = sgy->dbl_from_IEEE_double;
                break;
        case 7:
                sgy->read_sample = dbl_from_i24;
                break;
        case 8:
                sgy->read_sample = dbl_from_i8;
                break;
        case 9:
                sgy->read_sample = dbl_from_i64;
                break;
        case 10:
                sgy->read_sample = dbl_from_u32;
                break;
        case 11:
                sgy->read_sample = dbl_from_u16;
                break;
        case 12:
                sgy->read_sample = dbl_from_u64;
                break;
        case 15:
                sgy->read_sample = dbl_from_u24;
                break;
        case 16:
                sgy->read_sample = dbl_from_u8;
                break;
        default:
                com->err.code = SEIS_SEGY_ERR_UNSUPPORTED_FORMAT;
                com->err.message = "unsupported format code";
        }
        return com->err.code;
}

SeisSegyErrCode assign_bytes_per_sample(SeisISegy *sgy) {
        SeisCommonSegy *com = sgy->com;
        switch (com->bin_hdr.format_code) {
        case 1:
        case 2:
        case 4:
        case 5:
        case 10:
                com->bytes_per_sample = 4;
                break;
        case 3:
        case 11:
                com->bytes_per_sample = 2;
                break;
        case 6:
        case 9:
        case 12:
                com->bytes_per_sample = 8;
                break;
        case 7:
        case 15:
                com->bytes_per_sample = 3;
                break;
        case 8:
        case 16:
                com->bytes_per_sample = 1;
                break;
        default:
                sgy->com->err.code = SEIS_SEGY_ERR_UNSUPPORTED_FORMAT;
                sgy->com->err.message = "unknown format code in binary header";
        }
        return sgy->com->err.code;
}

SeisSegyErrCode read_ext_text_headers(SeisISegy *sgy) {
        SeisCommonSegy *com = sgy->com;
        int num = com->bin_hdr.ext_text_headers_num;
        if (!num)
                goto error;
        if (num == -1) {
                char *end_stanza = "((SEG: EndText))";
                while (1) {
                        TRY(read_text_header(
                            sgy, seis_common_segy_add_text_header, 1));
                        size_t hdrs_read =
                            seis_common_segy_get_text_headers_num(com);
                        char const *hdr = seis_common_segy_get_text_header(
                            com, hdrs_read - 1);
                        if (!strstr(hdr, end_stanza))
                                goto error;
                }
        } else {
                TRY(read_text_header(sgy, seis_common_segy_add_text_header,
                                     num));
        }
        return com->err.code;
error:
        return com->err.code;
}

SeisSegyErrCode read_trailer_stanzas(SeisISegy *sgy) {
        SeisCommonSegy *com = sgy->com;
        int32_t stanz_num = com->bin_hdr.num_of_trailer_stanza;
        /* skip if no trailer stanzas */
        if (stanz_num) {
                char *text_buf = (char *)malloc(TEXT_HEADER_SIZE);
                if (!text_buf) {
                        com->err.code = SEIS_SEGY_ERR_NO_MEM;
                        com->err.message = "no mem for trailer stanza";
                        goto error;
                }
                /* if number is unknown */
                if (stanz_num == -1) {
                        /* we need to know number of traces to skip them */
                        if (!com->bin_hdr.num_of_tr_in_file) {
                                com->err.code = SEIS_SEGY_ERR_BROKEN_FILE;
                                com->err.message =
                                    "unable to determine end of trace data";
                                goto error;
                        }
                        /* if traces has fixed length */
                        if (com->bin_hdr.fixed_tr_length) {
                                /* assums that fixed length trace should have
                                 * fixed additional trace headers */
                                fseek(
                                    com->file,
                                    (com->bytes_per_sample * com->samp_per_tr +
                                     TRACE_HEADER_SIZE *
                                         com->bin_hdr.max_num_add_tr_headers) *
                                        com->bin_hdr.num_of_tr_in_file,
                                    SEEK_CUR);
                                sgy->end_of_data = ftell(com->file);
                                char *end_stanza = "((SEG: EndText))";
                                while (1) {
                                        TRY(read_text_header(
                                            sgy, seis_common_segy_add_stanza,
                                            1));
                                        size_t stanz_read =
                                            seis_common_segy_get_stanzas_num(
                                                com);
                                        char const *last_stanz =
                                            seis_common_segy_get_stanza(
                                                com, stanz_read - 1);
                                        if (!strstr(last_stanz, end_stanza))
                                                goto error;
                                }
                                /* if traces has variable trace length */
                        } else {
                                for (uint64_t i = 0;
                                     i < com->bin_hdr.num_of_tr_in_file; ++i) {
                                        TRY(fill_from_file(sgy, com->hdr_buf,
                                                           TRACE_HEADER_SIZE));
                                        char const *ptr = com->hdr_buf + 114;
                                        uint32_t trc_samp_num =
                                            sgy->read_i16(&ptr);
                                        if (com->bin_hdr
                                                .max_num_add_tr_headers) {
                                                TRY(fill_from_file(
                                                    sgy, com->hdr_buf,
                                                    TRACE_HEADER_SIZE));
                                                ptr = com->hdr_buf + 136;
                                                trc_samp_num =
                                                    sgy->read_u32(&ptr);
                                                ptr = com->hdr_buf + 156;
                                                uint16_t add_tr_hdr_num =
                                                    sgy->read_i16(&ptr);
                                                add_tr_hdr_num =
                                                    add_tr_hdr_num
                                                        ? add_tr_hdr_num
                                                        : com->bin_hdr
                                                              .max_num_add_tr_headers;
                                                fseek(com->file,
                                                      (add_tr_hdr_num - 1) *
                                                          TRACE_HEADER_SIZE,
                                                      SEEK_CUR);
                                        }
                                        fseek(com->file,
                                              trc_samp_num *
                                                  com->bytes_per_sample,
                                              SEEK_CUR);
                                }
                                sgy->end_of_data = ftell(com->file);
                                char *end_stanza = "((SEG: EndText))";
                                while (1) {
                                        TRY(read_text_header(
                                            sgy, seis_common_segy_add_stanza,
                                            1));
                                        size_t stanz_read =
                                            seis_common_segy_get_stanzas_num(
                                                com);
                                        char const *last_stanz =
                                            seis_common_segy_get_stanza(
                                                com, stanz_read - 1);
                                        if (!strstr(last_stanz, end_stanza))
                                                goto error;
                                }
                        }
                        /* if we know number of stanzas just read them */
                } else {
                        fseek(com->file,
                              com->bin_hdr.num_of_trailer_stanza *
                                  TEXT_HEADER_SIZE,
                              SEEK_END);
                        sgy->end_of_data = ftell(com->file);
                        TRY(read_text_header(
                            sgy, seis_common_segy_add_stanza,
                            com->bin_hdr.num_of_trailer_stanza));
                }
        error:
                if (text_buf)
                        free(text_buf);
                return com->err.code;
        } else {
                fseek(com->file, 0, SEEK_END);
                sgy->end_of_data = ftell(com->file);
        }
        return com->err.code;
}

SeisSegyErrCode read_trc_smpls_fix(SeisISegy *sgy, SeisTraceHeader *hdr,
                                   SeisTrace **trc) {
        SeisCommonSegy *com = sgy->com;
        TRY(fill_from_file(sgy, com->samp_buf,
                           com->samp_per_tr * com->bytes_per_sample));
        char const *ptr = com->samp_buf;
        *trc = seis_trace_new_with_header(com->samp_per_tr, hdr);
        if (!trc) {
                com->err.code = SEIS_SEGY_ERR_NO_MEM;
                com->err.message = "can't get memory at trace sample reading";
        }
        double *samples = seis_trace_get_samples(*trc);
        for (double *end = samples + com->samp_per_tr; samples != end;
             ++samples)
                *samples = sgy->read_sample(sgy, &ptr);
error:
        return com->err.code;
}

SeisSegyErrCode read_trc_smpls_var(SeisISegy *sgy, SeisTraceHeader *hdr,
                                   SeisTrace **trc) {
        SeisCommonSegy *com = sgy->com;
        long long const *samp_num = seis_trace_header_get_int(hdr, "SAMP_NUM");
        if (!samp_num || !*samp_num) {
                com->err.code = SEIS_SEGY_ERR_BROKEN_FILE;
                com->err.message =
                    "variable trace length and no samples number specified";
                goto error;
        }
        if (com->samp_per_tr < *samp_num) {
                com->samp_per_tr = *samp_num;
                void *res =
                    realloc(com->samp_buf, *samp_num * com->bytes_per_sample);
                if (!res) {
                        com->err.code = SEIS_SEGY_ERR_NO_MEM;
                        com->err.message = "can not allocate memory for "
                                           "trace samples reading";
                        goto error;
                }
        }
        TRY(fill_from_file(sgy, com->samp_buf,
                           *samp_num * com->bytes_per_sample));
        char const *ptr = com->samp_buf;
        *trc = seis_trace_new_with_header(*samp_num, hdr);
        if (!trc) {
                com->err.code = SEIS_SEGY_ERR_NO_MEM;
                com->err.message = "can't get memory at trace sample reading";
        }
        double *samples = seis_trace_get_samples(*trc);
        for (double *end = samples + *samp_num; samples != end; ++samples)
                *samples = sgy->read_sample(sgy, &ptr);
error:
        return com->err.code;
}

SeisSegyErrCode skip_trc_smpls_fix(SeisISegy *sgy, SeisTraceHeader *hdr) {
        UNUSED(hdr);
        SeisCommonSegy *com = sgy->com;
        file_skip_bytes(sgy, com->bytes_per_sample * com->samp_per_tr);
        return com->err.code;
}

SeisSegyErrCode skip_trc_smpls_var(SeisISegy *sgy, SeisTraceHeader *hdr) {
        SeisCommonSegy *com = sgy->com;
        long long const *samp_num = seis_trace_header_get_int(hdr, "SAMP_NUM");
        if (!samp_num) {
                com->err.code = SEIS_SEGY_ERR_BROKEN_FILE;
                com->err.message =
                    "variable trace length and zero samples number";
                goto error;
        }
        file_skip_bytes(sgy, com->bytes_per_sample * *samp_num);
error:
        return com->err.code;
}

void fill_hdr_from_fmt_arr(SeisISegy *sgy, single_hdr_fmt_t *arr,
                           SeisTraceHeader *hdr) {
        SeisCommonSegy *com = sgy->com;
        char const *ptr;
        for
                M_EACH(item, *arr, M_OPL_single_hdr_fmt_t()) {
                        ptr = com->hdr_buf + (*item)->offset;
                        switch ((*item)->format) {
                        case i8:
                                seis_trace_header_set_int(
                                    hdr, string_get_cstr((*item)->name),
                                    sgy->read_i8(&ptr));
                                break;
                        case u8:
                                seis_trace_header_set_int(
                                    hdr, string_get_cstr((*item)->name),
                                    sgy->read_u8(&ptr));
                                break;
                        case i16:
                                seis_trace_header_set_int(
                                    hdr, string_get_cstr((*item)->name),
                                    sgy->read_i16(&ptr));
                                break;
                        case u16:
                                seis_trace_header_set_int(
                                    hdr, string_get_cstr((*item)->name),
                                    sgy->read_u16(&ptr));
                                break;
                        case i32:
                                seis_trace_header_set_int(
                                    hdr, string_get_cstr((*item)->name),
                                    sgy->read_i32(&ptr));
                                break;
                        case u32:
                                seis_trace_header_set_int(
                                    hdr, string_get_cstr((*item)->name),
                                    sgy->read_u32(&ptr));
                                break;
                        case i64:
                                seis_trace_header_set_int(
                                    hdr, string_get_cstr((*item)->name),
                                    sgy->read_i64(&ptr));
                                break;
                        case u64:
                                seis_trace_header_set_int(
                                    hdr, string_get_cstr((*item)->name),
                                    sgy->read_u64(&ptr));
                                break;
                        case f32:
                                seis_trace_header_set_real(
                                    hdr, string_get_cstr((*item)->name),
                                    sgy->dbl_from_IEEE_float(sgy, (&ptr)));
                                break;
                        case f64:
                                seis_trace_header_set_real(
                                    hdr, string_get_cstr((*item)->name),
                                    sgy->dbl_from_IEEE_double(sgy, (&ptr)));
                                break;
                        case b64:
                                break;
                        }
                }
}

SeisSegyErrCode read_trc_hdr(SeisISegy *sgy, SeisTraceHeader *hdr) {
        SeisCommonSegy *com = sgy->com;
        SeisCommonSegyPrivate *priv = (SeisCommonSegyPrivate *)com;
        TRY(fill_from_file(sgy, com->hdr_buf, TRACE_HEADER_SIZE));
        fill_hdr_from_fmt_arr(sgy, mult_hdr_fmt_get(priv->trc_hdr_map, 0), hdr);
        if (com->bin_hdr.max_num_add_tr_headers) {
                TRY(fill_from_file(sgy, com->hdr_buf, TRACE_HEADER_SIZE));
                fill_hdr_from_fmt_arr(
                    sgy, mult_hdr_fmt_get(priv->trc_hdr_map, 1), hdr);
                long long const *add_hdr_num =
                    seis_trace_header_get_int(hdr, "ADD_TRC_HDR_NUM");
                int to_read;
                if (!*add_hdr_num)
                        to_read = com->bin_hdr.max_num_add_tr_headers - 1;
                else
                        to_read = *add_hdr_num - 1;
                for (int i = 2; i < 2 + to_read; ++i) {
                        TRY(fill_from_file(sgy, com->hdr_buf,
                                           TRACE_HEADER_SIZE));
                        fill_hdr_from_fmt_arr(
                            sgy, mult_hdr_fmt_get(priv->trc_hdr_map, i), hdr);
                }
        }
error:
        return com->err.code;
}

int8_t read_i8(char const **buf) {
        int8_t res;
        memcpy(&res, *buf, sizeof(int8_t));
        ++*buf;
        return res;
}

uint8_t read_u8(char const **buf) {
        uint8_t res;
        memcpy(&res, *buf, sizeof(uint8_t));
        ++*buf;
        return res;
}

int16_t read_i16(char const **buf) {
        int16_t res;
        memcpy(&res, *buf, sizeof(int16_t));
        *buf += sizeof(int16_t);
        return res;
}

uint16_t read_u16(char const **buf) {
        uint16_t res;
        memcpy(&res, *buf, sizeof(uint16_t));
        *buf += sizeof(uint16_t);
        return res;
}

int32_t read_i24(char const **buf) {
        int32_t res;
        memcpy(&res, *buf, sizeof(int16_t) + sizeof(int8_t));
        *buf += sizeof(int16_t) + sizeof(int8_t);
        return res;
}

uint32_t read_u24(char const **buf) {
        uint32_t res;
        memcpy(&res, *buf, sizeof(uint16_t) + sizeof(uint8_t));
        *buf += sizeof(uint16_t) + sizeof(uint8_t);
        return res;
}

int32_t read_i32(char const **buf) {
        int32_t res;
        memcpy(&res, *buf, sizeof(int32_t));
        *buf += sizeof(int32_t);
        return res;
}

uint32_t read_u32(char const **buf) {
        uint32_t res;
        memcpy(&res, *buf, sizeof(uint32_t));
        *buf += sizeof(uint32_t);
        return res;
}

int64_t read_i64(char const **buf) {
        int64_t res;
        memcpy(&res, *buf, sizeof(int64_t));
        *buf += sizeof(int64_t);
        return res;
}

uint64_t read_u64(char const **buf) {
        uint64_t res;
        memcpy(&res, *buf, sizeof(uint64_t));
        *buf += sizeof(uint64_t);
        return res;
}

int16_t read_i16_sw(char const **buf) {
        uint16_t res;
        memcpy(&res, *buf, sizeof(int16_t));
        *buf += sizeof(uint16_t);
        return (int16_t)((res & 0xff) << 8 | (res & 0xff00) >> 8);
}

uint16_t read_u16_sw(char const **buf) {
        uint16_t res;
        memcpy(&res, *buf, sizeof(uint16_t));
        *buf += sizeof(uint16_t);
        return (res & 0xff) << 8 | (res & 0xff00) >> 8;
}

int32_t read_i24_sw(char const **buf) {
        uint32_t res = 0;
        memcpy(&res, *buf, sizeof(int16_t) + sizeof(int8_t));
        *buf += sizeof(int16_t) + sizeof(int8_t);
        return (int32_t)((res & 0xff) << 24 | (res & 0xff000000) >> 24 |
                         (res & 0xff00) << 8 | (res & 0xff0000) >> 8);
}

uint32_t read_u24_sw(char const **buf) {
        uint32_t res = 0;
        memcpy(&res, *buf, sizeof(uint16_t) + sizeof(uint8_t));
        *buf += sizeof(uint16_t) + sizeof(uint8_t);
        return ((res & 0xff) << 24 | (res & 0xff000000) >> 24 |
                (res & 0xff00) << 8 | (res & 0xff0000) >> 8);
}

int32_t read_i32_sw(char const **buf) {
        uint32_t res;
        memcpy(&res, *buf, sizeof(int32_t));
        *buf += sizeof(uint32_t);
        return (int32_t)((res & 0xff) << 24 | (res & 0xff000000) >> 24 |
                         (res & 0xff00) << 8 | (res & 0xff0000) >> 8);
}

uint32_t read_u32_sw(char const **buf) {
        uint32_t res;
        memcpy(&res, *buf, sizeof(int32_t));
        *buf += sizeof(uint32_t);
        return (res & 0xff) << 24 | (res & 0xff000000) >> 24 |
               (res & 0xff00) << 8 | (res & 0xff0000) >> 8;
}

int64_t read_i64_sw(char const **buf) {
        uint64_t res;
        memcpy(&res, *buf, sizeof(int64_t));
        *buf += sizeof(uint64_t);
        return (int64_t)((res & 0xff) << 56 | (res & 0xff00000000000000) >> 56 |
                         (res & 0xff00) << 40 | (res & 0xff000000000000) >> 40 |
                         (res & 0xff0000) << 24 | (res & 0xff0000000000) >> 24 |
                         (res & 0xff000000) << 8 | (res & 0xff00000000) >> 8);
}

uint64_t read_u64_sw(char const **buf) {
        uint64_t res;
        memcpy(&res, *buf, sizeof(int64_t));
        *buf += sizeof(uint64_t);
        return ((res & 0xff) << 56 | (res & 0xff00000000000000) >> 56 |
                (res & 0xff00) << 40 | (res & 0xff000000000000) >> 40 |
                (res & 0xff0000) << 24 | (res & 0xff0000000000) >> 24 |
                (res & 0xff000000) << 8 | (res & 0xff00000000) >> 8);
}

double dbl_from_IBM_float(SeisISegy *sgy, char const **buf) {
        uint32_t ibm = sgy->read_u32(buf);
        int sign = ibm >> 31 ? -1 : 1;
        int exp = ibm >> 24 & 0x7f;
        uint32_t fraction = ibm & 0x00ffffff;
        return fraction / pow(2, 24) * pow(16, exp - 64) * sign;
}

double dbl_from_IEEE_float(SeisISegy *sgy, char const **buf) {
        uint32_t tmp = sgy->read_u32(buf);
        int sign = tmp >> 31 ? -1 : 1;
        int exp = (tmp & 0x7fffffff) >> 23;
        uint32_t fraction = tmp & 0x7fffff;
        return sign * pow(2, exp - 127) * (1 + fraction / pow(2, 23));
}

double dbl_from_IEEE_double(SeisISegy *sgy, char const **buf) {
        uint64_t tmp = sgy->read_u64(buf);
        int sign = tmp >> 63 ? -1 : 1;
        int exp = (tmp & 0x7fffffffffffffff) >> 52;
        uint64_t fraction = tmp & 0x000fffffffffffff;
        return sign * pow(2, exp - 1023) * (1 + fraction / pow(2, 52));
}

double dbl_from_IEEE_float_native(SeisISegy *sgy, char const **buf) {
        uint32_t tmp = sgy->read_u32(buf);
        float result;
        memcpy(&result, &tmp, sizeof(result));
        return (double)result;
}

double dbl_from_IEEE_double_native(SeisISegy *sgy, char const **buf) {
        uint64_t tmp = sgy->read_u64(buf);
        double result;
        memcpy(&result, &tmp, sizeof(result));
        return result;
}

double dbl_from_i8(SeisISegy *sgy, char const **buf) {
        return sgy->read_i8(buf);
}

double dbl_from_u8(SeisISegy *sgy, char const **buf) {
        return sgy->read_u8(buf);
}

double dbl_from_i16(SeisISegy *sgy, char const **buf) {
        return sgy->read_i16(buf);
}

double dbl_from_u16(SeisISegy *sgy, char const **buf) {
        return sgy->read_u16(buf);
}

double dbl_from_i24(SeisISegy *sgy, char const **buf) {
        return sgy->read_i24(buf);
}

double dbl_from_u24(SeisISegy *sgy, char const **buf) {
        return sgy->read_u24(buf);
}

double dbl_from_i32(SeisISegy *sgy, char const **buf) {
        return sgy->read_i32(buf);
}

double dbl_from_u32(SeisISegy *sgy, char const **buf) {
        return sgy->read_u32(buf);
}

double dbl_from_i64(SeisISegy *sgy, char const **buf) {
        return sgy->read_i64(buf);
}

double dbl_from_u64(SeisISegy *sgy, char const **buf) {
        return sgy->read_u64(buf);
}
