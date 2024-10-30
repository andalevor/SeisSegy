#include "SeisCommonSegy.h"
#include "m-array.h"
#include "m-string.h"
#include "stdlib.h"
#include <assert.h>
#include <string.h>

ARRAY_DEF(str_arr, string_t)

typedef struct SeisCommonSegyPrivate {
  struct SeisCommonSegy com;
  str_arr_t text_hdrs, end_stanzas;
} SeisCommonSegyPrivate;

SeisCommonSegy *seis_common_segy_new() {
  SeisCommonSegyPrivate *priv =
      (SeisCommonSegyPrivate *)malloc(sizeof(struct SeisCommonSegyPrivate));
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
  return (SeisCommonSegy *)priv;
}

void seis_common_segy_unref(SeisCommonSegy *sgy) {
  SeisCommonSegyPrivate *psgy = (SeisCommonSegyPrivate *)sgy;
  free(psgy->com.hdr_buf);
  if (psgy->com.samp_buf)
    free(psgy->com.samp_buf);
  if (psgy->com.file)
    fclose(psgy->com.file);
  str_arr_clear(psgy->text_hdrs);
  str_arr_clear(psgy->end_stanzas);
  free(psgy);
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
