/**
 * \file SeisISegy.h
 * \brief Main type and functions for SEGY manipulation.
 * \author andalevor
 * \date 2022\11\20
 */

#ifndef SEIS_ISEGY_H
#define SEIS_ISEGY_H

#include "SeisCommonSegy.h"
#include <SeisTrace.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * \struct SeisISegy
 * \brief Main type for SEGY manipulation.
 */
typedef struct SeisISegy SeisISegy;

/**
 * \fn seis_isegy_new
 * \brief Initiates SeisISegy instance.
 * \return NULLable.
 */
SeisISegy *seis_isegy_new(void);

/**
 * \fn seis_isegy_ref
 * \brief Makes rc increment.
 * \param sgy Pointer to SeisISegy object.
 * \return nonNULL. Pointer to SeisISegy object.
 */
SeisISegy *seis_isegy_ref(SeisISegy *sgy);

/**
 * \fn seis_isegy_unref
 * \brief Frees memory.
 * \param sgy Pointer to SeisISegy object.
 */
void seis_isegy_unref(SeisISegy **sgy);

/**
 * \fn seis_isegy_get_error
 * \brief Gets SeisSegyErr structure for error checking.
 * \return nonNULL. You should not free this memory.
 */
SeisSegyErr const *seis_isegy_get_error(SeisISegy const *sgy);

/**
 * \fn seis_isegy_open
 * \brief Opens file, loads headers, prepares for trace reading.
 * \param sgy SeisISegy instance.
 * \param file_name Name of file to open.
 * \return Error code.
 */
SeisSegyErrCode seis_isegy_open(SeisISegy *sgy, char const *file_name);

/**
 * \fn seis_isegy_read_trace
 * \brief Reads current trace from file.
 * \param sgy SeisISegy instance.
 * \return NULLable. You should free this memory.
 */
SeisTrace *seis_isegy_read_trace(SeisISegy *sgy);

/**
 * \fn seis_isegy_read_trace_header
 * \brief Reads current trace header from file.
 * \param sgy SeisISegy instance.
 * \return NULLable. You should free this memory.
 */
SeisTraceHeader *seis_isegy_read_trace_header(SeisISegy *sgy);

/**
 * \fn seis_isegy_get_text_headers_num
 * \brief gets number of text headers in SEGY
 * \param sgy SeisISegy instance.
 * return number of text headers in file.
 */
size_t seis_isegy_get_text_headers_num(SeisISegy const *sgy);

/**
 * \fn seis_isegy_get_text_header
 * \brief gets text header by its index.
 * \param sgy SeisISegy instance.
 * \param idx index of tet header. main headers index 0
 * \return nonNULL. Zero terminated header. You should not free this memory.
 */
char const *seis_isegy_get_text_header(SeisISegy const *sgy, size_t idx);

/**
 * \fn seis_isegy_get_binary_header
 * \brief Getter for struct SeisISegyBinHdr
 * \param sgy SeisISegy instance.
 * \return nonNULL. You should not free this memory.
 */
SeisSegyBinHdr const *seis_isegy_get_binary_header(SeisISegy const *sgy);

/**
 * \fn seis_isegy_end_of_data
 * \brief Checks if it is possible to read one more trace.
 * \param sgy SeisISegy instance.
 * \return true if end of file reached
 */
bool seis_isegy_end_of_data(SeisISegy const *sgy);

/**
 * \fn seis_isegy_rewind
 * \brief Lets start reading data from first trace.
 * \param sgy SeisISegy instance.
 */
void seis_isegy_rewind(SeisISegy *sgy);

/**
 * \fn seis_isegy_remap_trace_header
 * \brief changes header reading parameters
 * offset + size of format should not exceed 240
 * \param sgy SeisISegy instance
 * \param hdr_name Name of header to remap
 * \param hdr_num Number of header to remap. Main header is 1, additional is 2..
 * \param offset Byte offset inside header. Should be in range 1-240.
 * \param fmt Format of header to read.
 * \return Error code.
 */
SeisSegyErrCode seis_isegy_remap_trace_header(SeisISegy *sgy,
                                              char const *hdr_name, int hdr_num,
                                              int offset, enum FORMAT fmt);

/**
 * \fn seis_isegy_get_offset
 * \brief gets current file offset to come back later and read the same trace
 * again
 * \param sgy SeisISegy instance
 * \return file offset
 */
size_t seis_isegy_get_offset(SeisISegy *sgy);

/**
 * \fn seis_isegy_set_offset
 * \brief sets file offset to read trace
 * \param sgy SeisISegy instance
 * \param offset file offset
 */
void seis_isegy_set_offset(SeisISegy *sgy, size_t offset);

#endif /* SEIS_ISEGY_H */
