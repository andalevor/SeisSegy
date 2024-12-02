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
        long long const *p_val = seis_trace_header_get_int(hdr, "TRC_SEQ_LINE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TRC_SEQ_LINE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "TRC_SEQ_SGY");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TRC_SEQ_SGY:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "FFID");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "FFID:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "CHAN");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "CHAN:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "ESP");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "ESP:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "ENS_NO");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "ENS_NO:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SEQ_NO");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SEQ_NO:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "TRACE_ID");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TRACE_ID:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "VERT_SUM");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "VERT_SUM:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "HOR_SUM");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "HOR_SUM:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "DATA_USE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "DATA_USE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "OFFSET");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "OFFSET:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "R_ELEV");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "R_ELEV:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "S_ELEV");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "S_ELEV:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "S_DEPTH");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "S_DEPTH:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "R_DATUM");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "R_DATUM:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "S_DATUM");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "S_DATUM:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "S_WATER");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "S_WATER:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "R_WATER");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "R_WATER:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "ELEV_SCALAR");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "ELEV_SCALAR:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "COORD_SCALAR");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "COORD_SCALAR:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SOU_X");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_X:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SOU_Y");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_Y:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "REC_X");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "REC_X:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "REC_Y");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "REC_Y:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "COORD_UNITS");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "COORD_UNITS:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "WEATH_VEL");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "WEATH_VEL:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SUBWEATH_VEL");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SUBWEATH_VEL:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "S_UPHOLE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "S_UPHOLE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "R_UPHOLE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "R_UPHOLE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "S_STAT");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "S_STAT:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "R_STAT");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "R_STAT:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "TOT_STAT");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TOT_STAT:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "LAG_A");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "LAG_A:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "LAG_B");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "LAG_B:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "DELAY_TIME");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "DELAY_TIME:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "MUTE_START");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "MUTE_START:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "MUTE_END");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "MUTE_END:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SAMP_NUM");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SAMP_NUM:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SAMP_INT");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SAMP_INT:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "GAIN_TYPE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "GAIN_TYPE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "GAIN_CONST");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "GAIN_CONST:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "INIT_GAIN");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "INIT_GAIN:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "CORRELATED");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "CORRELATED:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SW_START");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SW_START:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SW_END");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SW_END:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SW_LENGTH");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SW_LENGTH:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SW_TYPE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SW_TYPE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SW_TAPER_START");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SW_TAPER_START:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SW_TAPER_END");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SW_TAPER_END:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "TAPER_TYPE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TAPER_TYPE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "ALIAS_FILT_FREQ");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "ALIAS_FILT_FREQ:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "ALIAS_FILT_SLOPE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "ALIAS_FILE_SLOPE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "NOTCH_FILT_FREQ");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "NOTCH_FILE_FREQ:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "NOTCH_FILT_SLOPE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "NOTCH_FILT_SLOPE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "LOW_CUT_FREQ");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "LOW_CUT_FREQ:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "HIGH_CUT_FREQ");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "HIGH_CUT_FREQ:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "LOW_CUT_SLOPE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "LOW_CUT_SLOPE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "HIGH_CUT_SLOPE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "HIGH_CUT_SLOPE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "YEAR");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "YEAR:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "DAY");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "DAY:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "HOUR");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "HOUR:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "MINUTE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "MINUTE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SECOND");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SECOND:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "TIME_BASIS_CODE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TIME_BASIS_CODE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "TRACE_WEIGHT");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TRACE_WEIGHT:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "GROUP_NUM_ROLL");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "GROUP_NUM_ROLL:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "GROUP_NUM_LAST");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "GROUP_NUM_LAST:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "GAP_SIZE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "GAP_SIZE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "OVER_TRAVEL");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "OVER_TRAVEL:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "CDP_X");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "CDP_X:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "CDP_Y");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "CDP_Y:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "INLINE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "INLINE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "XLINE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "XLINE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SP_NUM");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SP_NUM:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SP_NUM_SCALAR");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SP_NUM_SCALAR:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "TR_VAL_UNIT");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TR_VAL_UNIT:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "TRANS_CONST_MANT");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TRANS_CONST_MANT:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "TRANS_CONST_EXP");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TRANS_CONST_EXP:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "TRANS_UNITS");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TRANS_UNITS:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "DEVICE_ID");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "DEVICE_ID:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "TIME_SCALAR");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "TIME_SCALAR:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SOURCE_TYPE");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOURCE_TYPE:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SOU_V_DIR");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_V_DIR:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SOU_X_DIR");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_X_DIR:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SOU_I_DIR");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_I_DIR:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SOU_MEAS_MANT");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_MEAS_MANT:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SOU_MEAS_EXP");
        if (!p_val)
                goto error;
        printf("%20s%10lld\n", "SOU_MEAS_EXP:", *p_val);
        p_val = seis_trace_header_get_int(hdr, "SOU_MEAS_UNIT");
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
