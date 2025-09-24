/**
 * \file SeisCommonSegy.h
 * \brief Type and functions for common SEGY parts that could be shared.
 * \author andalevor
 * \date 2023\01\31
 */

#ifndef SEIS_COMMON_SEGY_H
#define SEIS_COMMON_SEGY_H

#include <SeisTrace.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define SEIS_SEGY_TEXT_HEADER_SIZE 3200
#define SEIS_SEGY_BIN_HEADER_SIZE 400
#define SEIS_SEGY_TRACE_HEADER_SIZE 240

enum FORMAT { i8, u8, i16, u16, i32, u32, i64, u64, f32, f64, b64 };

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
} SeisSegyBinHdr;

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
        SEIS_SEGY_ERR_FILE_WRITE,
        SEIS_SEGY_ERR_BAD_PARAMS,
} SeisSegyErrCode;

/**
 * \struct SeisSegyErr
 * \brief Type for SEGY manipulations error checking.
 */
typedef struct SeisSegyErr {
        SeisSegyErrCode code;
        char *message;
} SeisSegyErr;

/**
 * \struct SeisCommonSegy
 * \brief SEGY common parts.
 */
typedef struct SeisCommonSegy {
        struct SeisSegyBinHdr bin_hdr;
        struct SeisSegyErr err;
        FILE *file;
        char *samp_buf, *hdr_buf;
        int bytes_per_sample;
        long samp_per_tr;
} SeisCommonSegy;

/**
 * \fn seis_common_segy_new
 * \brief Initiates SeisCommonSegy instance.
 * \return Initiated SeisCommonSegy or NULL.
 */
SeisCommonSegy *seis_common_segy_new(void);

/**
 * \fn seis_common_segy_unref
 * \brief Frees memory.
 * \param com Pointer to SeisCommonSegy object.
 */
void seis_common_segy_unref(SeisCommonSegy **com);

/**
 * \fn seis_common_segy_set_text_header
 * \brief sets text header by its index
 * \param com Pointer to SeisCommonSegy object.
 * \param idx index of header. must be less than number of headers
 * \param hdr text header. must have 3200 chars length
 */
void seis_common_segy_set_text_header(SeisCommonSegy *com, size_t idx,
                                      char const *hdr);

/**
 * \fn seis_common_add_text_header
 * \brief adds SEGY text header to internal storage.
 * \param com Pointer to SeisCommonSegy object.
 * \param com Pointer to buffer with header.
 */
void seis_common_segy_add_text_header(SeisCommonSegy *com, char const *buf);

/**
 * \fn seis_common_add_stanza
 * \brief adds SEGY end stanza to internal storage.
 * \param com Pointer to SeisCommonSegy object.
 * \param com Pointer to buffer with stanza.
 */
void seis_common_segy_add_stanza(SeisCommonSegy *com, char const *buf);

/**
 * \fn seis_common_get_text_headers_num
 * \brief gets number of SEGY text headers.
 * \param com Pointer to SeisCommonSegy object.
 * \return number of text headers.
 */
size_t seis_common_segy_get_text_headers_num(SeisCommonSegy const *com);

/**
 * \fn seis_common_get_text_header
 * \brief Gets text header by its index in storage. Main header at 0.
 * \param com Pointer to SeisCommonSegy object.
 * \prarm idx Index of desired text header from 0 to text_headers_num - 1.
 * \return Pointer to header. Should not be freed.
 */
char const *seis_common_segy_get_text_header(SeisCommonSegy const *com,
                                             size_t idx);

/**
 * \fn seis_common_get_stanzas_num
 * \brief gets number of SEGY end stanzas.
 * \param com Pointer to SeisCommonSegy object.
 * \return number of stanzas.
 */
size_t seis_common_segy_get_stanzas_num(SeisCommonSegy const *com);

/**
 * \fn seis_common_get_stanza
 * \brief Gets end stanza by its index in storage. Main header at 0.
 * \param com Pointer to SeisCommonSegy object.
 * \prarm idx Index of desired end stanza from 0 to end_stanzas_num - 1.
 * \return Pointer to stanza. Should not be freed.
 */
char const *seis_common_segy_get_stanza(SeisCommonSegy const *com, size_t idx);

/**
 * \brief Text header from SEGY revision 0 standard
 */
extern char const *seis_segy_default_text_header_rev0;

/**
 * \brief Text header from SEGY revision 1 standard
 */
extern char const *seis_segy_default_text_header_rev1;

/**
 * \brief Text header from SEGY revision 2 standard
 */
extern char const *seis_segy_default_text_header_rev2;

/**
 * \fn seis_common_segy_remap_trace_header
 * \brief changes header reading parameters
 * offset + size of format should not exceed 240
 * \param sgy SeisISegy instance
 * \param hdr_name Name of header to remap
 * \param hdr_num Number of header to remap. Main header is 1, additional is 2..
 * \param offset Byte offset inside header. Should be in range 1-240.
 * \param fmt Format of header to read.
 * \return Error code.
 */
SeisSegyErrCode seis_common_remap_trace_header(SeisCommonSegy *sgy,
                                               char const *hdr_name,
                                               int hdr_num, int offset,
                                               enum FORMAT fmt);

#endif /* SEIS_COMMON_SEGY_H */
