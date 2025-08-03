#include "SeisCommonSegy.h"
#include "SeisISegy.h"
#include <SeisTrace.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
        if (argc < 2) {
                printf("Parameters needed\n");
                return 1;
        }
        SeisISegy *isgy = seis_isegy_new();
        if (!isgy)
                return 2;
        SeisSegyErr const *err = seis_isegy_get_error(isgy);
        if (seis_isegy_open(isgy, argv[1]))
                goto error;
        SeisTrace *trc = seis_isegy_read_trace(isgy);
        if (err->code)
                goto error;
        SeisTraceHeader const *hdr = seis_trace_get_header_const(trc);
        SeisTraceHeaderValue v = seis_trace_header_get(hdr, "TRC_SEQ_LINE");
        long long const *p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TRC_SEQ_LINE:", *p_val);
        v = seis_trace_header_get(hdr, "TRC_SEQ_SGY");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TRC_SEQ_SGY:", *p_val);
        v = seis_trace_header_get(hdr, "FFID");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "FFID:", *p_val);
        v = seis_trace_header_get(hdr, "CHAN");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "CHAN:", *p_val);
        v = seis_trace_header_get(hdr, "ESP");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "ESP:", *p_val);
        v = seis_trace_header_get(hdr, "ENS_NO");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "ENS_NO:", *p_val);
        v = seis_trace_header_get(hdr, "SEQ_NO");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SEQ_NO:", *p_val);
        v = seis_trace_header_get(hdr, "TRACE_ID");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TRACE_ID:", *p_val);
        v = seis_trace_header_get(hdr, "VERT_SUM");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "VERT_SUM:", *p_val);
        v = seis_trace_header_get(hdr, "HOR_SUM");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "HOR_SUM:", *p_val);
        v = seis_trace_header_get(hdr, "DATA_USE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "DATA_USE:", *p_val);
        v = seis_trace_header_get(hdr, "OFFSET");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "OFFSET:", *p_val);
        v = seis_trace_header_get(hdr, "R_ELEV");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "R_ELEV:", *p_val);
        v = seis_trace_header_get(hdr, "S_ELEV");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "S_ELEV:", *p_val);
        v = seis_trace_header_get(hdr, "S_DEPTH");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "S_DEPTH:", *p_val);
        v = seis_trace_header_get(hdr, "R_DATUM");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "R_DATUM:", *p_val);
        v = seis_trace_header_get(hdr, "S_DATUM");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "S_DATUM:", *p_val);
        v = seis_trace_header_get(hdr, "S_WATER");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "S_WATER:", *p_val);
        v = seis_trace_header_get(hdr, "R_WATER");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "R_WATER:", *p_val);
        v = seis_trace_header_get(hdr, "ELEV_SCALAR");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "ELEV_SCALAR:", *p_val);
        v = seis_trace_header_get(hdr, "COORD_SCALAR");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "COORD_SCALAR:", *p_val);
        v = seis_trace_header_get(hdr, "SOU_X");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_X:", *p_val);
        v = seis_trace_header_get(hdr, "SOU_Y");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_Y:", *p_val);
        v = seis_trace_header_get(hdr, "REC_X");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "REC_X:", *p_val);
        v = seis_trace_header_get(hdr, "REC_Y");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "REC_Y:", *p_val);
        v = seis_trace_header_get(hdr, "COORD_UNITS");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "COORD_UNITS:", *p_val);
        v = seis_trace_header_get(hdr, "WEATH_VEL");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "WEATH_VEL:", *p_val);
        v = seis_trace_header_get(hdr, "SUBWEATH_VEL");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SUBWEATH_VEL:", *p_val);
        v = seis_trace_header_get(hdr, "S_UPHOLE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "S_UPHOLE:", *p_val);
        v = seis_trace_header_get(hdr, "R_UPHOLE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "R_UPHOLE:", *p_val);
        v = seis_trace_header_get(hdr, "S_STAT");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "S_STAT:", *p_val);
        v = seis_trace_header_get(hdr, "R_STAT");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "R_STAT:", *p_val);
        v = seis_trace_header_get(hdr, "TOT_STAT");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TOT_STAT:", *p_val);
        v = seis_trace_header_get(hdr, "LAG_A");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "LAG_A:", *p_val);
        v = seis_trace_header_get(hdr, "LAG_B");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "LAG_B:", *p_val);
        v = seis_trace_header_get(hdr, "DELAY_TIME");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "DELAY_TIME:", *p_val);
        v = seis_trace_header_get(hdr, "MUTE_START");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "MUTE_START:", *p_val);
        v = seis_trace_header_get(hdr, "MUTE_END");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "MUTE_END:", *p_val);
        v = seis_trace_header_get(hdr, "SAMP_NUM");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SAMP_NUM:", *p_val);
        v = seis_trace_header_get(hdr, "SAMP_INT");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SAMP_INT:", *p_val);
        v = seis_trace_header_get(hdr, "GAIN_TYPE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "GAIN_TYPE:", *p_val);
        v = seis_trace_header_get(hdr, "GAIN_CONST");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "GAIN_CONST:", *p_val);
        v = seis_trace_header_get(hdr, "INIT_GAIN");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "INIT_GAIN:", *p_val);
        v = seis_trace_header_get(hdr, "CORRELATED");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "CORRELATED:", *p_val);
        v = seis_trace_header_get(hdr, "SW_START");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SW_START:", *p_val);
        v = seis_trace_header_get(hdr, "SW_END");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SW_END:", *p_val);
        v = seis_trace_header_get(hdr, "SW_LENGTH");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SW_LENGTH:", *p_val);
        v = seis_trace_header_get(hdr, "SW_TYPE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SW_TYPE:", *p_val);
        v = seis_trace_header_get(hdr, "SW_TAPER_START");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SW_TAPER_START:", *p_val);
        v = seis_trace_header_get(hdr, "SW_TAPER_END");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SW_TAPER_END:", *p_val);
        v = seis_trace_header_get(hdr, "TAPER_TYPE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TAPER_TYPE:", *p_val);
        v = seis_trace_header_get(hdr, "ALIAS_FILT_FREQ");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "ALIAS_FILT_FREQ:", *p_val);
        v = seis_trace_header_get(hdr, "ALIAS_FILT_SLOPE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "ALIAS_FILE_SLOPE:", *p_val);
        v = seis_trace_header_get(hdr, "NOTCH_FILT_FREQ");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "NOTCH_FILE_FREQ:", *p_val);
        v = seis_trace_header_get(hdr, "NOTCH_FILT_SLOPE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "NOTCH_FILT_SLOPE:", *p_val);
        v = seis_trace_header_get(hdr, "LOW_CUT_FREQ");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "LOW_CUT_FREQ:", *p_val);
        v = seis_trace_header_get(hdr, "HIGH_CUT_FREQ");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "HIGH_CUT_FREQ:", *p_val);
        v = seis_trace_header_get(hdr, "LOW_CUT_SLOPE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "LOW_CUT_SLOPE:", *p_val);
        v = seis_trace_header_get(hdr, "HIGH_CUT_SLOPE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "HIGH_CUT_SLOPE:", *p_val);
        v = seis_trace_header_get(hdr, "YEAR");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "YEAR:", *p_val);
        v = seis_trace_header_get(hdr, "DAY");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "DAY:", *p_val);
        v = seis_trace_header_get(hdr, "HOUR");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "HOUR:", *p_val);
        v = seis_trace_header_get(hdr, "MINUTE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "MINUTE:", *p_val);
        v = seis_trace_header_get(hdr, "SECOND");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SECOND:", *p_val);
        v = seis_trace_header_get(hdr, "TIME_BASIS_CODE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TIME_BASIS_CODE:", *p_val);
        v = seis_trace_header_get(hdr, "TRACE_WEIGHT");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TRACE_WEIGHT:", *p_val);
        v = seis_trace_header_get(hdr, "GROUP_NUM_ROLL");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "GROUP_NUM_ROLL:", *p_val);
        v = seis_trace_header_get(hdr, "GROUP_NUM_LAST");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "GROUP_NUM_LAST:", *p_val);
        v = seis_trace_header_get(hdr, "GAP_SIZE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "GAP_SIZE:", *p_val);
        v = seis_trace_header_get(hdr, "OVER_TRAVEL");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "OVER_TRAVEL:", *p_val);
        v = seis_trace_header_get(hdr, "CDP_X");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "CDP_X:", *p_val);
        v = seis_trace_header_get(hdr, "CDP_Y");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "CDP_Y:", *p_val);
        v = seis_trace_header_get(hdr, "INLINE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "INLINE:", *p_val);
        v = seis_trace_header_get(hdr, "XLINE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "XLINE:", *p_val);
        v = seis_trace_header_get(hdr, "SP_NUM");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SP_NUM:", *p_val);
        v = seis_trace_header_get(hdr, "SP_NUM_SCALAR");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SP_NUM_SCALAR:", *p_val);
        v = seis_trace_header_get(hdr, "TR_VAL_UNIT");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TR_VAL_UNIT:", *p_val);
        v = seis_trace_header_get(hdr, "TRANS_CONST_MANT");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TRANS_CONST_MANT:", *p_val);
        v = seis_trace_header_get(hdr, "TRANS_CONST_EXP");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TRANS_CONST_EXP:", *p_val);
        v = seis_trace_header_get(hdr, "TRANS_UNITS");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TRANS_UNITS:", *p_val);
        v = seis_trace_header_get(hdr, "DEVICE_ID");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "DEVICE_ID:", *p_val);
        v = seis_trace_header_get(hdr, "TIME_SCALAR");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TIME_SCALAR:", *p_val);
        v = seis_trace_header_get(hdr, "SOURCE_TYPE");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOURCE_TYPE:", *p_val);
        v = seis_trace_header_get(hdr, "SOU_V_DIR");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_V_DIR:", *p_val);
        v = seis_trace_header_get(hdr, "SOU_X_DIR");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_X_DIR:", *p_val);
        v = seis_trace_header_get(hdr, "SOU_I_DIR");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_I_DIR:", *p_val);
        v = seis_trace_header_get(hdr, "SOU_MEAS_MANT");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_MEAS_MANT:", *p_val);
        v = seis_trace_header_get(hdr, "SOU_MEAS_EXP");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_MEAS_EXP:", *p_val);
        v = seis_trace_header_get(hdr, "SOU_MEAS_UNIT");
        p_val = seis_trace_header_value_get_int(v);
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_MEAS_UNIT:", *p_val);
        seis_isegy_unref(&isgy);
        seis_trace_unref(&trc);
        return 0;
error:
        if (err->code)
                printf("%s\n", err->message);
        seis_isegy_unref(&isgy);
        seis_trace_unref(&trc);
        return 1;
}
