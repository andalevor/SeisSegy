/**
 * \file SeisEncodings.h
 * \brief functions for ascii and ebcdic conversion.
 * \author andalevor
 * \date 2023\03\01
 */

#ifndef SEIS_ENCODINGS_H
#define SEIS_ENCODINGS_H

/**
 * \fn ebcdic_to_ascii
 * \brief change every byte in string from ebcdic to ascii encoding.
 * \param s null terminated string
 */
void ebcdic_to_ascii(char *s);

/**
 * \fn ascii_to_ebcdic
 * \brief change every byte in string from ascii to ebcdic encoding.
 * \param s null terminated string
 */
void ascii_to_ebcdic(char *s);

#endif /* SEIS_ENCODINGS_H */
