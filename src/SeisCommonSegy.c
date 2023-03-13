#include "SeisCommonSegy.h"
#include "m-array.h"
#include "m-string.h"
#include "stdlib.h"

ARRAY_DEF(str_arr, string_t)

typedef struct SeisCommonSegyPrivate {
	struct SeisCommonSegy com;
	str_arr_t text_hdrs, end_stanzas;
} *SeisCommonSegyPrivate_t;

SeisCommonSegy *seis_common_segy_new()
{
	SeisCommonSegyPrivate_t priv =
	   	(SeisCommonSegyPrivate_t)malloc(sizeof(struct SeisCommonSegyPrivate));
	if (!priv)
		return NULL;
	priv->com.err.code = SEIS_SEGY_ERR_OK;
	priv->com.err.message = "";
	priv->com.hdr_buf = (char*)malloc(TRACE_HEADER_SIZE);
	if (!priv->com.hdr_buf)
		return NULL;
	priv->com.samp_buf = NULL;
	priv->com.file = NULL;
	str_arr_init(priv->text_hdrs);
	str_arr_init(priv->end_stanzas);
	return (SeisCommonSegy*)priv;
}

void seis_common_segy_unref(SeisCommonSegy *sgy)
{
	SeisCommonSegyPrivate_t psgy = (SeisCommonSegyPrivate_t)sgy;
	free(psgy->com.hdr_buf);
	if (psgy->com.samp_buf)
		free(psgy->com.samp_buf);
	if (psgy->com.file)
		fclose(psgy->com.file);
	str_arr_clear(psgy->text_hdrs);
	str_arr_clear(psgy->end_stanzas);
	free(psgy);
}

void seis_common_add_text_header(SeisCommonSegy *com, char* buf)
{
	SeisCommonSegyPrivate_t priv = (SeisCommonSegyPrivate_t)com;
	string_t tmp;
	string_init(tmp);
	string_set_strn(tmp, buf, TEXT_HEADER_SIZE);
	str_arr_push_back(priv->text_hdrs, tmp);
	string_clear(tmp);
}

void seis_common_add_stanza(SeisCommonSegy *com, char* buf)
{
	SeisCommonSegyPrivate_t priv = (SeisCommonSegyPrivate_t)com;
	string_t tmp;
	string_init(tmp);
	string_set_strn(tmp, buf, TEXT_HEADER_SIZE);
	str_arr_push_back(priv->end_stanzas, tmp);
	string_clear(tmp);
}

size_t seis_common_get_text_headers_num(SeisCommonSegy *com)
{
	SeisCommonSegyPrivate_t priv = (SeisCommonSegyPrivate_t)com;
	return str_arr_size(priv->text_hdrs);
}

char const* seis_common_get_text_header(SeisCommonSegy *com, size_t idx)
{
	SeisCommonSegyPrivate_t priv = (SeisCommonSegyPrivate_t)com;
	string_t *tmp = str_arr_get(priv->text_hdrs, idx);
	return string_get_cstr(*tmp);
}

size_t seis_common_get_stanzas_num(SeisCommonSegy *com)
{
	SeisCommonSegyPrivate_t priv = (SeisCommonSegyPrivate_t)com;
	return str_arr_size(priv->end_stanzas);
}

char const* seis_common_get_stanza(SeisCommonSegy *com, size_t idx)
{
	SeisCommonSegyPrivate_t priv = (SeisCommonSegyPrivate_t)com;
	string_t *tmp = str_arr_get(priv->end_stanzas, idx);
	return string_get_cstr(*tmp);
}
