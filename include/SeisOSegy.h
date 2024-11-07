/**
 * \file SeisOSegy.h
 * \brief Some common write data gunctions.
 * \author andalevor
 * \date 2023\03\06
 */

#ifndef SEIS_OSEGY_H
#define SEIS_OSEGY_H

#include "SeisCommonSegy.h"
#include <SeisTrace.h>

/**
 * \struct SeisOSegy
 * \brief Common parts for SEGY output.
 */
typedef struct SeisOSegy SeisOSegy;

/**
 * \fn seis_osegy_new
 * \brief Initiates SeisOSegy instance.
 * \return Initiated SeisOSegy or NULL.
 */
SeisOSegy *seis_osegy_new(void);

/**
 * \fn seis_osegy_ref
 * \brief Increments reference counter
 * \param pointer to SeisOSegy instance.
 * \return pointer to SeisOSegy
 */
SeisOSegy *seis_osegy_ref(SeisOSegy *sgy);

/**
 * \fn seis_osegy_unref
 * \brief free memory and writes trailer stanzas
 * \param sgy pointer to SeisOSegy instance.
 */
void seis_osegy_unref(SeisOSegy **sgy);

/**
 * \fn seis_osegy_get_error
 * \brief gets pointer to SeisSegyErr to check errors later
 * \param sgy pointer to SeisOSegy instance.
 * \return error code to check
 */
SeisSegyErr const *seis_osegy_get_error(SeisOSegy const *sgy);

/**
 * \fn seis_osegy_set_binary_header
 * \brief sets binary header. Should be called before open function
 * \param sgy to SeisOSegy instance.
 * \param bh will be copied by value
 */
void seis_osegy_set_binary_header(SeisOSegy *sgy, SeisSegyBinHdr const *bh);

/**
 * \fn seis_osegy_set_text_header
 * \brief sets main SEGY text header. Should be called before open function
 * \param sgy pointer to SeisOSegy instance
 * \param hdr text header. must have 3200 chars
 */
void seis_osegy_set_text_header(SeisOSegy *sgy, char const *hdr);

/**
 * \fn seis_osegy_open
 * \brief opens file for writing. Writes text and binary headers
 * \param sgy pointer to SeisOSegy instance.
 * \param file_name name of the file to open
 * \return error code to check
 */
SeisSegyErrCode seis_osegy_open(SeisOSegy *sgy, char const *file_name);

/**
 * \fn seis_osegy_add_ext_text_header
 * \brief adds additional SEGY text header
 * \param sgy pointer to SeisOSegy instance
 * \param hdr text header. must have 3200 chars
 */
void seis_osegy_add_ext_text_header(SeisOSegy *sgy, char *hdr);

/**
 * \fn seis_osegy_add_traler_stanza
 * \brief adds SEGY trailer stanza
 * \param sgy pointer to SeisOSegy instance
 * \param hdr text header. must have 3200 chars
 */
void seis_osegy_add_traler_stanza(SeisOSegy *sgy, char *buf);

/**
 * \fn seis_osegy_write_trace
 * \brief Writes given trace to the file.
 * \param sgy pointer to SeisOSegy instance.
 * \param trc Pointer to trace structure to write.
 * \return error code to check
 */
SeisSegyErrCode seis_osegy_write_trace(SeisOSegy *sgy, SeisTrace const *trc);

/**
 * \fn seis_osegy_remap_trace_header
 * \brief changes header reading parameters
 * offset + size of format should not exceed 240
 * \param sgy SeisOSegy instance
 * \param hdr_name Name of header to remap
 * \param hdr_num Number of header to remap. Main header is 1, additional is 2..
 * \param offset Byte offset inside header. Should be in range 1-240.
 * \param fmt Format of header to write.
 * \return Error code.
 */
SeisSegyErrCode seis_osegy_remap_trace_header(SeisOSegy *sgy,
                                              char const *hdr_name, int hdr_num,
                                              int offset, enum FORMAT fmt);

#endif /* SEIS_OSEGY_H */
