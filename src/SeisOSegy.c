#include "SeisOSegy.h"
#include "SeisCommonSegy.h"
#include "SeisEncodings.h"
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

static SeisSegyErrCode assign_raw_writers(SeisOSegy *sgy);
static SeisSegyErrCode assign_sample_writer(SeisOSegy *sgy);
static SeisSegyErrCode assign_bytes_per_sample(SeisOSegy *sgy);
static SeisSegyErrCode write_text_header(SeisOSegy *sgy);
static SeisSegyErrCode write_bin_header(SeisOSegy *sgy);
static SeisSegyErrCode write_ext_text_headers(SeisOSegy *sgy);
static SeisSegyErrCode write_trailer_stanzas(SeisOSegy *sgy);
static SeisSegyErrCode write_trace_header(SeisOSegy *sgy,
                                          SeisTraceHeader const *hdr);
static SeisSegyErrCode
write_additional_trace_headers(SeisOSegy *sgy, SeisTraceHeader const *hdr);
static SeisSegyErrCode dummy(SeisOSegy *sgy, SeisTraceHeader const *hdr);
static SeisSegyErrCode write_trace_samples_fix(SeisOSegy *sgy,
                                               SeisTrace const *t);
static SeisSegyErrCode write_trace_samples_var(SeisOSegy *sgy,
                                               SeisTrace const *t);

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

SeisOSegy *seis_osegy_new() {
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

void seis_osegy_unref(SeisOSegy *sgy) {
        if (!--sgy->rc) {
                if (!sgy->com->err.code) {
                        if (sgy->com->bin_hdr.SEGY_rev_major_ver > 1)
                                write_trailer_stanzas(sgy);
                        /* update samp_per_tr value for variable trace length
                         * SEGYs */
                        if (!sgy->com->bin_hdr.fixed_tr_length &&
                            sgy->com->bin_hdr.SEGY_rev_major_ver) {
                                sgy->com->bin_hdr.samp_per_tr =
                                    sgy->com->bin_hdr.ext_samp_per_tr =
                                        sgy->com->samp_per_tr;
                                fseek(sgy->com->file, TEXT_HEADER_SIZE,
                                      SEEK_SET);
                                write_bin_header(sgy);
                        }
                }
                seis_common_segy_unref(sgy->com);
                free(sgy);
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
        sgy->com->samp_per_tr = com->bin_hdr.ext_samp_per_tr
                                    ? com->bin_hdr.ext_samp_per_tr
                                    : com->bin_hdr.samp_per_tr;
        sgy->com->samp_buf =
            (char *)malloc(com->samp_per_tr * com->bytes_per_sample);
        if (!sgy->com->samp_buf) {
                com->err.code = SEIS_SEGY_ERR_NO_MEM;
                com->err.message = "can't allocate memory in osegy open";
                goto error;
        }
        if (!sgy->com->bin_hdr.SEGY_rev_major_ver ||
            !sgy->com->bin_hdr.max_num_add_tr_headers)
                sgy->write_add_trc_hdrs = dummy;
        else
                sgy->write_add_trc_hdrs = write_additional_trace_headers;
        if (sgy->com->bin_hdr.fixed_tr_length ||
            !sgy->com->bin_hdr.SEGY_rev_major_ver)
                sgy->write_trace_samples = write_trace_samples_fix;
        else
                sgy->write_trace_samples = write_trace_samples_var;
        if (sgy->com->bin_hdr.SEGY_rev_major_ver)
                TRY(write_ext_text_headers(sgy));
error:
        return com->err.code;
}

void seis_osegy_add_traler_stanza(SeisOSegy *sgy, char *buf) {
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
                buf = (char *)malloc(TEXT_HEADER_SIZE);
                if (!buf) {
                        com->err.code = SEIS_SEGY_ERR_NO_MEM;
                        com->err.message = "no memory for text header";
                }
                switch (com->bin_hdr.SEGY_rev_major_ver) {
                case 0:
                default:
                        strncpy(buf, seis_segy_default_text_header_rev0,
                                TEXT_HEADER_SIZE);
                        break;
                case 1:
                        strncpy(buf, seis_segy_default_text_header_rev1,
                                TEXT_HEADER_SIZE);
                        break;
                case 2:
                        strncpy(buf, seis_segy_default_text_header_rev2,
                                TEXT_HEADER_SIZE);
                        break;
                }
                ascii_to_ebcdic(buf);
                hdr = buf;
        } else {
                hdr = seis_common_segy_get_text_header(com, 0);
        }
        size_t written = fwrite(hdr, 1, TEXT_HEADER_SIZE, com->file);
        if (written != TEXT_HEADER_SIZE) {
                com->err.code = SEIS_SEGY_ERR_FILE_WRITE;
                com->err.message = "i/o error on text header writing";
        }
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
        size_t written = fwrite(bin_buf, 1, BIN_HEADER_SIZE, com->file);
        if (written != BIN_HEADER_SIZE) {
                com->err.code = SEIS_SEGY_ERR_FILE_WRITE;
                com->err.message = "i/o error on binary header writing";
        }
        free(bin_buf);
        return com->err.code;
}

SeisSegyErrCode write_ext_text_headers(SeisOSegy *sgy) {
        SeisCommonSegy *com = sgy->com;
        size_t num = seis_common_segy_get_text_headers_num(com);
        for (size_t i = 1; i < num; ++i) {
                char const *hdr = seis_common_segy_get_text_header(com, i);
                size_t written = fwrite(hdr, 1, TEXT_HEADER_SIZE, com->file);
                if (written != TEXT_HEADER_SIZE) {
                        com->err.code = SEIS_SEGY_ERR_FILE_WRITE;
                        com->err.message = "i/o error on ext header writing";
                }
        }
        return com->err.code;
}

SeisSegyErrCode write_trailer_stanzas(SeisOSegy *sgy) {
        SeisCommonSegy *com = sgy->com;
        size_t num = seis_common_segy_get_stanzas_num(com);
        for (size_t i = 1; i < num; ++i) {
                char const *hdr = seis_common_segy_get_stanza(com, i);
                size_t written = fwrite(hdr, 1, TEXT_HEADER_SIZE, com->file);
                if (written != TEXT_HEADER_SIZE) {
                        com->err.code = SEIS_SEGY_ERR_FILE_WRITE;
                        com->err.message =
                            "i/o error on trailer stanza writing";
                }
        }
        return com->err.code;
}

SeisSegyErrCode write_trace_header(SeisOSegy *sgy, SeisTraceHeader const *hdr) {
        SeisCommonSegy *com = sgy->com;
        char *buf = com->hdr_buf;
        memset(buf, 0, TRACE_HEADER_SIZE);
        long long const *i = seis_trace_header_get_int(hdr, "TRC_SEQ_LINE");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "TRC_SEQ_SGY");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "FFID");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "CHAN");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "ESP");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "ENS_NO");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SEQ_NO");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "TRACE_ID");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "VERT_SUM");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "HOR_SUM");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "DATA_USE");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "OFFSET");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "R_ELEV");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "S_ELEV");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "S_DEPTH");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "R_DATUM");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "S_DATUM");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "S_WATER");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "R_WATER");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "ELEV_SCALAR");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "COORD_SCALAR");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SOU_X");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SOU_Y");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "REC_X");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "REC_Y");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "COORD_UNITS");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "WEATH_VEL");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SUBWEATH_VEL");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "S_UPHOLE");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "R_UPHOLE");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "S_STAT");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "R_STAT");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "TOT_STAT");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "LAG_A");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "LAG_B");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "DELAY_TIME");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "MUTE_START");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "MUTE_END");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SAMP_NUM");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SAMP_INT");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "GAIN_TYPE");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "GAIN_CONST");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "INIT_GAIN");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "CORRELATED");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SW_START");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SW_END");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SW_LENGTH");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SW_TYPE");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SW_TAPER_START");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SW_TAPER_END");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "TAPER_TYPE");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "ALIAS_FILT_FREQ");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "ALIAS_FILT_SLOPE");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "NOTCH_FILT_FREQ");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "NOTCH_FILT_SLOPE");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "LOW_CUT_FREQ");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "HIGH_CUT_FREQ");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "LOW_CUT_SLOPE");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "HIGH_CUT_SLOPE");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "YEAR");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "DAY");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "HOUR");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "MINUTE");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SECOND");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "TIME_BASIS_CODE");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "TRACE_WEIGHT");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "GROUP_NUM_ROLL");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "GROUP_NUM_FIRST");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "GROUP_NUM_LAST");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "GAP_SIZE");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "OVER_TRAVEL");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "CDP_X");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "CDP_Y");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "INLINE");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "XLINE");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SP_NUM");
        sgy->write_i32(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SP_NUM_SCALAR");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "TR_VAL_UNIT");
        sgy->write_i16(&buf, i ? *i : 0);
        double const *d = seis_trace_header_get_real(hdr, "TRANS_CONST");
        double val = d ? *d : 1;
        int16_t exp = log10(val);
        sgy->write_i32(&buf, val / pow(10, exp));
        sgy->write_i16(&buf, exp);
        i = seis_trace_header_get_int(hdr, "TRANS_UNITS");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "DEVICE_ID");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "TIME_SCALAR");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SOURCE_TYPE");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SOU_V_DIR");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SOU_X_DIR");
        sgy->write_i16(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "SOU_I_DIR");
        sgy->write_i16(&buf, i ? *i : 0);
        d = seis_trace_header_get_real(hdr, "SOURCE_MEASUREMENT");
        val = d ? *d : 1;
        exp = log10(val);
        sgy->write_i32(&buf, val / pow(10, exp));
        sgy->write_i16(&buf, exp);
        i = seis_trace_header_get_int(hdr, "SOU_MEAS_UNIT");
        sgy->write_i16(&buf, i ? *i : 0);
        size_t written = fwrite(com->hdr_buf, 1, TRACE_HEADER_SIZE, com->file);
        if (written != TRACE_HEADER_SIZE) {
                com->err.code = SEIS_SEGY_ERR_FILE_WRITE;
                com->err.message = "i/o error on trace header writing";
        }
        sgy->write_add_trc_hdrs(sgy, hdr);
        return com->err.code;
}

SeisSegyErrCode write_additional_trace_headers(SeisOSegy *sgy,
                                               SeisTraceHeader const *hdr) {
        SeisCommonSegy *com = sgy->com;
        char *buf = com->hdr_buf;
        memset(buf, 0, TRACE_HEADER_SIZE);
        long long const *i = seis_trace_header_get_int(hdr, "TRC_SEQ_LINE");
        sgy->write_u64(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "TRC_SEQ_SGY");
        sgy->write_u64(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "FFID");
        sgy->write_u64(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "ENS_NO");
        sgy->write_u64(&buf, i ? *i : 0);
        double const *d = seis_trace_header_get_real(hdr, "R_ELEV");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        d = seis_trace_header_get_real(hdr, "R_DEPTH");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        d = seis_trace_header_get_real(hdr, "S_DEPTH");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        d = seis_trace_header_get_real(hdr, "R_DATUM");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        d = seis_trace_header_get_real(hdr, "S_DATUM");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        d = seis_trace_header_get_real(hdr, "R_WATER");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        d = seis_trace_header_get_real(hdr, "S_WATER");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        d = seis_trace_header_get_real(hdr, "SOU_X");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        d = seis_trace_header_get_real(hdr, "SOU_Y");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        d = seis_trace_header_get_real(hdr, "REC_X");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        d = seis_trace_header_get_real(hdr, "REC_Y");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        d = seis_trace_header_get_real(hdr, "OFFSET");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        i = seis_trace_header_get_int(hdr, "SAMP_NUM");
        sgy->write_u64(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "NANOSECOND");
        sgy->write_u64(&buf, i ? *i : 0);
        d = seis_trace_header_get_real(hdr, "SAMP_INT");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        i = seis_trace_header_get_int(hdr, "CABLE_NUM");
        sgy->write_u64(&buf, i ? *i : 0);
        i = seis_trace_header_get_int(hdr, "ADD_TRC_HDR_NUM");
        uint16_t add_trc_hdr_num = i ? *i : 0;
        add_trc_hdr_num = add_trc_hdr_num ? add_trc_hdr_num
                                          : com->bin_hdr.max_num_add_tr_headers;
        sgy->write_u64(&buf, add_trc_hdr_num);
        i = seis_trace_header_get_int(hdr, "LAST_TRC_FLAG");
        sgy->write_u64(&buf, i ? *i : 0);
        d = seis_trace_header_get_real(hdr, "CDP_X");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        d = seis_trace_header_get_real(hdr, "CDP_Y");
        sgy->write_IEEE_double(sgy, &buf, d ? *d : 0);
        size_t written = fwrite(com->hdr_buf, 1, TRACE_HEADER_SIZE, com->file);
        if (written != TRACE_HEADER_SIZE) {
                com->err.code = SEIS_SEGY_ERR_FILE_WRITE;
                com->err.message =
                    "i/o error on additional trace header writing";
                goto error;
        }
        for (int i = 0; i < add_trc_hdr_num - 1; ++i) {
                memset(com->hdr_buf, 0, TRACE_HEADER_SIZE);
                /* TODO: writing custom additional headers */
                size_t written =
                    fwrite(com->hdr_buf, 1, TRACE_HEADER_SIZE, com->file);
                if (written != TRACE_HEADER_SIZE) {
                        com->err.code = SEIS_SEGY_ERR_FILE_WRITE;
                        com->err.message =
                            "i/o error on additional trace header writing";
                        goto error;
                }
        }
error:
        return com->err.code;
}

SeisSegyErrCode write_trace_samples_fix(SeisOSegy *sgy, SeisTrace const *t) {
        SeisCommonSegy *com = sgy->com;
        char *buf = com->samp_buf;
        double const *samples = seis_trace_get_samples_const(t);
        long long const sam_num = seis_trace_get_samples_num(t);
        for (long long i = 0; i < sam_num; ++i)
                sgy->write_sample(sgy, &buf, samples[i]);
        size_t written =
            fwrite(com->samp_buf, 1, com->bytes_per_sample * com->samp_per_tr,
                   com->file);
        if (written != com->bytes_per_sample * com->samp_per_tr) {
                com->err.code = SEIS_SEGY_ERR_FILE_WRITE;
                com->err.message = "i/o error on fixed trace writing";
        }
        return com->err.code;
}

SeisSegyErrCode write_trace_samples_var(SeisOSegy *sgy, SeisTrace const *t) {
        SeisCommonSegy *com = sgy->com;
        SeisTraceHeader const *hdr = seis_trace_get_header_const(t);
        long long const *samp_num = seis_trace_header_get_int(hdr, "SAMP_NUM");
        assert(samp_num);
        long long bytes_num = *samp_num * com->bytes_per_sample;
        if (*samp_num != com->samp_per_tr) {
                com->samp_buf = realloc(com->samp_buf, bytes_num);
                if (!com->samp_buf) {
                        com->err.code = SEIS_SEGY_ERR_NO_MEM;
                        com->err.message =
                            "can't get memory to write variable trace";
                        goto error;
                }
                com->samp_per_tr = *samp_num;
        }
        return write_trace_samples_fix(sgy, t);
error:
        return com->err.code;
}

SeisSegyErrCode dummy(SeisOSegy *sgy, SeisTraceHeader const *hdr) {
        return SEIS_SEGY_ERR_OK;
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
