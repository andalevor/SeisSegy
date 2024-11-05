#include "SeisCommonSegy.h"
#include "SeisCommonSegyPrivate.h"
#include "m-string.h"
#include <SeisTrace.h>
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void fill_hdr_map(SeisCommonSegyPrivate *sgy);

SeisCommonSegy *seis_common_segy_new(void) {
        SeisCommonSegyPrivate *priv = (SeisCommonSegyPrivate *)malloc(
            sizeof(struct SeisCommonSegyPrivate));
        if (!priv)
                return NULL;
        memset(&priv->com.bin_hdr, 0, sizeof(struct SeisSegyBinHdr));
        priv->com.err.code = SEIS_SEGY_ERR_OK;
        priv->com.err.message = "";
        priv->com.hdr_buf = (char *)malloc(TRACE_HEADER_SIZE);
        if (!priv->com.hdr_buf)
                return NULL;
        priv->com.samp_buf = NULL;
        priv->com.file = NULL;
        str_arr_init(priv->text_hdrs);
        str_arr_init(priv->end_stanzas);
        mult_hdr_fmt_init(priv->trc_hdr_map);
        fill_hdr_map(priv);
        return (SeisCommonSegy *)priv;
}

void seis_common_segy_unref(SeisCommonSegy **sgy) {
        if (*sgy) {
                SeisCommonSegyPrivate *psgy = (SeisCommonSegyPrivate *)*sgy;
                free(psgy->com.hdr_buf);
                if (psgy->com.samp_buf)
                        free(psgy->com.samp_buf);
                if (psgy->com.file)
                        fclose(psgy->com.file);
                str_arr_clear(psgy->text_hdrs);
                str_arr_clear(psgy->end_stanzas);
                mult_hdr_fmt_clear(psgy->trc_hdr_map);
                free(psgy);
                *sgy = NULL;
        }
}

SeisSegyErrCode seis_common_remap_trace_header(SeisCommonSegy *sgy,
                                               char const *hdr_name,
                                               int hdr_num, int offset,
                                               enum FORMAT format) {
        SeisCommonSegyPrivate *priv = (SeisCommonSegyPrivate *)sgy;
        if (hdr_num - 1 > sgy->bin_hdr.max_num_add_tr_headers) {
                sgy->err.code = SEIS_SEGY_ERR_BAD_PARAMS;
                sgy->err.message =
                    "number of header is greater than max number of additional "
                    "trace headers in binary header";
                goto error;
        }
        int hdr_size;
        switch (format) {
        case i8:
        case u8:
                hdr_size = 1;
                break;
        case i16:
        case u16:
                hdr_size = 2;
                break;
        case i32:
        case u32:
        case f32:
                hdr_size = 4;
                break;
        case i64:
        case u64:
        case f64:
        case b64:
                hdr_size = 8;
                break;
        }
        if (offset + hdr_size - 1 > TRACE_HEADER_SIZE) {
                sgy->err.code = SEIS_SEGY_ERR_BAD_PARAMS;
                sgy->err.message = "it is impossible to write more than 240 "
                                   "bytes to trace header";
                goto error;
        }
        int hdrs_in_map = mult_hdr_fmt_size(priv->trc_hdr_map);
        if (hdrs_in_map < hdr_num)
                mult_hdr_fmt_resize(priv->trc_hdr_map, hdr_num);
        single_hdr_fmt_t *h = mult_hdr_fmt_get(priv->trc_hdr_map, hdr_num - 1);
        hdr_fmt_t fmt;
        hdr_fmt_init(fmt);
        string_init_set(fmt->name, hdr_name);
        fmt->offset = offset - 1;
        fmt->format = format;
        single_hdr_fmt_push_back(*h, fmt);
        hdr_fmt_clear(fmt);
error:
        return sgy->err.code;
}
void seis_common_segy_set_text_header(SeisCommonSegy *com, size_t idx,
                                      char const *hdr) {
        SeisCommonSegyPrivate *priv = (SeisCommonSegyPrivate *)com;
        assert(idx < str_arr_size(priv->text_hdrs));
        string_t tmp;
        string_init(tmp);
        string_set_strn(tmp, hdr, TEXT_HEADER_SIZE);
        str_arr_set_at(priv->text_hdrs, idx, tmp);
        string_clear(tmp);
}

void seis_common_segy_add_text_header(SeisCommonSegy *com, char const *buf) {
        assert(strlen(buf) == TEXT_HEADER_SIZE);
        SeisCommonSegyPrivate *priv = (SeisCommonSegyPrivate *)com;
        str_arr_emplace_back(priv->text_hdrs, buf);
}

void seis_common_segy_add_stanza(SeisCommonSegy *com, char const *buf) {
        assert(strlen(buf) == TEXT_HEADER_SIZE);
        SeisCommonSegyPrivate *priv = (SeisCommonSegyPrivate *)com;
        str_arr_emplace_back(priv->end_stanzas, buf);
}

size_t seis_common_segy_get_text_headers_num(SeisCommonSegy const *com) {
        SeisCommonSegyPrivate *priv = (SeisCommonSegyPrivate *)com;
        return str_arr_size(priv->text_hdrs);
}

char const *seis_common_segy_get_text_header(SeisCommonSegy const *com,
                                             size_t idx) {
        SeisCommonSegyPrivate *priv = (SeisCommonSegyPrivate *)com;
        string_t *tmp = str_arr_get(priv->text_hdrs, idx);
        return string_get_cstr(*tmp);
}

size_t seis_common_segy_get_stanzas_num(SeisCommonSegy const *com) {
        SeisCommonSegyPrivate *priv = (SeisCommonSegyPrivate *)com;
        return str_arr_size(priv->end_stanzas);
}

char const *seis_common_segy_get_stanza(SeisCommonSegy const *com, size_t idx) {
        SeisCommonSegyPrivate *priv = (SeisCommonSegyPrivate *)com;
        string_t *tmp = str_arr_get(priv->end_stanzas, idx);
        return string_get_cstr(*tmp);
}

void fill_hdr_map(SeisCommonSegyPrivate *psgy) {
        hdr_fmt_t fmt;
        hdr_fmt_init(fmt);
        single_hdr_fmt_t hdr;
        single_hdr_fmt_init(hdr);
        string_set_str(fmt->name, "TRC_SEQ_LINE");
        fmt->offset = 0;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "TRC_SEQ_SGY");
        fmt->offset = 4;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "FFID");
        fmt->offset = 8;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "CHAN");
        fmt->offset = 12;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "ESP");
        fmt->offset = 16;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "ENS_NO");
        fmt->offset = 20;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SEQ_NO");
        fmt->offset = 24;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "TRACE_ID");
        fmt->offset = 28;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "VERT_SUM");
        fmt->offset = 30;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "HOR_SUM");
        fmt->offset = 32;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "DATA_USE");
        fmt->offset = 34;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "OFFSET");
        fmt->offset = 36;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "R_ELEV");
        fmt->offset = 40;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "S_ELEV");
        fmt->offset = 44;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "S_DEPTH");
        fmt->offset = 48;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "R_DATUM");
        fmt->offset = 52;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "S_DATUM");
        fmt->offset = 56;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "S_WATER");
        fmt->offset = 60;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "R_WATER");
        fmt->offset = 64;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "ELEV_SCALAR");
        fmt->offset = 68;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "COORD_SCALAR");
        fmt->offset = 70;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SOU_X");
        fmt->offset = 72;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SOU_Y");
        fmt->offset = 76;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "REC_X");
        fmt->offset = 80;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "REC_Y");
        fmt->offset = 84;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "COORD_UNITS");
        fmt->offset = 88;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "WEATH_VEL");
        fmt->offset = 90;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SUBWEATH_VEL");
        fmt->offset = 92;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "S_UPHOLE");
        fmt->offset = 94;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "R_UPHOLE");
        fmt->offset = 96;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "S_STAT");
        fmt->offset = 98;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "R_STAT");
        fmt->offset = 100;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "TOT_STAT");
        fmt->offset = 102;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "LAG_A");
        fmt->offset = 104;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "LAG_B");
        fmt->offset = 106;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "DELAY_TIME");
        fmt->offset = 108;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "MUTE_START");
        fmt->offset = 110;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "MUTE_END");
        fmt->offset = 112;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SAMP_NUM");
        fmt->offset = 114;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SAMP_INT");
        fmt->offset = 116;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "GAIN_TYPE");
        fmt->offset = 118;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "GAIN_CONST");
        fmt->offset = 120;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "INIT_GAIN");
        fmt->offset = 122;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "CORRELATED");
        fmt->offset = 124;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SW_START");
        fmt->offset = 126;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SW_END");
        fmt->offset = 128;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SW_LENGTH");
        fmt->offset = 130;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SW_TYPE");
        fmt->offset = 132;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SW_TAPER_START");
        fmt->offset = 134;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SW_TAPER_END");
        fmt->offset = 136;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "TAPER_TYPE");
        fmt->offset = 138;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "ALIAS_FILT_FREQ");
        fmt->offset = 140;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "ALIAS_FILT_SLOPE");
        fmt->offset = 142;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "NOTCH_FILT_FREQ");
        fmt->offset = 144;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "NOTCH_FILT_SLOPE");
        fmt->offset = 146;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "LOW_CUT_FREQ");
        fmt->offset = 148;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "HIGH_CUT_FREQ");
        fmt->offset = 150;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "LOW_CUT_SLOPE");
        fmt->offset = 152;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "HIGH_CUT_SLOPE");
        fmt->offset = 154;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "YEAR");
        fmt->offset = 156;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "DAY");
        fmt->offset = 158;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "HOUR");
        fmt->offset = 160;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "MINUTE");
        fmt->offset = 162;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SECOND");
        fmt->offset = 164;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "TIME_BASIS_CODE");
        fmt->offset = 166;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "TRACE_WEIGHT");
        fmt->offset = 168;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "GROUP_NUM_ROLL");
        fmt->offset = 170;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "GROUP_NUM_FIRST");
        fmt->offset = 172;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "GROUP_NUM_LAST");
        fmt->offset = 174;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "GAP_SIZE");
        fmt->offset = 176;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "OVER_TRAVEL");
        fmt->offset = 178;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "CDP_X");
        fmt->offset = 180;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "CDP_Y");
        fmt->offset = 184;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "INLINE");
        fmt->offset = 188;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "XLINE");
        fmt->offset = 192;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SP_NUM");
        fmt->offset = 196;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SP_NUM_SCALAR");
        fmt->offset = 200;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "TR_VAL_UNIT");
        fmt->offset = 202;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "TRANS_CONST_MANT");
        fmt->offset = 204;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "TRANS_CONST_EXP");
        fmt->offset = 208;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "TRANS_UNITS");
        fmt->offset = 210;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "DEVICE_ID");
        fmt->offset = 212;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "TIME_SCALAR");
        fmt->offset = 214;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SOURCE_TYPE");
        fmt->offset = 216;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SOU_V_DIR");
        fmt->offset = 218;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SOU_X_DIR");
        fmt->offset = 220;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SOU_I_DIR");
        fmt->offset = 222;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SOU_MEAS_MANT");
        fmt->offset = 224;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SOU_MEAS_EXP");
        fmt->offset = 228;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SOU_MEAS_UNIT");
        fmt->offset = 230;
        fmt->format = i16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SEG00000");
        fmt->offset = 232;
        fmt->format = b64;
        single_hdr_fmt_push_back(hdr, fmt);
        mult_hdr_fmt_push_back(psgy->trc_hdr_map, hdr);
        single_hdr_fmt_reset(hdr);
        string_set_str(fmt->name, "TRC_SEQ_LINE");
        fmt->offset = 0;
        fmt->format = u64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "TRC_SEQ_SGY");
        fmt->offset = 8;
        fmt->format = u64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "FFID");
        fmt->offset = 16;
        fmt->format = i64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "ESN_NO");
        fmt->offset = 24;
        fmt->format = i64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "R_ELEV_R");
        fmt->offset = 32;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "R_DEPTH_R");
        fmt->offset = 40;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "S_ELEV_R");
        fmt->offset = 48;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "S_DEPTH_R");
        fmt->offset = 56;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "R_DATUM_R");
        fmt->offset = 64;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "S_DATUM_R");
        fmt->offset = 72;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "S_WATER_R");
        fmt->offset = 80;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "R_WATER_R");
        fmt->offset = 88;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SOU_X_R");
        fmt->offset = 96;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SOU_Y_R");
        fmt->offset = 104;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "REC_X_R");
        fmt->offset = 112;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "REC_Y_R");
        fmt->offset = 120;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "OFFSET_R");
        fmt->offset = 128;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SAMP_NUM");
        fmt->offset = 136;
        fmt->format = u32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "NANOSEC");
        fmt->offset = 140;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SAMP_INT_R");
        fmt->offset = 144;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SENS_ID");
        fmt->offset = 152;
        fmt->format = i32;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "ADD_TRC_HDR");
        fmt->offset = 156;
        fmt->format = u16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "LAST_TR_FLAG");
        fmt->offset = 158;
        fmt->format = u16;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "CDP_X_R");
        fmt->offset = 160;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "CDP_X_R");
        fmt->offset = 168;
        fmt->format = f64;
        single_hdr_fmt_push_back(hdr, fmt);
        string_set_str(fmt->name, "SEG00001");
        fmt->offset = 232;
        fmt->format = b64;
        single_hdr_fmt_push_back(hdr, fmt);
        mult_hdr_fmt_push_back(psgy->trc_hdr_map, hdr);
        single_hdr_fmt_clear(hdr);
        hdr_fmt_clear(fmt);
}

char const *seis_segy_default_text_header_rev0 =
    "C 1 CLIENT                        COMPANY                       CREW NO   "
    "      "
    "C 2 LINE            AREA                        MAP ID                    "
    "      "
    "C 3 REEL NO           DAY-START OF REEL     YEAR      OBSERVER            "
    "      "
    "C 4 INSTRUMENT: MFG            MODEL            SERIAL NO                 "
    "      "
    "C 5 DATA TRACES/RECORD        AUXILIARY TRACES/RECORD         CDP FOLD    "
    "      "
    "C 6 SAMPLE INTERVAL        SAMPLES/TRACE        BITS/IN     BYTES/SAMPLE  "
    "      "
    "C 7 RECORDING FORMAT       FORMAT THIS REEL         MEASUREMENT SYSTEM    "
    "      "
    "C 8 SAMPLE CODE: FLOATING PT     FIXED PT     FIXED PT-GAIN     "
    "CORRELATED      "
    "C 9 GAIN  TYPE: FIXED     BINARY     FLOATING POINT     OTHER             "
    "      "
    "C10 FILTERS: ALIAS     HZ  NOTCH     HZ  BAND     -     HZ  SLOPE    -    "
    "DB/OCT"
    "C11 SOURCE: TYPE            NUMBER/POINT        POINT INTERVAL            "
    "      "
    "C12     PATTERN:                           LENGTH        WIDTH            "
    "      "
    "C13 SWEEP: START     HZ END      HZ  LENGTH      MS  CHANNEL NO     TYPE  "
    "      "
    "C14 TAPER: START LENGTH       MS  END LENGTH       MS TYPE                "
    "      "
    "C15 SPREAD: OFFSET        MAX DISTANCE        GROUP INTEVAL               "
    "      "
    "C16 GEOPHONES: PER GROUP     SPACEING    FREQUENCY     MFG          MODEL "
    "      "
    "C17     PATTERN:                           LENGTH        WIDTH            "
    "      "
    "C18 TRACES SORTED BY: RECORD     CDP     OTHER                            "
    "      "
    "C19 AMPLITUDE RECOVERY: NONE      SPHERICAL DIV       AGC    OTHER        "
    "      "
    "C20 MAP PROJECTION                      ZONE ID       COORDINATE UNITS    "
    "      "
    "C21 PROCESSING:                                                           "
    "      "
    "C22 PROCESSING:                                                           "
    "      "
    "C23                                                                       "
    "      "
    "C24                                                                       "
    "      "
    "C25                                                                       "
    "      "
    "C26                                                                       "
    "      "
    "C27                                                                       "
    "      "
    "C28                                                                       "
    "      "
    "C29                                                                       "
    "      "
    "C30                                                                       "
    "      "
    "C31                                                                       "
    "      "
    "C32                                                                       "
    "      "
    "C33                                                                       "
    "      "
    "C34                                                                       "
    "      "
    "C35                                                                       "
    "      "
    "C36                                                                       "
    "      "
    "C37                                                                       "
    "      "
    "C38                                                                       "
    "      "
    "C39                                                                       "
    "      "
    "C40 END TEXTUAL HEADER                                                    "
    "      ";

char const *seis_segy_default_text_header_rev1 =
    "C 1 CLIENT                        COMPANY                       CREW NO   "
    "      "
    "C 2 LINE            AREA                        MAP ID                    "
    "      "
    "C 3 REEL NO           DAY-START OF REEL     YEAR      OBSERVER            "
    "      "
    "C 4 INSTRUMENT: MFG            MODEL            SERIAL NO                 "
    "      "
    "C 5 DATA TRACES/RECORD        AUXILIARY TRACES/RECORD         CDP FOLD    "
    "      "
    "C 6 SAMPLE INTERVAL        SAMPLES/TRACE        BITS/IN     BYTES/SAMPLE  "
    "      "
    "C 7 RECORDING FORMAT       FORMAT THIS REEL         MEASUREMENT SYSTEM    "
    "      "
    "C 8 SAMPLE CODE: FLOATING PT     FIXED PT     FIXED PT-GAIN     "
    "CORRELATED      "
    "C 9 GAIN  TYPE: FIXED     BINARY     FLOATING POINT     OTHER             "
    "      "
    "C10 FILTERS: ALIAS     HZ  NOTCH     HZ  BAND     -     HZ  SLOPE    -    "
    "DB/OCT"
    "C11 SOURCE: TYPE            NUMBER/POINT        POINT INTERVAL            "
    "      "
    "C12     PATTERN:                           LENGTH        WIDTH            "
    "      "
    "C13 SWEEP: START     HZ END      HZ  LENGTH      MS  CHANNEL NO     TYPE  "
    "      "
    "C14 TAPER: START LENGTH       MS  END LENGTH       MS TYPE                "
    "      "
    "C15 SPREAD: OFFSET        MAX DISTANCE        GROUP INTEVAL               "
    "      "
    "C16 GEOPHONES: PER GROUP     SPACEING    FREQUENCY     MFG          MODEL "
    "      "
    "C17     PATTERN:                           LENGTH        WIDTH            "
    "      "
    "C18 TRACES SORTED BY: RECORD     CDP     OTHER                            "
    "      "
    "C19 AMPLITUDE RECOVERY: NONE      SPHERICAL DIV       AGC    OTHER        "
    "      "
    "C20 MAP PROJECTION                      ZONE ID       COORDINATE UNITS    "
    "      "
    "C21 PROCESSING:                                                           "
    "      "
    "C22 PROCESSING:                                                           "
    "      "
    "C23                                                                       "
    "      "
    "C24                                                                       "
    "      "
    "C25                                                                       "
    "      "
    "C26                                                                       "
    "      "
    "C27                                                                       "
    "      "
    "C28                                                                       "
    "      "
    "C29                                                                       "
    "      "
    "C30                                                                       "
    "      "
    "C31                                                                       "
    "      "
    "C32                                                                       "
    "      "
    "C33                                                                       "
    "      "
    "C34                                                                       "
    "      "
    "C35                                                                       "
    "      "
    "C36                                                                       "
    "      "
    "C37                                                                       "
    "      "
    "C38                                                                       "
    "      "
    "C39 SEG Y REV1                                                            "
    "      "
    "C40 END TEXTUAL HEADER                                                    "
    "      ";

char const *seis_segy_default_text_header_rev2 =
    "C 1 CLIENT                        COMPANY                       CREW NO   "
    "      "
    "C 2 LINE            AREA                        MAP ID                    "
    "      "
    "C 3 REEL NO           DAY-START OF REEL     YEAR      OBSERVER            "
    "      "
    "C 4 INSTRUMENT: MFG            MODEL            SERIAL NO                 "
    "      "
    "C 5 DATA TRACES/RECORD        AUXILIARY TRACES/RECORD         CDP FOLD    "
    "      "
    "C 6 SAMPLE INTERVAL        SAMPLES/TRACE        BITS/IN     BYTES/SAMPLE  "
    "      "
    "C 7 RECORDING FORMAT       FORMAT THIS REEL         MEASUREMENT SYSTEM    "
    "      "
    "C 8 SAMPLE CODE: FLOATING PT     FIXED PT     FIXED PT-GAIN     "
    "CORRELATED      "
    "C 9 GAIN  TYPE: FIXED     BINARY     FLOATING POINT     OTHER             "
    "      "
    "C10 FILTERS: ALIAS     HZ  NOTCH     HZ  BAND     -     HZ  SLOPE    -    "
    "DB/OCT"
    "C11 SOURCE: TYPE            NUMBER/POINT        POINT INTERVAL            "
    "      "
    "C12     PATTERN:                           LENGTH        WIDTH            "
    "      "
    "C13 SWEEP: START     HZ END      HZ  LENGTH      MS  CHANNEL NO     TYPE  "
    "      "
    "C14 TAPER: START LENGTH       MS  END LENGTH       MS TYPE                "
    "      "
    "C15 SPREAD: OFFSET        MAX DISTANCE        GROUP INTEVAL               "
    "      "
    "C16 GEOPHONES: PER GROUP     SPACEING    FREQUENCY     MFG          MODEL "
    "      "
    "C17     PATTERN:                           LENGTH        WIDTH            "
    "      "
    "C18 TRACES SORTED BY: RECORD     CDP     OTHER                            "
    "      "
    "C19 AMPLITUDE RECOVERY: NONE      SPHERICAL DIV       AGC    OTHER        "
    "      "
    "C20 MAP PROJECTION                      ZONE ID       COORDINATE UNITS    "
    "      "
    "C21 PROCESSING:                                                           "
    "      "
    "C22 PROCESSING:                                                           "
    "      "
    "C23                                                                       "
    "      "
    "C24                                                                       "
    "      "
    "C25                                                                       "
    "      "
    "C26                                                                       "
    "      "
    "C27                                                                       "
    "      "
    "C28                                                                       "
    "      "
    "C29                                                                       "
    "      "
    "C30                                                                       "
    "      "
    "C31                                                                       "
    "      "
    "C32                                                                       "
    "      "
    "C33                                                                       "
    "      "
    "C34                                                                       "
    "      "
    "C35                                                                       "
    "      "
    "C36                                                                       "
    "      "
    "C37                                                                       "
    "      "
    "C38                                                                       "
    "      "
    "C39 SEG-Y_REV2.0                                                          "
    "      "
    "C40 END TEXTUAL HEADER                                                    "
    "      ";
