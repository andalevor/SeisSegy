/**
 * \file SeisISU.h
 * \brief Main type and functions for seismic unix files reading.
 * \author andalevor
 * \date 2024\11\06
 */

#ifndef SEIS_ISU_H
#define SEIS_ISU_H

#include "SeisCommonSegy.h"
#include <SeisTrace.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * \struct SeisISU
 * \brief Main type for seismic unix files reading.
 */
typedef struct SeisISU SeisISU;

/**
 * \fn seis_isu_new
 * \brief Initiates SeisISU instance.
 * \return NULLable.
 */
SeisISU *seis_isu_new(void);

/**
 * \fn seis_isu_ref
 * \brief Makes rc increment.
 * \param sgy Pointer to SeisISU object.
 * \return nonNULL. Pointer to SeisISU object.
 */
SeisISU *seis_isu_ref(SeisISU *su);

/**
 * \fn seis_isu_unref
 * \brief Decriments rc. Frees memory.
 * \param sgy Pointer to SeisISU object.
 */
void seis_isu_unref(SeisISU **su);

/**
 * \fn seis_isu_get_error
 * \brief Gets SeisSegyErr structure for error checking.
 * \return nonNULL. You should not free this memory.
 */
SeisSegyErr const *seis_isu_get_error(SeisISU const *su);

/**
 * \fn seis_isu_open
 * \brief Opens file, prepares for trace reading.
 * \param sgy SeisISU instance.
 * \param file_name Name of file to open.
 * \return Error code.
 */
SeisSegyErrCode seis_isu_open(SeisISU *su, char const *file_name);

/**
 * \fn seis_isu_read_trace
 * \brief Reads current trace from file.
 * \param sgy SeisISU instance.
 * \return NULLable. You should free this memory.
 */
SeisTrace *seis_isu_read_trace(SeisISU *su);

/**
 * \fn seis_isu_read_trace_header
 * \brief Reads current trace header from file.
 * \param sgy SeisISU instance.
 * \return NULLable. You should free this memory.
 */
SeisTraceHeader *seis_isu_read_trace_header(SeisISU *su);

/**
 * \fn seis_isu_end_of_data
 * \brief Checks if it is possible to read one more trace.
 * \param sgy SeisISU instance.
 * \return true if end of file reached
 */
bool seis_isu_end_of_data(SeisISU const *su);

/**
 * \fn seis_isu_rewind
 * \brief Lets start reading data from first trace.
 * \param sgy SeisISU instance.
 */
void seis_isu_rewind(SeisISU *su);

/**
 * \fn seis_isu_remap_trace_header
 * \brief changes header reading parameters
 * offset + size of format should not exceed 240
 * \param sgy SeisISU instance
 * \param hdr_name Name of header to remap
 * \param offset Byte offset inside header. Should be in range 1-240.
 * \param fmt Format of header to read.
 * \return Error code.
 */
SeisSegyErrCode seis_isu_remap_trace_header(SeisISU *su, char const *hdr_name,
                                            int offset, enum FORMAT fmt);

#endif /* SEIS_ISU_H */
