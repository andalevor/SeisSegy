/**
 * \file SeisISegy.h
 * \brief Main type and functions for SEGY manipulation.
 * \author andalevor
 * \date 2022\11\20
 */

#ifndef SEIS_SEGY_H
#define SEIS_SEGY_H

#include <stdbool.h>
#include <SeisTrace.h>
#include <stddef.h>
#include "SeisCommonSegy.h"

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
SeisISegy *seis_isegy_new();

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
void seis_isegy_unref(SeisISegy *sgy);

/**
 * \fn seis_isegy_get_error
 * \brief Gets SeisISegyErr structure for error checking.
 * \return nonNULL. You should not free this memory.
 */
SeisSegyErr *seis_isegy_get_error(SeisISegy *sgy);

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
 * SeisISegy should be created in 'r' mode.
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
size_t seis_isegy_get_text_headers_num(SeisISegy *sgy);

/**
 * \fn seis_isegy_get_text_header
 * \brief gets text header by its index.
 * \param sgy SeisISegy instance.
 * \param idx index of tet header. main headers index 0
 * \return nonNULL. Zero terminated header. You should not free this memory.
 */
char const*seis_isegy_get_text_header(SeisISegy *sgy, size_t idx);

/**
 * \fn seis_isegy_get_binary_header
 * \brief Getter for struct SeisISegyBinHdr
 * \param sgy SeisISegy instance.
 * \return nonNULL. You should not free this memory.
 */
SeisSegyBinHdr *seis_isegy_get_binary_header(SeisISegy *sgy);

/**
 * \fn seis_isegy_end_of_data
 * \brief Checks if it is possible to read one more trace.
 * \return true if end of file reached
 */
bool seis_isegy_end_of_data(SeisISegy *sgy);

#endif /* SEIS_SEGY_H */
