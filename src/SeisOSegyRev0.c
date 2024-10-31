#include "SeisOSegyRev0.h"
#include "SeisCommonSegy.h"
#include "SeisOSegy.h"
#include "TRY.h"
#include <stdlib.h>

struct SeisOSegyRev0 {
    SeisOSegy* osgy;
    int rc;
};

SeisOSegyRev0* seis_osegy_rev0_new()
{
    SeisOSegyRev0* sgy = (SeisOSegyRev0*)malloc(sizeof(struct SeisOSegyRev0));
    if (!sgy)
        return NULL;
    sgy->osgy = seis_osegy_new();
    sgy->rc = 1;
    return sgy;
}

SeisOSegyRev0* seis_osegy_rev0_ref(SeisOSegyRev0* sgy)
{
    ++sgy->rc;
    return sgy;
}

void seis_osegy_rev0_unref(SeisOSegyRev0* sgy)
{
    if (!--sgy->rc) {
        seis_osegy_unref(sgy->osgy);
        free(sgy);
    }
}

SeisSegyErr const* seis_osegy_rev0_get_error(SeisOSegyRev0 const* sgy)
{
    return seis_osegy_get_error(sgy->osgy);
}

SeisSegyErrCode seis_osegy_rev0_open(SeisOSegyRev0* sgy, char const* file_name)
{
    return seis_osegy_open(sgy->osgy, file_name);
}

SeisSegyErrCode seis_osegy_rev0_write_trace(SeisOSegyRev0* sgy,
    SeisTrace const* trc)
{
    SeisSegyErr const* err = seis_osegy_get_error(sgy->osgy);
    TRY(seis_osegy_write_trace_header(sgy->osgy,
        seis_trace_get_header_const(trc)));
    return seis_osegy_write_trace_samples_fix(sgy->osgy, trc);
error:
    return err->code;
}

void seis_osegy_rev0_set_text_header(SeisOSegyRev0* sgy, char const* hdr)
{
    seis_osegy_set_text_header(sgy->osgy, hdr);
}

void seis_osegy_rev0_set_binary_header(SeisOSegyRev0* sgy,
    SeisSegyBinHdr const* bh)
{
    seis_osegy_set_binary_header(sgy->osgy, bh);
}
