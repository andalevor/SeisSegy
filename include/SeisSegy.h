/**
 * \file SeisSegy.h
 * \brief Main type and functions for SEGY manipulation.
 * \author andalevor
 * \date 2022\11\20
 */

#ifndef SEIS_SEGY_H
#define SEIS_SEGY_H

#include <stdbool.h>
#include <stdint.h>
#include <SeisTrace.h>

/**
 * \struct SeisSegyBinHdr
 * \brief SEGY binary header struct
 */
typedef struct SeisSegyBinHdr {
	int32_t job_id;
	int32_t line_num;
	int32_t reel_num;
	int16_t tr_per_ens;
	int16_t aux_per_ens;
	int16_t samp_int;
	int16_t samp_int_orig;
	int16_t samp_per_tr;
	int16_t samp_per_tr_orig;
	int16_t format_code;
	int16_t ens_fold;
	int16_t sort_code;
	int16_t vert_sum_code;
	int16_t sw_freq_at_start;
	int16_t sw_freq_at_end;
	int16_t sw_length;
	int16_t sw_type_code;
	int16_t sw_ch_tr_num;
	int16_t taper_at_start;
	int16_t taper_at_end;
	int16_t taper_type;
	int16_t corr_traces;
	int16_t bin_gain_recov;
	int16_t amp_recov_meth;
	int16_t measure_system;
	int16_t impulse_sig_pol;
	int16_t vib_pol_code;
	int32_t ext_tr_per_ens;
	int32_t ext_aux_per_ens;
	int32_t ext_samp_per_tr;
	double ext_samp_int;
	double ext_samp_int_orig;
	int32_t ext_samp_per_tr_orig;
	int32_t ext_ens_fold;
	int32_t endianness;
	uint8_t SEGY_rev_major_ver;
	uint8_t SEGY_rev_minor_ver;
	int16_t fixed_tr_length;
	int16_t ext_text_headers_num;
	int32_t max_num_add_tr_headers;
	int16_t time_basis_code;
	uint64_t num_of_tr_in_file;
	uint64_t byte_off_of_first_tr;
	int32_t num_of_trailer_stanza;
} *SeisSegyBinHdr_t;

/**
 * \struct SeisSegy
 * \brief Main type for SEGY manipulation.
 */
typedef struct SeisSegy *SeisSegy_t;

/**
 * \enum SeisSegyErrCode
 * \brief Enumeration for error codes.
 */
typedef enum SeisSegyErrCode {
	SEIS_SEGY_ERR_OK,
	SEIS_SEGY_ERR_FILE_OPEN,
	SEIS_SEGY_ERR_FILE_READ,
	SEIS_SEGY_ERR_NO_MEM,
	SEIS_SEGY_ERR_UNKNOWN_ENDIANNESS,
	SEIS_SEGY_ERR_UNSUPPORTED_FORMAT,
	SEIS_SEGY_ERR_BROKEN_FILE,
} SeisSegyErrCode;

/**
 * \struct SeisSegyErr
 * \brief Type for SEGY manipulations error checking.
 */
typedef struct SeisSegyErr {
	SeisSegyErrCode code;
	char *message;
} *SeisSegyErr_t;

/**
 * \fn seis_segy_new
 * \brief Initiates SeisSegy instance.
 * \param mode 'r' or 'w'
 * \return Initiated SeisSegy or NULL.
 */
SeisSegy_t seis_segy_new(char mode);

/**
 * \fn seis_segy_ref
 * \brief Makes rc increment.
 * \param t Pointer to SeisSegy object.
 * \return Pointer to SeisSegy object.
 */
SeisSegy_t seis_segy_ref(SeisSegy_t sgy);

/**
 * \fn seis_segy_unref
 * \brief Frees memory.
 * \param t Pointer to SeisSegy object.
 */
void seis_segy_unref(SeisSegy_t sgy);

/**
 * \fn seis_segy_get_error
 * \brief Gets SeisSegyErr structure for error checking.
 * You should not free this memory.
 * \return SeisSegyErr_t
 */
SeisSegyErr_t seis_segy_get_error(SeisSegy_t sgy);

/**
 * \fn seis_segy_open
 * \brief Opens file, loads headers, prepares for trace reading.
 * \param sgy SeisSegy instance.
 * \param file_name Name of file to open.
 * \return Error code.
 */
SeisSegyErrCode seis_segy_open(SeisSegy_t sgy, char const *file_name);

/**
 * \fn seis_segy_read_trace
 * \brief Reads current trace from file.
 * SeisSegy should be created in 'r' mode.
 * \param sgy SeisSegy instance.
 */
SeisTrace_t seis_segy_read_trace(SeisSegy_t sgy);

/**
 * \fn seis_segy_read_trace_header
 * \brief Reads current trace header from file.
 * SeisSegy should be created in 'r' mode.
 * \param sgy SeisSegy instance.
 */
SeisTrace_t seis_segy_read_trace_header(SeisSegy_t sgy);

/**
 * \fn seis_segy_get_binary_header
 * \brief Getter for struct SeisSegyBinHdr
 * \param sgy SeisSegy instance.
 */
SeisSegyBinHdr_t seis_segy_get_binary_header(SeisSegy_t sgy);

/**
 * \fn seis_segy_end_of_data
 * \brief Checks if it is possible to read one more trace.
 */
bool seis_segy_end_of_data(SeisSegy_t sgy);

#endif /* SEIS_SEGY_H */
