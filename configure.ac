AC_PREREQ([2.69])
AC_INIT([iBASIC], [0.1], [ian@mnementh.co.uk], [ibasic], [https://github.com/spyro2/ibasic])
AC_CONFIG_SRCDIR([ast.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([memmove strchr strtoul])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
