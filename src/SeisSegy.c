#include <SeisTrace.h>
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "SeisSegy.h"
#include "m-array.h"
#include "m-string.h"

ARRAY_DEF(str_arr, string_t)

#define TEXT_HEADER_SIZE 3200
#define BIN_HEADER_SIZE 400
#define TRACE_HEADER_SIZE 240
#define TRY(x) do { if (x) goto error; } while (0)

struct SeisSegy {
	str_arr_t text_hdrs, end_stanzas;
	struct SeisSegyBinHdr bin_hdr;
	FILE *file;
	char mode, *trc_hdr_buf, *samp_buf, *hdr_buf;
	long curr_pos, first_trace_pos, end_of_data, samp_per_tr;
	int bytes_per_sample;
	int8_t (*read_i8)(char const **buf);
	uint8_t (*read_u8)(char const **buf);
	int16_t (*read_i16)(char const **buf);
	uint16_t (*read_u16)(char const **buf);
	int32_t (*read_i24)(char const **buf);
	uint32_t (*read_u24)(char const **buf);
	int32_t (*read_i32)(char const **buf);
	uint32_t (*read_u32)(char const **buf);
	int64_t (*read_i64)(char const **buf);
	uint64_t (*read_u64)(char const **buf);
	double (*dbl_from_IEEE_float)(SeisSegy_t sgy, char const **buf);
	double (*dbl_from_IEEE_double)(SeisSegy_t sgy, char const **buf);
	double (*read_sample)(SeisSegy_t sgy, char const **buf);
	SeisSegyErrCode (*read_trc_smpls)(SeisSegy_t sgy, SeisTraceHeader_t hdr,
									  SeisTrace_t *trc);
	struct SeisSegyErr err;
	int rc;
};

static SeisSegyErrCode fill_from_file(SeisSegy_t sgy, char *buf, size_t num);
static void 		   file_skip_bytes(SeisSegy_t sgy, size_t num);
static SeisSegyErrCode open_file(SeisSegy_t sgy, char const *file_name);
static SeisSegyErrCode read_text_header(SeisSegy_t sgy, str_arr_t arr, int num);
static SeisSegyErrCode read_bin_header(SeisSegy_t sgy);
static SeisSegyErrCode assign_raw_readers(SeisSegy_t sgy);
static SeisSegyErrCode assign_sample_reader(SeisSegy_t sgy);
static void            assign_bytes_per_sample(SeisSegy_t sgy);
static SeisSegyErrCode read_ext_text_headers(SeisSegy_t sgy);
static SeisSegyErrCode read_trailer_stanzas(SeisSegy_t sgy);
static SeisSegyErrCode read_trc_smpls_fix(SeisSegy_t sgy, SeisTraceHeader_t hdr,
										  SeisTrace_t *trc);
static SeisSegyErrCode read_trc_smpls_var(SeisSegy_t sgy, SeisTraceHeader_t hdr,
										  SeisTrace_t *trc);
static SeisSegyErrCode read_trc_hdr(SeisSegy_t sgy, SeisTraceHeader_t hdr);

static int8_t read_i8(char const **buf);
static uint8_t read_u8(char const **buf);
static int16_t read_i16(char const **buf);
static uint16_t read_u16(char const **buf);
static int32_t read_i24(char const **buf);
static uint32_t read_u24(char const **buf);
static int32_t read_i32(char const **buf);
static uint32_t read_u32(char const **buf);
static int64_t read_i64(char const **buf);
static uint64_t read_u64(char const **buf);
static int16_t read_i16_sw(char const **buf);
static uint16_t read_u16_sw(char const **buf);
static int32_t read_i24_sw(char const **buf);
static uint32_t read_u24_sw(char const **buf);
static int32_t read_i32_sw(char const **buf);
static uint32_t read_u32_sw(char const **buf);
static int64_t read_i64_sw(char const **buf);
static uint64_t read_u64_sw(char const **buf);
static double dbl_from_IBM_float(SeisSegy_t sgy, char const **buf);
static double dbl_from_IEEE_float(SeisSegy_t sgy, char const **buf);
static double dbl_from_IEEE_double(SeisSegy_t sgy, char const **buf);
static double dbl_from_IEEE_float_native(SeisSegy_t sgy,char const **buf);
static double dbl_from_IEEE_double_native(SeisSegy_t sgy, char const **buf);
static double dbl_from_i8(SeisSegy_t sgy, char const **buf);
static double dbl_from_u8(SeisSegy_t sgy, char const **buf);
static double dbl_from_i16(SeisSegy_t sgy, char const **buf);
static double dbl_from_u16(SeisSegy_t sgy, char const **buf);
static double dbl_from_i24(SeisSegy_t sgy, char const **buf);
static double dbl_from_u24(SeisSegy_t sgy, char const **buf);
static double dbl_from_i32(SeisSegy_t sgy, char const **buf);
static double dbl_from_u32(SeisSegy_t sgy, char const **buf);
static double dbl_from_i64(SeisSegy_t sgy, char const **buf);
static double dbl_from_u64(SeisSegy_t sgy, char const **buf);

SeisSegy_t seis_segy_new(char mode)
{
	assert(mode == 'r' || mode == 'w');
	SeisSegy_t sgy = (SeisSegy_t)malloc(sizeof(struct SeisSegy));
	if (!sgy)
		goto error;
	sgy->mode = mode;
	sgy->err.code = SEIS_SEGY_ERR_OK;
	sgy->err.message = "";
	sgy->hdr_buf = (char*)malloc(TRACE_HEADER_SIZE);
	if (!sgy->hdr_buf)
		goto error;
	sgy->samp_buf = NULL;
	sgy->file = NULL;
	sgy->rc = 1;
	str_arr_init(sgy->text_hdrs);
	str_arr_init(sgy->end_stanzas);
	return sgy;
error:
	return NULL;
}

SeisSegy_t seis_segy_ref(SeisSegy_t sgy)
{
	++sgy->rc;
	return sgy;
}

void seis_segy_unref(SeisSegy_t sgy)
{
	if (--sgy->rc == 0) {
		free(sgy->hdr_buf);
		if (sgy->samp_buf)
			free(sgy->samp_buf);
		if (sgy->file)
			fclose(sgy->file);
		str_arr_clear(sgy->text_hdrs);
		str_arr_clear(sgy->end_stanzas);
		free(sgy);
	}
}

bool seis_segy_end_of_data(SeisSegy_t sgy)
{
	return sgy->end_of_data == sgy->curr_pos;
}

SeisSegyErr_t seis_segy_get_error(SeisSegy_t sgy)
{
	return &sgy->err;
}

SeisSegyErrCode seis_segy_open(SeisSegy_t sgy, char const *file_name)
{
	assert(!sgy->file);
	TRY(open_file(sgy, file_name));
	TRY(read_text_header(sgy, sgy->text_hdrs, 1));
	TRY(read_bin_header(sgy));
	TRY(assign_sample_reader(sgy));
	assign_bytes_per_sample(sgy);
	TRY(read_ext_text_headers(sgy));
	if (sgy->bin_hdr.byte_off_of_first_tr) {
		sgy->first_trace_pos = sgy->bin_hdr.byte_off_of_first_tr;
		fseek(sgy->file, sgy->first_trace_pos, SEEK_SET);
	} else {
		sgy->first_trace_pos = ftell(sgy->file);
	}
	sgy->samp_per_tr = sgy->bin_hdr.ext_samp_per_tr ?
		sgy->bin_hdr.ext_samp_per_tr : sgy->bin_hdr.samp_per_tr;
	TRY(read_trailer_stanzas(sgy));
	fseek(sgy->file, sgy->first_trace_pos, SEEK_SET);
	sgy->curr_pos = sgy->first_trace_pos;
	sgy->samp_buf = (char*)malloc(sgy->samp_per_tr * sgy->bytes_per_sample);
	if (sgy->bin_hdr.fixed_tr_length || !sgy->bin_hdr.SEGY_rev_major_ver)
		sgy->read_trc_smpls = read_trc_smpls_fix;
	else
		sgy->read_trc_smpls = read_trc_smpls_var;
error:
	return sgy->err.code;
}

SeisTrace_t seis_segy_read_trace(SeisSegy_t sgy)
{
	SeisTraceHeader_t hdr = seis_trace_header_new();
	if (!hdr)
		goto error;
	TRY(read_trc_hdr(sgy, hdr));
	SeisTrace_t trc;
	TRY(sgy->read_trc_smpls(sgy, hdr, &trc));
	return trc;
error:
	return NULL;
}

SeisSegyBinHdr_t seis_segy_get_binary_header(SeisSegy_t sgy)
{
	return &sgy->bin_hdr;
}

SeisSegyErrCode fill_from_file(SeisSegy_t sgy, char *buf, size_t num)
{
	size_t read = fread(buf, 1, num, sgy->file);
	if (read != num) {
		sgy->err.code = SEIS_SEGY_ERR_FILE_READ;
		sgy->err.message = "read less bytes than should";
	}
	sgy->curr_pos = ftell(sgy->file);
	return sgy->err.code;
}

void file_skip_bytes(SeisSegy_t sgy, size_t num)
{
	fseek(sgy->file, num, SEEK_CUR);
	sgy->curr_pos = ftell(sgy->file);
}

SeisSegyErrCode open_file(SeisSegy_t sgy, char const *file_name)
{
	switch (sgy->mode) {
		case 'r':
		sgy->file = fopen(file_name, "r");
			break;
		case 'w':
		sgy->file = fopen(file_name, "w");
			break;
	}
	if (!sgy->file) {
		sgy->err.code = SEIS_SEGY_ERR_FILE_OPEN;
		sgy->err.message = "file open error";
	}
	return sgy->err.code;
}

SeisSegyErrCode read_text_header(SeisSegy_t sgy, str_arr_t arr, int num)
{
	string_t tmp;
	char *text_buf = (char*)malloc(TEXT_HEADER_SIZE + 1);
	text_buf[TEXT_HEADER_SIZE] = '\0';
	if (!text_buf) {
		sgy->err.code = SEIS_SEGY_ERR_NO_MEM;
		sgy->err.message = "no memory for text header buf";
		goto error;
	}
	string_init(tmp);
	for (int i = 0; i < num; ++i) {
		TRY(fill_from_file(sgy, text_buf, TEXT_HEADER_SIZE));
		string_set_strn(tmp, text_buf, TEXT_HEADER_SIZE);
		str_arr_push_back(arr, tmp);
	}
	string_clear(tmp);
	free(text_buf);
	return sgy->err.code;
error:
	string_clear(tmp);
	if (text_buf)
		free(text_buf);
	return sgy->err.code;
}

SeisSegyErrCode read_bin_header(SeisSegy_t sgy)
{
	char *const bin_buf = (char*)malloc(BIN_HEADER_SIZE);
	if (!bin_buf) {
		sgy->err.code = SEIS_SEGY_ERR_NO_MEM;
		sgy->err.message = "no memory for bin header";
	}
	TRY(fill_from_file(sgy, bin_buf, BIN_HEADER_SIZE));
    memcpy(&sgy->bin_hdr.endianness, bin_buf + 96, sizeof(int32_t));
	TRY(assign_raw_readers(sgy));
	char const *ptr = bin_buf;
	sgy->bin_hdr.job_id = sgy->read_i32(&ptr);
	sgy->bin_hdr.line_num = sgy->read_i32(&ptr);
	sgy->bin_hdr.reel_num = sgy->read_i32(&ptr);
	sgy->bin_hdr.tr_per_ens = sgy->read_i16(&ptr);
	sgy->bin_hdr.aux_per_ens = sgy->read_i16(&ptr);
	sgy->bin_hdr.samp_int = sgy->read_i16(&ptr);
	sgy->bin_hdr.samp_int_orig = sgy->read_i16(&ptr);
	sgy->bin_hdr.samp_per_tr = sgy->read_i16(&ptr);
	sgy->bin_hdr.samp_per_tr_orig = sgy->read_i16(&ptr);
	sgy->bin_hdr.format_code = sgy->read_i16(&ptr);
	sgy->bin_hdr.ens_fold = sgy->read_i16(&ptr);
	sgy->bin_hdr.sort_code = sgy->read_i16(&ptr);
	sgy->bin_hdr.vert_sum_code = sgy->read_i16(&ptr);
	sgy->bin_hdr.sw_freq_at_start = sgy->read_i16(&ptr);
	sgy->bin_hdr.sw_freq_at_end = sgy->read_i16(&ptr);
	sgy->bin_hdr.sw_length = sgy->read_i16(&ptr);
	sgy->bin_hdr.sw_type_code = sgy->read_i16(&ptr);
	sgy->bin_hdr.sw_ch_tr_num = sgy->read_i16(&ptr);
	sgy->bin_hdr.taper_at_start = sgy->read_i16(&ptr);
	sgy->bin_hdr.taper_at_end = sgy->read_i16(&ptr);
	sgy->bin_hdr.taper_type = sgy->read_i16(&ptr);
	sgy->bin_hdr.corr_traces = sgy->read_i16(&ptr);
	sgy->bin_hdr.bin_gain_recov = sgy->read_i16(&ptr);
	sgy->bin_hdr.amp_recov_meth = sgy->read_i16(&ptr);
	sgy->bin_hdr.measure_system = sgy->read_i16(&ptr);
	sgy->bin_hdr.impulse_sig_pol = sgy->read_i16(&ptr);
	sgy->bin_hdr.vib_pol_code = sgy->read_i16(&ptr);
	sgy->bin_hdr.ext_tr_per_ens = sgy->read_i32(&ptr);
	sgy->bin_hdr.ext_aux_per_ens = sgy->read_i32(&ptr);
	sgy->bin_hdr.ext_samp_per_tr = sgy->read_i32(&ptr);
	sgy->bin_hdr.ext_samp_int = sgy->dbl_from_IEEE_double(sgy, &ptr);
	sgy->bin_hdr.ext_samp_int_orig = sgy->dbl_from_IEEE_double(sgy, &ptr);
	sgy->bin_hdr.ext_samp_per_tr_orig = sgy->read_i32(&ptr);
	sgy->bin_hdr.ext_ens_fold = sgy->read_i32(&ptr);
	ptr += 204; // skip unassigned fields and endianness
	sgy->bin_hdr.SEGY_rev_major_ver = sgy->read_u8(&ptr);
	sgy->bin_hdr.SEGY_rev_minor_ver = sgy->read_u8(&ptr);
	sgy->bin_hdr.fixed_tr_length = sgy->read_i16(&ptr);
	sgy->bin_hdr.ext_text_headers_num = sgy->read_i16(&ptr);
	sgy->bin_hdr.max_num_add_tr_headers = sgy->read_i32(&ptr);
	sgy->bin_hdr.time_basis_code = sgy->read_i16(&ptr);
	sgy->bin_hdr.num_of_tr_in_file = sgy->read_i64(&ptr);
	sgy->bin_hdr.byte_off_of_first_tr = sgy->read_u64(&ptr);
	sgy->bin_hdr.num_of_trailer_stanza = sgy->read_i32(&ptr);
error:
	free(bin_buf);
	return sgy->err.code;
}

SeisSegyErrCode assign_raw_readers(SeisSegy_t sgy)
{
	sgy->read_i8 = read_i8;
	sgy->read_u8 = read_u8;
	switch (sgy->bin_hdr.endianness) {
	case 0x01020304:
		sgy->read_i16 = read_i16;
		sgy->read_u16 = read_u16;
		sgy->read_i24 = read_i24;
		sgy->read_u24 = read_u24;
		sgy->read_i32 = read_i32;
		sgy->read_u32 = read_u32;
		sgy->read_i64 = read_i64;
		sgy->read_u64 = read_u64;
		break;
	case 0:
	case 0x04030201:
		sgy->read_i16 = read_i16_sw;
		sgy->read_u16 = read_u16_sw;
		sgy->read_i24 = read_i24_sw;
		sgy->read_u24 = read_u24_sw;
		sgy->read_i32 = read_i32_sw;
		sgy->read_u32 = read_u32_sw;
		sgy->read_i64 = read_i64_sw;
		sgy->read_u64 = read_u64_sw;
		break;
	default:
		sgy->err.code = SEIS_SEGY_ERR_UNKNOWN_ENDIANNESS;
		sgy->err.message = "unsupported endianness";
	}
	if (FLT_RADIX == 2 && DBL_MANT_DIG == 53) {
		sgy->dbl_from_IEEE_float = dbl_from_IEEE_float_native;
		sgy->dbl_from_IEEE_double = dbl_from_IEEE_double_native;
	} else {
		sgy->dbl_from_IEEE_float = dbl_from_IEEE_float;
		sgy->dbl_from_IEEE_double = dbl_from_IEEE_double;
	}
	return sgy->err.code;
}

SeisSegyErrCode assign_sample_reader(SeisSegy_t sgy)
{
    switch (sgy->bin_hdr.format_code) {
    case 1:
        sgy->read_sample = dbl_from_IBM_float;
        break;
    case 2:
        sgy->read_sample = dbl_from_i32;
        break;
    case 3:
        sgy->read_sample = dbl_from_i16;
        break;
    case 5:
        sgy->read_sample = sgy->dbl_from_IEEE_float;
        break;
    case 6:
        sgy->read_sample = sgy->dbl_from_IEEE_double;
        break;
    case 7:
        sgy->read_sample = dbl_from_i24;
        break;
    case 8:
        sgy->read_sample = dbl_from_i8;
        break;
    case 9:
        sgy->read_sample = dbl_from_i64;
        break;
    case 10:
        sgy->read_sample = dbl_from_u32;
        break;
    case 11:
        sgy->read_sample = dbl_from_u16;
        break;
    case 12:
        sgy->read_sample = dbl_from_u64;
        break;
    case 15:
        sgy->read_sample = dbl_from_u24;
        break;
    case 16:
        sgy->read_sample = dbl_from_u8;
        break;
    default:
		sgy->err.code = SEIS_SEGY_ERR_UNSUPPORTED_FORMAT;
		sgy->err.message = "unsupported format code";
    }
	return sgy->err.code;
}

void assign_bytes_per_sample(SeisSegy_t sgy)
{
    switch (sgy->bin_hdr.format_code) {
    case 1:
    case 2:
    case 4:
    case 5:
    case 10:
        sgy->bytes_per_sample = 4;
        break;
    case 3:
    case 11:
        sgy->bytes_per_sample = 2;
        break;
    case 6:
    case 9:
    case 12:
        sgy->bytes_per_sample = 8;
        break;
    case 7:
    case 15:
        sgy->bytes_per_sample = 3;
        break;
    case 8:
    case 16:
        sgy->bytes_per_sample = 1;
        break;
    }
}

SeisSegyErrCode read_ext_text_headers(SeisSegy_t sgy)
{
    int num = sgy->bin_hdr.ext_text_headers_num;
    if (!num)
		goto error;
    if (num == -1) {
        char *end_stanza = "((SEG: EndText))";
        while (1) {
			TRY(read_text_header(sgy, sgy->text_hdrs, 1));
            if (string_search_str(*str_arr_back(sgy->text_hdrs), end_stanza) !=
				STRING_FAILURE)
				goto error;
        }
    } else {
		TRY(read_text_header(sgy, sgy->text_hdrs, num));
    }
error:
	return sgy->err.code;
}

SeisSegyErrCode read_trailer_stanzas(SeisSegy_t sgy)
{
	/* skip if no trailer stanzas */
	if (sgy->bin_hdr.num_of_trailer_stanza) {
		char *text_buf = (char*)malloc(TEXT_HEADER_SIZE);
		if (!text_buf) {
			sgy->err.code = SEIS_SEGY_ERR_NO_MEM;
			sgy->err.message = "no mem for trailer stanza";
			goto error;
		}
		/* if number is unknown */
		if (sgy->bin_hdr.num_of_trailer_stanza == -1) {
			/* we need to know number of traces to skip them */
			if (!sgy->bin_hdr.num_of_tr_in_file) {
				sgy->err.code = SEIS_SEGY_ERR_BROKEN_FILE;
				sgy->err.message = "unable to determine end of trace data";
				goto error;
			}
			/* if traces has fixed length */
			if (sgy->bin_hdr.fixed_tr_length) {
				fseek(sgy->file, (sgy->bytes_per_sample * sgy->samp_per_tr +
					  TRACE_HEADER_SIZE) * sgy->bin_hdr.num_of_tr_in_file,
					  SEEK_CUR);
				sgy->end_of_data = ftell(sgy->file);
				char *end_stanza = "((SEG: EndText))";
				while (1) {
					TRY(read_text_header(sgy, sgy->end_stanzas, 1));
					if (string_search_str(*str_arr_back(sgy->end_stanzas),
										  end_stanza) != STRING_FAILURE)
						goto error;
				}
			/* if traces has variable trace length */
			} else {
				for (uint64_t i = 0; i < sgy->bin_hdr.num_of_tr_in_file; ++i) {
					TRY(fill_from_file(sgy, sgy->trc_hdr_buf,
									   TRACE_HEADER_SIZE));
					char const* ptr = sgy->trc_hdr_buf + 114;
					uint32_t trc_samp_num = sgy->read_i16(&ptr);
					if (sgy->bin_hdr.max_num_add_tr_headers) {
						TRY(fill_from_file(sgy, sgy->trc_hdr_buf,
										   TRACE_HEADER_SIZE));
						ptr = sgy->trc_hdr_buf + 136;
						trc_samp_num = sgy->read_u32(&ptr);
						ptr = sgy->trc_hdr_buf + 156;
						uint16_t add_tr_hdr_num = sgy->read_i16(&ptr);
						add_tr_hdr_num = add_tr_hdr_num ? add_tr_hdr_num :
							sgy->bin_hdr.max_num_add_tr_headers;
						fseek(sgy->file, (add_tr_hdr_num - 1) *
							  TRACE_HEADER_SIZE, SEEK_CUR);
					}
					fseek(sgy->file, trc_samp_num * sgy->bytes_per_sample,
						  SEEK_CUR);
				}
				sgy->end_of_data = ftell(sgy->file);
				char *end_stanza = "((SEG: EndText))";
				while (1) {
					TRY(read_text_header(sgy, sgy->end_stanzas, 1));
					if (string_search_str(*str_arr_back(sgy->end_stanzas),
										  end_stanza) != STRING_FAILURE)
						goto error;
				}
			}
		/* if we know number of stanzas just read them */
		} else {
			fseek(sgy->file, sgy->bin_hdr.num_of_trailer_stanza *
				  TEXT_HEADER_SIZE, SEEK_END);
			sgy->end_of_data = ftell(sgy->file);
			TRY(read_text_header(sgy, sgy->end_stanzas,
							 sgy->bin_hdr.num_of_trailer_stanza));
		}
error:
		if (text_buf)
			free(text_buf);
		return sgy->err.code;
	} else {
		fseek(sgy->file, 0, SEEK_END);
		sgy->end_of_data = ftell(sgy->file);
	}
	return sgy->err.code;
}

SeisSegyErrCode read_trc_smpls_fix(SeisSegy_t sgy, SeisTraceHeader_t hdr,
								   SeisTrace_t *trc)
{
	TRY(fill_from_file(sgy, sgy->samp_buf, sgy->samp_per_tr *
					   sgy->bytes_per_sample));
	char const* ptr = sgy->samp_buf;
	*trc = seis_trace_new_with_header(sgy->samp_per_tr, hdr);
	double *samples = seis_trace_get_samples(*trc);
	for (double *end = samples + sgy->samp_per_tr; samples != end; ++samples)
		*samples = sgy->read_sample(sgy, &ptr);
error:
	return sgy->err.code;
}

SeisSegyErrCode read_trc_smpls_var(SeisSegy_t sgy, SeisTraceHeader_t hdr,
								   SeisTrace_t *trc)
{
	long long *samp_num = seis_trace_header_get_int(hdr, "SAMP_NUM");
	assert(samp_num);
	if (sgy->samp_per_tr < *samp_num) {
		sgy->samp_per_tr = *samp_num;
		void *res = realloc(sgy->samp_buf, *samp_num * sgy->bytes_per_sample);
		if (!res) {
			sgy->err.code = SEIS_SEGY_ERR_NO_MEM;
			sgy->err.message = "can not allocate memory for "
				"trace samples reading";
			goto error;
		}
	}
	TRY(fill_from_file(sgy, sgy->samp_buf, *samp_num * sgy->bytes_per_sample));
	char const* ptr = sgy->samp_buf;
	*trc = seis_trace_new_with_header(*samp_num, hdr);
	double *samples = seis_trace_get_samples(*trc);
	for (double *end = samples + *samp_num; samples != end; ++samples)
		*samples = sgy->read_sample(sgy, &ptr);
error:
	return sgy->err.code;
}

SeisSegyErrCode read_trc_hdr(SeisSegy_t sgy, SeisTraceHeader_t hdr)
{
	TRY(fill_from_file(sgy, sgy->hdr_buf, TRACE_HEADER_SIZE));
	char const* ptr = sgy->hdr_buf;
	seis_trace_header_set_int(hdr, "TRC_SEQ_LINE", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "TRC_SEQ_SGY", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "FFID", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "CHAN", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "ESP", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "ENS_NO", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "SEQ_NO", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "TRACE_ID", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "VERT_SUM", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "HOR_SUM", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "DATA_USE", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "OFFSET", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "R_ELEV", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "S_ELEV", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "S_DEPTH", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "R_DATUM", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "S_DATUM", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "S_WATER", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "R_WATER", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "ELEV_SCALAR", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "COORD_SCALAR", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SOU_X", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "SOU_Y", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "REC_X", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "REC_Y", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "COORD_UNITS", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "WEATH_VEL", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SUBWEATH_VEL", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "S_UPHOLE", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "R_UPHOLE", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "S_STAT", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "R_STAT", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "TOT_STAT", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "LAG_A", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "LAG_B", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "DELAY_TIME", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "MUTE_START", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "MUTE_END", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SAMP_NUM", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SAMP_INT", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "GAIN_TYPE", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "GAIN_CONST", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "INIT_GAIN", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "CORRELATED", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SW_START", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SW_END", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SW_LENGTH", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SW_TYPE", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SW_TAPER_START", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SW_TAPER_END", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "TAPER_TYPE", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "ALIAS_FILT_FREQ", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "ALIAS_FILT_SLOPE", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "NOTCH_FILT_FREQ", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "NOTCH_FILT_SLOPE", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "LOW_CUT_FREQ", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "HIGH_CUT_FREQ", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "LOW_CUT_SLOPE", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "HIGH_CUT_SLOPE", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "YEAR", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "DAY", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "HOUR", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "MINUTE", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SECOND", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "TIME_BASIS_CODE", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "TRACE_WEIGHT", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "GROUP_NUM_ROLL", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "GROUP_NUM_FIRST", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "GROUP_NUM_LAST", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "GAP_SIZE", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "OVER_TRAVEL", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "CDP_X", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "CDP_Y", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "INLINE", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "XLINE", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "SP_NUM", sgy->read_i32(&ptr));
	seis_trace_header_set_int(hdr, "SP_NUM_SCALAR", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "TR_VAL_UNIT", sgy->read_i16(&ptr));
	int32_t mant = sgy->read_i32(&ptr);
	seis_trace_header_set_real(hdr, "TRANS_CONST", mant *
							  pow(10, sgy->read_i16(&ptr)));
	seis_trace_header_set_int(hdr, "TRANS_UNITS", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "DEVICE_ID", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "TIME_SCALAR", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SOURCE_TYPE", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SOU_V_DIR", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SOU_X_DIR", sgy->read_i16(&ptr));
	seis_trace_header_set_int(hdr, "SOU_I_DIR", sgy->read_i16(&ptr));
	mant = sgy->read_i32(&ptr);
	seis_trace_header_set_real(hdr, "SOURCE_MEASUREMENT", mant *
							  pow(10, sgy->read_i16(&ptr)));
	seis_trace_header_set_int(hdr, "SOU_MEAS_UNIT", sgy->read_i16(&ptr));
	if (sgy->bin_hdr.max_num_add_tr_headers) {
		TRY(fill_from_file(sgy, sgy->hdr_buf, TRACE_HEADER_SIZE));
		char const* ptr = sgy->hdr_buf;
		seis_trace_header_set_int(hdr, "TRC_SEQ_LINE", sgy->read_u64(&ptr));
		seis_trace_header_set_int(hdr, "TRC_SEQ_SGY", sgy->read_u64(&ptr));
		seis_trace_header_set_int(hdr, "FFID", sgy->read_u64(&ptr));
		seis_trace_header_set_int(hdr, "ENS_NO", sgy->read_u64(&ptr));
		seis_trace_header_set_real(hdr, "R_ELEV",
								   sgy->dbl_from_IEEE_double(sgy, &ptr));
		seis_trace_header_set_real(hdr, "R_DEPTH",
								   sgy->dbl_from_IEEE_double(sgy, &ptr));
		seis_trace_header_set_real(hdr, "S_DEPTH",
								   sgy->dbl_from_IEEE_double(sgy, &ptr));
		seis_trace_header_set_real(hdr, "R_DATUM",
								   sgy->dbl_from_IEEE_double(sgy, &ptr));
		seis_trace_header_set_real(hdr, "S_DATUM",
								   sgy->dbl_from_IEEE_double(sgy, &ptr));
		seis_trace_header_set_real(hdr, "S_WATER",
								   sgy->dbl_from_IEEE_double(sgy, &ptr));
		seis_trace_header_set_real(hdr, "R_WATER",
								   sgy->dbl_from_IEEE_double(sgy, &ptr));
		seis_trace_header_set_real(hdr, "SOU_X",
								   sgy->dbl_from_IEEE_double(sgy, &ptr));
		seis_trace_header_set_real(hdr, "SOU_Y",
								   sgy->dbl_from_IEEE_double(sgy, &ptr));
		seis_trace_header_set_real(hdr, "REC_X",
								   sgy->dbl_from_IEEE_double(sgy, &ptr));
		seis_trace_header_set_real(hdr, "REC_Y",
								   sgy->dbl_from_IEEE_double(sgy, &ptr));
		seis_trace_header_set_real(hdr, "OFFSET",
								   sgy->dbl_from_IEEE_double(sgy, &ptr));
		seis_trace_header_set_int(hdr, "SAMP_NUM", sgy->read_u32(&ptr));
		seis_trace_header_set_int(hdr, "NANOSECOND", sgy->read_i32(&ptr));
		double samp_int = sgy->dbl_from_IEEE_double(sgy, &ptr);
		if (samp_int)
			seis_trace_header_set_int(hdr, "SAMP_INT", samp_int);
		seis_trace_header_set_int(hdr, "CABLE_NUM", sgy->read_i32(&ptr));
		uint16_t add_trc_hdr_num = sgy->read_u16(&ptr);
		if (!add_trc_hdr_num)
			seis_trace_header_set_int(hdr, "ADD_TRC_HDR_NUM",
									  sgy->bin_hdr.max_num_add_tr_headers);
		else
			seis_trace_header_set_int(hdr, "ADD_TRC_HDR_NUM", add_trc_hdr_num);
		seis_trace_header_set_int(hdr, "LAST_TRC_FLAG", sgy->read_i16(&ptr));
		seis_trace_header_set_int(hdr, "CDP_X",
								  sgy->dbl_from_IEEE_double(sgy, &ptr));
		seis_trace_header_set_int(hdr, "CDP_Y",
								  sgy->dbl_from_IEEE_double(sgy, &ptr));
		add_trc_hdr_num = add_trc_hdr_num ? add_trc_hdr_num :
			sgy->bin_hdr.max_num_add_tr_headers;
		for (int32_t i = 0; i < add_trc_hdr_num - 1; ++i) {
			/* TODO: add custom additional headers reading */
			file_skip_bytes(sgy, TRACE_HEADER_SIZE);
		}
	}
error:
	return sgy->err.code;
}

int8_t read_i8(char const **buf)
{
	int8_t res;
	memcpy(&res, *buf, sizeof(int8_t));
	++*buf;
	return res;
}

uint8_t read_u8(char const **buf)
{
	uint8_t res;
	memcpy(&res, *buf, sizeof(uint8_t));
	++*buf;
	return res;
}

int16_t read_i16(char const **buf)
{
	int16_t res;
	memcpy(&res, *buf, sizeof(int16_t));
	*buf += sizeof(int16_t);
	return res;
}

uint16_t read_u16(char const **buf)
{
	uint16_t res;
	memcpy(&res, *buf, sizeof(uint16_t));
	*buf += sizeof(uint16_t);
	return res;
}

int32_t read_i24(char const **buf)
{
	int32_t res;
	memcpy(&res, *buf, sizeof(int16_t) + sizeof(int8_t));
	*buf += sizeof(int16_t) + sizeof(int8_t);
	return res;
}

uint32_t read_u24(char const **buf)
{
	uint32_t res;
	memcpy(&res, *buf, sizeof(uint16_t) + sizeof(uint8_t));
	*buf += sizeof(uint16_t) + sizeof(uint8_t);
	return res;
}

int32_t read_i32(char const **buf)
{
	int32_t res;
	memcpy(&res, *buf, sizeof(int32_t));
	*buf += sizeof(int32_t);
	return res;
}

uint32_t read_u32(char const **buf)
{
	uint32_t res;
	memcpy(&res, *buf, sizeof(uint32_t));
	*buf += sizeof(uint32_t);
	return res;
}

int64_t read_i64(char const **buf)
{
	int64_t res;
	memcpy(&res, *buf, sizeof(int64_t));
	*buf += sizeof(int64_t);
	return res;
}

uint64_t read_u64(char const **buf)
{
	uint64_t res;
	memcpy(&res, *buf, sizeof(uint64_t));
	*buf += sizeof(uint64_t);
	return res;
}

int16_t read_i16_sw(char const **buf)
{
	uint16_t res;
	memcpy(&res, *buf, sizeof(int16_t));
	*buf += sizeof(uint16_t);
	return (int16_t)((res & 0xff) << 8 | (res & 0xff00) >> 8);
}

uint16_t read_u16_sw(char const **buf)
{
	uint16_t res;
	memcpy(&res, *buf, sizeof(uint16_t));
	*buf += sizeof(uint16_t);
	return (res & 0xff) << 8 | (res & 0xff00) >> 8;
}

int32_t read_i24_sw(char const **buf)
{
	uint32_t res = 0;
	memcpy(&res, *buf, sizeof(int16_t) + sizeof(int8_t));
	*buf += sizeof(int16_t) + sizeof(int8_t);
	return (int32_t)((res & 0xff) << 24 | (res & 0xff000000) >> 24 |
					 (res & 0xff00) <<  8 | (res & 0xff0000) >> 8);
}

uint32_t read_u24_sw(char const **buf)
{
	uint32_t res = 0;
	memcpy(&res, *buf, sizeof(uint16_t) + sizeof(uint8_t));
	*buf += sizeof(uint16_t) + sizeof(uint8_t);
	return ((res & 0xff) << 24 | (res & 0xff000000) >> 24 |
			(res & 0xff00) <<  8 | (res & 0xff0000) >> 8);
}

int32_t read_i32_sw(char const **buf)
{
	uint32_t res;
	memcpy(&res, *buf, sizeof(int32_t));
	*buf += sizeof(uint32_t);
	return (int32_t)((res & 0xff) << 24 | (res & 0xff000000) >> 24 |
					 (res & 0xff00) <<  8 | (res & 0xff0000) >> 8);
}

uint32_t read_u32_sw(char const **buf)
{
	uint32_t res;
	memcpy(&res, *buf, sizeof(int32_t));
	*buf += sizeof(uint32_t);
	return (res & 0xff) << 24 | (res & 0xff000000) >> 24 |
		(res & 0xff00) << 8 | (res & 0xff0000) >> 8;
}

int64_t read_i64_sw(char const **buf)
{
	uint64_t res;
	memcpy(&res, *buf, sizeof(int64_t));
	*buf += sizeof(uint64_t);
	return (int64_t)((res & 0xff) << 56 | (res & 0xff00000000000000) >> 56 |
					 (res & 0xff00) << 40 | (res & 0xff000000000000) >> 40 |
					 (res & 0xff0000) << 24 | (res & 0xff0000000000) >> 24 |
					 (res & 0xff000000) <<  8 | (res & 0xff00000000) >> 8);
}

uint64_t read_u64_sw(char const **buf)
{
	uint64_t res;
	memcpy(&res, *buf, sizeof(int64_t));
	*buf += sizeof(uint64_t);
	return ((res & 0xff) << 56 | (res & 0xff00000000000000) >> 56 |
			(res & 0xff00) << 40 | (res & 0xff000000000000) >> 40 |
			(res & 0xff0000) << 24 | (res & 0xff0000000000) >> 24 |
			(res & 0xff000000) <<  8 | (res & 0xff00000000) >> 8);
}

double dbl_from_IBM_float(SeisSegy_t sgy, char const **buf)
{
	uint32_t ibm = sgy->read_u32(buf);
	int sign = ibm >> 31 ? -1 : 1;
	int exp = ibm >> 24 & 0x7f;
	uint32_t fraction = ibm & 0x00ffffff;
	return fraction / pow(2, 24) * pow(16, exp - 64) * sign;
}

double dbl_from_IEEE_float(SeisSegy_t sgy, char const **buf)
{
	uint32_t tmp = sgy->read_u32(buf);
	int sign = tmp >> 31 ? -1 : 1;
	int exp = (tmp & 0x7fffffff) >> 23;
	uint32_t fraction = tmp & 0x7fffff;
	return sign * pow(2, exp - 127) * (1 + fraction / pow(2, 23));
}

double dbl_from_IEEE_double(SeisSegy_t sgy, char const **buf)
{
	uint64_t tmp = sgy->read_u64(buf);
	int sign = tmp >> 63 ? -1 : 1;
	int exp = (tmp & 0x7fffffffffffffff) >> 52;
	uint64_t fraction = tmp & 0x000fffffffffffff;
	return sign * pow(2, exp - 1023) * (1 + fraction / pow(2, 52));
}

double dbl_from_IEEE_float_native(SeisSegy_t sgy, char const **buf)
{
	uint32_t tmp = sgy->read_u32(buf);
	float result;
	memcpy(&result, &tmp, sizeof(result));
	return (double)result;
}

double dbl_from_IEEE_double_native(SeisSegy_t sgy, char const **buf)
{
	uint64_t tmp = sgy->read_u64(buf);
	double result;
	memcpy(&result, &tmp, sizeof(result));
	return result;
}

double dbl_from_i8(SeisSegy_t sgy, char const **buf)
{
	return sgy->read_i8(buf);
}

double dbl_from_u8(SeisSegy_t sgy, char const **buf)
{
	return sgy->read_u8(buf);
}

double dbl_from_i16(SeisSegy_t sgy, char const **buf)
{
	return sgy->read_i16(buf);
}

double dbl_from_u16(SeisSegy_t sgy, char const **buf)
{
	return sgy->read_u16(buf);
}

double dbl_from_i24(SeisSegy_t sgy, char const **buf)
{
	return sgy->read_i24(buf);
}

double dbl_from_u24(SeisSegy_t sgy, char const **buf)
{
	return sgy->read_u24(buf);
}

double dbl_from_i32(SeisSegy_t sgy, char const **buf)
{
	return sgy->read_i32(buf);
}

double dbl_from_u32(SeisSegy_t sgy, char const **buf)
{
	return sgy->read_u32(buf);
}

double dbl_from_i64(SeisSegy_t sgy, char const **buf)
{
	return sgy->read_i64(buf);
}

double dbl_from_u64(SeisSegy_t sgy, char const **buf)
{
	return sgy->read_u64(buf);
}
