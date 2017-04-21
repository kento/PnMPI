/* This file is part of P^nMPI.
 *
 * Copyright (c)
 *  2008-2017 Lawrence Livermore National Laboratories, United States of America
 *  2011-2017 ZIH, Technische Universitaet Dresden, Federal Republic of Germany
 *  2013-2017 RWTH Aachen University, Federal Republic of Germany
 *
 *
 * P^nMPI is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation version 2.1 dated February 1999.
 *
 * P^nMPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with P^nMPI; if not, write to the
 *
 *   Free Software Foundation, Inc.
 *   51 Franklin St, Fifth Floor
 *   Boston, MA 02110, USA
 *
 *
 * Written by Martin Schulz, schulzm@llnl.gov.
 *
 * LLNL-CODE-402774
 */

/** \defgroup pnmpi_debug_io Print debug messages, warnings and  errors.
 */

#ifndef PNMPI_PRINT_H
#define PNMPI_PRINT_H


#include <pnmpi/attributes.h>


/** \brief Debug levels for \ref PNMPI_Debug.
 *
 *
 * \ingroup pnmpi_debug_io
 */
typedef enum pnmpi_debug_level {
  /** \brief Messages about PnMPI initialization (before MPI is initialized).
   *
   * \note Users should not enable this level when running PnMPI with more than
   *  one rank, as messages will be printed on all ranks, because PnMPI can't
   *  get the rank of the process at this time yet.
   */
  PNMPI_DEBUG_INIT = (1 << 0),

  /** \brief Messages about module loading.
   *
   * \details This log level is used for information about the loaded modules,
   *  what functions they provide and their settings.
   *
   * \note Users should not enable this level when running PnMPI with more than
   *  one rank, as messages will be printed on all ranks, because PnMPI can't
   *  get the rank of the process at this time yet.
   */
  PNMPI_DEBUG_MODULE = (1 << 1),

  /** \brief Messages about MPI call entry and exit.
   *
   * \details This log level is used for information about entry and exit points
   *  of MPI calls of the different layers.
   */
  PNMPI_DEBUG_CALL = (1 << 2)
} PNMPI_debug_level_t;


/** \brief Prefix for messages generated by PnMPI functions.
 *
 * \details This macro may be set by any file including this one to add a prefix
 *  in front of \ref PNMPI_Debug, \ref PNMPI_Warning or \ref PNMPI_Error. The
 *  output of these functions will be prefixed by the value of this macro
 *  surrounded by square brackets following by a space, e.g. `[PnMPI] `.
 *
 * \note If not set, the default value will be `PnMPI`.
 * \note This macro must be set \b before including any of PnMPI's headers.
 */
#ifndef PNMPI_MESSAGE_PREFIX
#define PNMPI_MESSAGE_PREFIX "PnMPI"
#endif


/** \brief Macro to use for debug printing.
 *
 * \details To optimize the code for speed, debug printing can be disabled with
 *  the PNMPI_NO_DEBUG compile flag.
 *
 * \note This macro will preprend the \ref PNMPI_MESSAGE_PREFIX.
 *
 *
 * \ingroup pnmpi_debug_io
 */
#ifndef PNMPI_NO_DEBUG
#define PNMPI_Debug(level, ...) \
  pnmpi_print_debug(level, "[" PNMPI_MESSAGE_PREFIX "] " __VA_ARGS__)
#else
#define PNMPI_Debug(...)
#endif


/** \brief Macro to use for warning printing.
 *
 * \note This macro will preprend the \ref PNMPI_MESSAGE_PREFIX.
 *
 *
 * \ingroup pnmpi_debug_io
 */
#define PNMPI_Warning(...) \
  pnmpi_print_warning("[" PNMPI_MESSAGE_PREFIX "] " __VA_ARGS__)


/** \brief Print an error message with file and line number and exit.
 *
 * \details This macro is used as shortcut to print a warning with the current
 *  function and line number and exiting the program with an error code after
 *  printing the message.
 *
 * \internal Instead of directly putting calls to \ref PNMPI_Warning and exit in
 *  the macro, the wrapper-function \ref pnmpi_print_error is used, as non-GCC
 *  compilers don't understand \c \#\#__VA_ARGS__ to delete the comma after the
 *  format string if no other arguments are passed.
 *
 * \note This macro will preprend the \ref PNMPI_MESSAGE_PREFIX.
 *
 *
 * \param ... Arguments to be evaluated. First argument has to be the
 *  printf-like format string.
 *
 *
 * \ingroup pnmpi_debug_io
 */
#define PNMPI_Error(...) \
  pnmpi_print_error(PNMPI_MESSAGE_PREFIX, __FUNCTION__, __LINE__, __VA_ARGS__);


/* The PnMPI API should be C++ compatible, too. We have to add the extern "C"
 * stanza to avoid name mangeling. Otherwise C++ PnMPI modules would not find
 * PnMPI API functions. */
#ifdef __cplusplus
extern "C" {
#endif


PNMPI_FUNCTION_PRINTF(2, 3)
PNMPI_FUNCTION_ARG_NONNULL(2)
void pnmpi_print_debug(PNMPI_debug_level_t level, const char *format, ...);

PNMPI_FUNCTION_PRINTF(4, 5)
PNMPI_FUNCTION_ARG_NONNULL(1)
PNMPI_FUNCTION_ARG_NONNULL(2)
PNMPI_FUNCTION_ARG_NONNULL(4)
PNMPI_FUNCTION_NORETURN
void pnmpi_print_error(const char *prefix, const char *function, const int line,
                       const char *format, ...);

PNMPI_FUNCTION_PRINTF(1, 2)
PNMPI_FUNCTION_ARG_NONNULL(1)
void pnmpi_print_warning(const char *format, ...);


#ifdef __cplusplus
}
#endif


#endif
