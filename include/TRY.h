/**
 * \file TRY.h
 * \brief try macros for shorter error handling
 * \author andalevor
 * \date 2023\03\30
 */

#ifndef TRY_H
#define TRY_H

#define TRY(x)                                                                 \
  do {                                                                         \
    if (x)                                                                     \
      goto error;                                                              \
  } while (0)

#endif /* TRY.h */
