 dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(timpi, 1.9.0, roystgnr@gmail.com)
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([src/utilities/include/timpi/timpi_config.h.tmp])
dnl Prefixes everything in the temporary file with TIMPI_
AX_PREFIX_CONFIG_H(src/utilities/include/timpi/timpi_config.h,TIMPI,src/utilities/include/timpi/timpi_config.h.tmp)

AC_CONFIG_AUX_DIR([build-aux])
AC_CANONICAL_TARGET()
AM_INIT_AUTOMAKE([dist-bzip2 tar-ustar 1.11 no-define color-tests parallel-tests subdir-objects])
AM_SILENT_RULES(yes)  # use silent rules where available - automake 1.11

dnl Turn off AM_MAINTAINER_MODE for bootstrapped release by
dnl uncommenting this:
AM_MAINTAINER_MODE # git trashes timestamps, forcing autoreconf needlessly

dnl Release versioning
AX_SPLIT_VERSION
GENERIC_MAJOR_VERSION=$AX_MAJOR_VERSION
GENERIC_MINOR_VERSION=$AX_MINOR_VERSION
GENERIC_MICRO_VERSION=$AX_POINT_VERSION
GENERIC_LIBRARY_NAME=timpi

dnl shared library versioning

PACKAGE=$GENERIC_LIBRARY_NAME
GENERIC_VERSION=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION.$GENERIC_MICRO_VERSION
GENERIC_RELEASE=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION

dnl AC_SUBST(GENERIC_LIBRARY_VERSION)
dnl AC_SUBST(GENERIC_LIBRARY_NAME)

AC_SUBST(GENERIC_VERSION)
AC_SUBST(GENERIC_RELEASE)
AC_SUBST(GENERIC_MAJOR_VERSION)
AC_SUBST(GENERIC_MINOR_VERSION)
AC_SUBST(GENERIC_MICRO_VERSION)

AC_DEFINE_UNQUOTED([MAJOR_VERSION],$GENERIC_MAJOR_VERSION,[Major version])
AC_DEFINE_UNQUOTED([MINOR_VERSION],$GENERIC_MINOR_VERSION,[Minor version])
AC_DEFINE_UNQUOTED([MICRO_VERSION],$GENERIC_MICRO_VERSION,[Micro version])


# by default CFLAGS and CXXFLAGS are set to '-g -O2' on systems that support them.
# this causes a problem if we then declare a different optimization level. So
# default them to empty, regardless of what the system supports.
: ${CFLAGS=""}
: ${CXXFLAGS=""}

#--------------------------------------------------------------------
# Check for whether to enable MPI and PETSc which will influence our
# choice of compiler
#--------------------------------------------------------------------
ACSM_COMPILER_CONTROL_ARGS

#-----------------------------------------------------------------------
# Scrape PETSc configure information for their CXX, MPI_INCLUDE, MPI_LIB
#-----------------------------------------------------------------------
ACSM_SCRAPE_PETSC_CONFIGURE


dnl------------------------------
dnl Checks for compilers
dnl------------------------------

TIMPI_SET_COMPILERS

# --------------------------------------------------------------
# Autoconf macro for determining the proper -std=c++??
# flag, for the current compiler, for the user's requested C++
# standards level.  Adds the required flag to CXXFLAGS if
# one is found.  Exits if no acceptable flag is found.
#
# We've started relying on C++14 code in TIMPI; we also use C++17
# but with backward compatibility shims.
# --------------------------------------------------------------
ACSM_CXX_COMPILER_STANDARD([2014], [2017])

#-----------------------------------------------------
# Set compiler flags for devel, opt, etc. methods
#-----------------------------------------------------
TIMPI_SET_CXX_FLAGS

#------------------------------------------------------
# Checks for code coverage
# this must be done after we look for our own compilers
#------------------------------------------------------
ACSM_CODE_COVERAGE
timpi_CFLAGS="$GCOV_FLAGS $timpi_CFLAGS"
timpi_CXXFLAGS="$GCOV_FLAGS $timpi_CXXFLAGS"

SET_BUILD_METHODS

ACSM_ENABLE_PARANOID

ACSM_ENABLE_WERROR

dnl---------------------------------------------------------
dnl Checks for library prerequisites for other libraries...
dnl---------------------------------------------------------

dnl------------------------------------------------------
dnl If the user didn't --disable-mpi, try hard to find it
dnl------------------------------------------------------
timpi_optional_INCLUDES=""
timpi_optional_LIBS=""

AS_IF([test "x$enablempi" = xyes],
      [
        ACSM_MPI
        AS_IF([test "x$enablempi" = xyes],
              [
                AS_IF([test x"$MPI_INCLUDES" = x],,[timpi_optional_INCLUDES="$MPI_INCLUDES $timpi_optional_INCLUDES"])
                AS_IF([test x"$MPI_LIBS" != x], [timpi_optional_LIBS="$MPI_LIBS $timpi_optional_LIBS"])
                AS_IF([test x"$MPI_LDFLAGS" != x], [timpi_optional_LIBS="$MPI_LDFLAGS $timpi_optional_LIBS"])
              ])
      ])

AC_SUBST(timpi_optional_INCLUDES)
AC_SUBST(timpi_optional_LIBS)

dnl---------------------------------------------------------
dnl Sets library options
dnl---------------------------------------------------------

TIMPI_OPTIONAL_FEATURES

dnl-----------------------------------------------------
dnl Initialize libtool.  By default, we will build
dnl only shared libraries on platforms that support them
dnl-----------------------------------------------------
LT_INIT([disable-static])

dnl---------------------------------
dnl Query configuration environment
dnl---------------------------------
ACSM_SUMMARIZE_ENV

dnl-----------------------------------------------
dnl Generate header files
dnl-----------------------------------------------
AC_CONFIG_FILES(src/utilities/include/timpi/timpi_version.h)

dnl-----------------------------------------------
dnl Generate Makefiles
dnl-----------------------------------------------
AC_CONFIG_FILES([
  Makefile
  timpi.pc
  src/Makefile
  test/Makefile
  ])

AC_CONFIG_FILES(bin/timpi-config, [chmod +x bin/timpi-config])
AC_CONFIG_FILES(test/run_unit_tests.sh, [chmod +x test/run_unit_tests.sh])

# Must still call AC_OUTPUT() after generating all the files
AC_OUTPUT()

dnl------------------------------
dnl Final summary
dnl------------------------------
AX_SUMMARIZE_CONFIG
