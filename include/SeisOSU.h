/**
 * \file SeisOSU.h
 * \brief Some common write functions for seismic unix writing.
 * \author andalevor
 * \date 2023\03\06
 */

#ifndef SEIS_OSU_H
#define SEIS_OSU_H

#include "SeisCommonSegy.h"
#include <SeisTrace.h>

/**
 * \struct SeisOSU
 * \brief Common parts for seismic unix output.
 */
typedef struct SeisOSU SeisOSU;

/**
 * \fn seis_osu_new
 * \brief Initiates SeisOSU instance.
 * \return Initiated SeisOSU or NULL.
 */
SeisOSU *seis_osu_new(void);

/**
 * \fn seis_osu_ref
 * \brief Increments reference counter
 * \param pointer to SeisOSU instance.
 * \return pointer to SeisOSU
 */
SeisOSU *seis_osu_ref(SeisOSU *su);

/**
 * \fn seis_osu_unref
 * \brief free memory and writes trailer stanzas
 * \param sgy pointer to SeisOSU instance.
 */
void seis_osu_unref(SeisOSU **su);

/**
 * \fn seis_osu_get_error
 * \brief gets pointer to SeisSegyErr to check errors later
 * \param sgy pointer to SeisOSU instance.
 * \return error code to check
 */
SeisSegyErr const *seis_osu_get_error(SeisOSU const *su);

/**
 * \fn seis_osu_open
 * \brief opens file for writing.
 * \param sgy pointer to SeisOSU instance.
 * \param file_name name of the file to open
 * \return error code to check
 */
SeisSegyErrCode seis_osu_open(SeisOSU *su, char const *file_name);

/**
 * \fn seis_osu_write_trace
 * \brief Writes given trace to the file.
 * \param sgy pointer to SeisOSU instance.
 * \param trc Pointer to trace structure to write.
 * \return error code to check
 */
SeisSegyErrCode seis_osu_write_trace(SeisOSU *su, SeisTrace const *trc);

/**
 * \fn seis_osu_remap_trace_header
 * \brief changes header reading parameters
 * offset + size of format should not exceed 240
 * \param sgy SeisOSU instance
 * \param hdr_name Name of header to remap
 * \param hdr_num Number of header to remap. Main header is 1, additional is 2..
 * \param offset Byte offset inside header. Should be in range 1-240.
 * \param fmt Format of header to write.
 * \return Error code.
 */
SeisSegyErrCode seis_osu_remap_trace_header(SeisOSU *su, char const *hdr_name,
                                            int offset, enum FORMAT fmt);

#endif /* SEIS_OSU_H */
