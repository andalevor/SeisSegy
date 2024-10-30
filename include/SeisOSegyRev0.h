/**
 * \file SeisOSegyRev0.h
 * \brief Create SEGY revision 0 file.
 * \author andalevor
 * \date 2023\03\29
 */

#ifndef SEIS_OSEGY_REV0_H
#define SEIS_OSEGY_REV0_H

#include "SeisCommonSegy.h"
#include <SeisTrace.h>

typedef struct SeisOSegyRev0 SeisOSegyRev0;

/**
 * \fn seis_osegy_rev0_new
 * \brief Initiates SeisOSegyRev0 instance.
 * \return Initiated SeisOSegyRev0 or NULL.
 */
SeisOSegyRev0 *seis_osegy_rev0_new();

/**
 * \fn seis_osegy_rev0_ref
 * \brief Increments reference counter
 * \param pointer to SeisOSegyRev0 instance.
 * \return pointer to SeisOSegy
 */
SeisOSegyRev0 *seis_osegy_rev0_ref(SeisOSegyRev0 *sgy);

/**
 * \fn seis_osegy_rev0_unref
 * \brief free memory.
 * \param pointer to SeisOSegyRev0 instance.
 */
void seis_osegy_rev0_unref(SeisOSegyRev0 *sgy);

/**
 * \fn seis_osegy_rev0_get_error
 * \brief gets pointer to SeisSegyErr to check errors later
 * \param pointer to SeisOSegyRev0 instance.
 * \return error code to check
 */
SeisSegyErr const *seis_osegy_rev0_get_error(SeisOSegyRev0 const *sgy);

/**
 * \fn seis_osegy_rev0_open
 * \brief Opens file to write SEGY data
 * \param pointer to SeisOSegyRev0 instance.
 * \param file_name Files name to open
 * \return error code to check
 */
SeisSegyErrCode seis_osegy_rev0_open(SeisOSegyRev0 *sgy, char const* file_name);

/**
 * \fn seis_osegy_rev0_write_trace
 * \brief writes trace
 * \param pointer to SeisOSegyRev0 instance.
 * \param trc seismic trace
 * \return error code to check
 */
SeisSegyErrCode seis_osegy_rev0_write_trace(SeisOSegyRev0 *sgy,
										   	SeisTrace const *trc);

/**
 * \fn seis_osegy_rev0_set_text_header
 * \brief sets main SEGY text header
 * \param sgy pointer to SeisOSegyRev0 instance
 * \param hdr text header. must have 3200 chars
 */
void seis_osegy_rev0_set_text_header(SeisOSegyRev0 *sgy, char const *hdr);

/**
 * \fn seis_osegy_rev0_set_binary_header
 * \brief sets binary header. Should be called before open function
 * \param sgy to SeisOSegyRev0 instance.
 * \param bh will be copied by value
 */
void seis_osegy_rev0_set_binary_header(SeisOSegyRev0 *sgy,
									   SeisSegyBinHdr const *bh);
#endif /* SEIS_OSEGY_REV0_H  */
