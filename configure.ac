#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([dsarch], [1.0.0], [vsam@softnet.tuc.gr])
AM_INIT_AUTOMAKE
# LT_INIT

AC_CONFIG_SRCDIR([dsarch.hh])
AC_CONFIG_HEADERS([config.h])

AC_ARG_ENABLE(debug,
     [  --enable-debug    Turn on debugging],
     [case "${enableval}" in
       yes) debug=true ;;
       no)  debug=false ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
     esac],[debug=false])

AM_CONDITIONAL(DEBUG, test x$debug = xtrue)


# Checks for programs.
AC_PROG_CXX
AC_PROG_RANLIB

# Checks for libraries.
AX_BOOST_BASE([1.58.0], [], [AC_MSG_ERROR(
              [Please install boost >= 1.58 (including filesystem)])])

DX_INIT_DOXYGEN([dsarch])
DX_DOXYGEN_FEATURE(ON)
DX_DOT_FEATURE(ON)
DX_HTML_FEATURE(ON)
DX_PS_FEATURE(OFF)
DX_PDF_FEATURE(OFF)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
# AC_CHECK_HEADER_STDBOOL
# AC_C_INLINE
# AC_TYPE_INT32_T
# AC_TYPE_SIZE_T
# AC_TYPE_UINT32_T

# Checks for library functions.
# AC_CHECK_FUNCS([select])

AC_CONFIG_FILES([
	Makefile
])

AC_OUTPUT
