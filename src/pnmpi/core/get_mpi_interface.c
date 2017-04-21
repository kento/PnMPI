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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <pnmpi/debug_io.h>
#include <pnmpi/private/attributes.h>
#include <pnmpi/private/config.h>
#include <pnmpi/private/mpi_interface.h>


/** \brief MPI interface used for `MPI_Init`.
 *
 * \details This variable stores the MPI interface used by the instrumented
 *  application for MPI initialization.
 *
 *
 * \private
 */
PNMPI_INTERNAL
pnmpi_mpi_interface pnmpi_mpi_init_interface = PNMPI_INTERFACE_NONE;


/** \brief Convert \p value to an \ref pnmpi_mpi_interface value.
 *
 * \details This function converts the value of \p s into a valid MPI interface
 *  language of \ref pnmpi_mpi_interface. The value of \p s will be compared
 *  case insensitive.
 *
 * \note If \p s is not a valid MPI interface language, \ref PNMPI_Error will be
 *  called and PnMPI will exit immediately.
 *
 *
 * \param value The string to be compared. Should be either `C` or `Fortran`
 *  (case insensitive).
 *
 * \return \ref PNMPI_INTERFACE_C User defined the C MPI interface to be used.
 * \return \ref PNMPI_INTERFACE_FORTRAN User defined the Fortran MPI interface
 *  to be used.
 */
static pnmpi_mpi_interface convert_env(const char *value)
{
  assert(value);


  if (strcasecmp(value, "C") == 0)
    return PNMPI_INTERFACE_C;
#ifdef ENABLE_FORTRAN
  else if (strcasecmp(value, "Fortran") == 0)
    return PNMPI_INTERFACE_FORTRAN;
#endif
  else
    PNMPI_Error("Unknown MPI interface '%s'.\n", value);
}


#ifdef __PIC__
/** \brief Check \p cmd for the used MPI interface.
 *
 * \details This function calls `nm` on \p cmd to determine the used MPI
 *  interface. If \p cmd uses the `MPI_Init` symbol, we can reliably tell, this
 *  is an application using the C MPI interface.
 *
 * \note If the application doesn't use dynamic symbols or doesn't use the
 *  `MPI_Init` symbol (i.e. is a non-MPI application), \ref
 *  PNMPI_INTERFACE_FORTRAN will be returned, which is false. You'll have to
 *  decide, if this affects your use-case.
 *
 *
 * \param cmd The instrumented application (= argv[0]).
 *
 * \return \ref PNMPI_INTERFACE_C \p cmd uses the C MPI interface.
 * \return \ref PNMPI_INTERFACE_FORTRAN \p cmd uses the Fortran MPI interface.
 * \return \ref PNMPI_INTERFACE_NONE \p cmd uses no known MPI interface.
 * \return \ref PNMPI_INTERFACE_UNKNOWN An error occured.
 */
static pnmpi_mpi_interface check_nm(const char *cmd)
{
  assert(cmd);


  /* Before executing any command via system(), LD_PRELOAD and similar
   * environment variables have to be cleared. Otherwise the commands will be
   * called with a preloaded PnMPI, which itself would execute this function, so
   * this would result in a fork bomb. */
  unsetenv("LD_PRELOAD");
  unsetenv("DYLD_INSERT_LIBRARIES");

  /* Check for all required commands first. If one of the required commands is
   * not available, print a warning and tell the callee that we can't determine
   * the MPI interface language. */
  if (system("which grep nm > /dev/null 2>&1") != 0)
    {
      PNMPI_Warning("Automatic MPI interface language detection requires the "
                    "commands grep, nm and which in your PATH.\n");
      return PNMPI_INTERFACE_UNKNOWN;
    }

  /* To determine the MPI interface language used by the instrumented
   * application, we'll have to check the following three cases:
   *
   *  1. The application uses either MPI_Init or MPI_Init_thread. These are
   *     symbols of the MPI C interface, so the application is using this
   *     interface.
   *
   *  2. The application uses a case insensitive symbol named mpi_init or
   *     mpi_init_thread, but not the ones of (1). The application is using the
   *     MPI Fortran interface.
   *
   *  3. In any other case, the application will use no (known) MPI interface.
   *
   * To lookup the used symbols, `nm` will be used. As it can't handle commands
   * from PATH, `which` will be used in this case to determine the actual
   * location of the binary. */
  const char *format =
    (access(cmd, F_OK) != -1)
      ? "nm %s 2>/dev/null | grep %s \"[^P]MPI_Init\" >/dev/null"
      : "nm $(which %s) 2>/dev/null | grep %s \"[^P]MPI_Init\" >/dev/null";

  /* Create a buffer for the command to be used. The length should be
   * sufficient for format with the substituded parameters. */
  size_t len = strlen(cmd) + strlen(format) - 1;
  char command[len];

  /* First handle case (1) for the C interface, so we'll lookup the symbol
   * case-sensitive. No additional expression for MPI_Init_thread is required,
   * as this is handled with MPI_Init, too. */
  int n = snprintf(command, len, format, cmd, "");
  if ((n >= 0 && n <= len) && (system(command) == 0))
#if !defined(ENABLE_FORTRAN) || !defined(__APPLE__)
    return PNMPI_INTERFACE_C;
#else
    {
      /* At mac OS, the MPI_Init symbol will be found for both C and Fortran MPI
       * applications. Currently there is no reliable way to tell, which
       * interface the application is using. The best we can do is using the
       * Fortran interface in this case, as this will itself call the C MPI
       * initializer and should have no side effects. However, if the user
       * observes errors, he should set the MPI interface to use explictly. */
      PNMPI_Warning("At mac OS, the MPI C and Fortran interfaces can't be "
                    "distinguished yet. The MPI Fortran interface will be used "
                    "now. This should have no side-effects. However, if you "
                    "observe any errors, set the MPI interface to use "
                    "explictly.\n");
      return PNMPI_INTERFACE_FORTRAN;
    }
#endif
  else
    {
#ifdef ENABLE_FORTRAN
      /* Next handle case (2) for the Fortran interface, so we'll lookup the
       * same expression case insensitive. */
      n = snprintf(command, len, format, cmd, "-i");
      if ((n >= 0 && n <= len) && (system(command) == 0))
        return PNMPI_INTERFACE_FORTRAN;

      /* In any other case, an unknown MPI interface will be used. */
      else
#endif
        return PNMPI_INTERFACE_NONE;
    }
}
#endif


/** \brief Get the MPI interface used by the instrumented application.
 *
 * \details This function determines the MPI interface \p cmd is using, so one
 *  can decide which code to execute depending on the used interface (e.g.
 *  starting Fortran MPI in \ref pnmpi_app_startup).
 *
 * \internal
 * \note See \ref check_nm for common problems of the auto-detection.
 * \endinternal
 *
 *
 * \param cmd The instrumented application (= argv[0]). If this parameter is the
 *  NULL-pointer, the instrumented application will not be checked for its used
 *  symbols.
 *
 * \return \ref PNMPI_INTERFACE_C \p cmd uses the C MPI interface.
 * \return \ref PNMPI_INTERFACE_FORTRAN \p cmd uses the Fortran MPI interface.
 * \return \ref PNMPI_INTERFACE_NONE \p cmd uses no known MPI interface.
 * \return \ref PNMPI_INTERFACE_UNKNOWN An error occured or no interface has
 *  been determined yet.
 *
 *
 * \private
 */
PNMPI_INTERNAL
pnmpi_mpi_interface pnmpi_get_mpi_interface(const char *cmd)
{
  /* Check for previous invocations of this function first. If a value has been
   * cached, avoid detecting the interface language a second time and return the
   * value from cache. */
  static pnmpi_mpi_interface mpi_interface = PNMPI_INTERFACE_UNKNOWN;
  if (mpi_interface != PNMPI_INTERFACE_UNKNOWN)
    return mpi_interface;


  /* If the instrumented application did initialize MPI yet, use the language of
   * the used interface. */
  if (pnmpi_mpi_init_interface != PNMPI_INTERFACE_NONE)
    return mpi_interface = pnmpi_mpi_init_interface;

  /* If the user defined the interface language in the environment, use the
   * defined language instead of detecting one. The interface language will be
   * stored in the PNMPI_INTERFACE environment variable and must be either C or
   * Fortran (case insensitive). */
  const char *env = getenv("PNMPI_INTERFACE");
  if (env != NULL)
    {
      PNMPI_Debug(PNMPI_DEBUG_INIT,
                  "Using MPI interface '%s' defined in environment.\n", env);
      return mpi_interface = convert_env(env);
    }

#ifdef __PIC__
  /* If no interface language was set in the environment, check the application
   * with nm for the used MPI interface. This will be done for non-NULL cmd
   * parameter.
   *
   * This method is supported for the shared library version of PnMPI, as a
   * binary linked to the static one will always include the MPI_Init symbol, so
   * it would be marked as C, even if it doesn't use any MPI function. */
  if (cmd != NULL)
    return mpi_interface = check_nm(cmd);
  else
#endif
    return PNMPI_INTERFACE_UNKNOWN;
}
