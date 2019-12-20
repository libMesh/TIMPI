#ifndef _SRC_UTILITIES_INCLUDE_TIMPI_TIMPI_CONFIG_H
#define _SRC_UTILITIES_INCLUDE_TIMPI_TIMPI_CONFIG_H 1
 
/* src/utilities/include/timpi/timpi_config.h. Generated automatically at end of configure. */
/* src/utilities/include/timpi/timpi_config.h.tmp.  Generated from timpi_config.h.tmp.in by configure.  */
/* src/utilities/include/timpi/timpi_config.h.tmp.in.  Generated from configure.ac by autoheader.  */

/* Architecture of the build host */
#ifndef TIMPI_BUILD_ARCH
#define TIMPI_BUILD_ARCH "x86_64-pc-linux-gnu"
#endif

/* Build date */
#ifndef TIMPI_BUILD_DATE
#define TIMPI_BUILD_DATE __DATE__ " " __TIME__
#endif

/* Dev/Release build */
#ifndef TIMPI_BUILD_DEVSTATUS
#define TIMPI_BUILD_DEVSTATUS "Development Build"
#endif

/* Build host name */
#ifndef TIMPI_BUILD_HOST
#define TIMPI_BUILD_HOST "stogner"
#endif

/* The fine user who built the package */
#ifndef TIMPI_BUILD_USER
#define TIMPI_BUILD_USER "roystgnr"
#endif

/* SVN revision */
#ifndef TIMPI_BUILD_VERSION
#define TIMPI_BUILD_VERSION "5435a3aa790d6e5152a8ce26d9da992c7925cb61"
#endif

/* Flag indicating if the library should support deprecated code */
#ifndef TIMPI_ENABLE_DEPRECATED
#define TIMPI_ENABLE_DEPRECATED 1
#endif

/* Flag indicating if the library should be built to throw C++ exceptions on
   unexpected errors */
#ifndef TIMPI_ENABLE_EXCEPTIONS
#define TIMPI_ENABLE_EXCEPTIONS 1
#endif

/* Flag indicating if the library should be built with compile time and date
   timestamps */
#ifndef TIMPI_ENABLE_TIMESTAMPS
#define TIMPI_ENABLE_TIMESTAMPS 1
#endif

/* Flag indicating if the library should have warnings enabled */
#ifndef TIMPI_ENABLE_WARNINGS
#define TIMPI_ENABLE_WARNINGS 1
#endif

/* define if the compiler supports basic C++11 syntax */
#ifndef TIMPI_HAVE_CXX11
#define TIMPI_HAVE_CXX11 1
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
#ifndef TIMPI_HAVE_DLFCN_H
#define TIMPI_HAVE_DLFCN_H 1
#endif

/* Define to 1 if you have the <inttypes.h> header file. */
#ifndef TIMPI_HAVE_INTTYPES_H
#define TIMPI_HAVE_INTTYPES_H 1
#endif

/* Define to 1 if you have the <memory.h> header file. */
#ifndef TIMPI_HAVE_MEMORY_H
#define TIMPI_HAVE_MEMORY_H 1
#endif

/* Flag indicating whether or not MPI is available */
#ifndef TIMPI_HAVE_MPI
#define TIMPI_HAVE_MPI 1
#endif

/* Define to 1 if you have the <stdint.h> header file. */
#ifndef TIMPI_HAVE_STDINT_H
#define TIMPI_HAVE_STDINT_H 1
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#ifndef TIMPI_HAVE_STDLIB_H
#define TIMPI_HAVE_STDLIB_H 1
#endif

/* Define to 1 if you have the <strings.h> header file. */
#ifndef TIMPI_HAVE_STRINGS_H
#define TIMPI_HAVE_STRINGS_H 1
#endif

/* Define to 1 if you have the <string.h> header file. */
#ifndef TIMPI_HAVE_STRING_H
#define TIMPI_HAVE_STRING_H 1
#endif

/* Define to 1 if you have the <sys/stat.h> header file. */
#ifndef TIMPI_HAVE_SYS_STAT_H
#define TIMPI_HAVE_SYS_STAT_H 1
#endif

/* Define to 1 if you have the <sys/types.h> header file. */
#ifndef TIMPI_HAVE_SYS_TYPES_H
#define TIMPI_HAVE_SYS_TYPES_H 1
#endif

/* Define to 1 if you have the <unistd.h> header file. */
#ifndef TIMPI_HAVE_UNISTD_H
#define TIMPI_HAVE_UNISTD_H 1
#endif

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#ifndef TIMPI_LT_OBJDIR
#define TIMPI_LT_OBJDIR ".libs/"
#endif

/* Major version */
#ifndef TIMPI_MAJOR_VERSION
#define TIMPI_MAJOR_VERSION 0
#endif

/* Micro version */
#ifndef TIMPI_MICRO_VERSION
#define TIMPI_MICRO_VERSION 0
#endif

/* Minor version */
#ifndef TIMPI_MINOR_VERSION
#define TIMPI_MINOR_VERSION 0
#endif

/* Define to the address where bug reports for this package should be sent. */
#ifndef TIMPI_PACKAGE_BUGREPORT
#define TIMPI_PACKAGE_BUGREPORT "roystgnr@ices.utexas.edu"
#endif

/* Define to the full name of this package. */
#ifndef TIMPI_PACKAGE_NAME
#define TIMPI_PACKAGE_NAME "timpi"
#endif

/* Define to the full name and version of this package. */
#ifndef TIMPI_PACKAGE_STRING
#define TIMPI_PACKAGE_STRING "timpi 0.0.0"
#endif

/* Define to the one symbol short name of this package. */
#ifndef TIMPI_PACKAGE_TARNAME
#define TIMPI_PACKAGE_TARNAME "timpi"
#endif

/* Define to the home page for this package. */
#ifndef TIMPI_PACKAGE_URL
#define TIMPI_PACKAGE_URL ""
#endif

/* Define to the version of this package. */
#ifndef TIMPI_PACKAGE_VERSION
#define TIMPI_PACKAGE_VERSION "0.0.0"
#endif

/* size of processor_id */
#ifndef TIMPI_PROCESSOR_ID_BYTES
#define TIMPI_PROCESSOR_ID_BYTES 4
#endif

/* Define to 1 if you have the ANSI C header files. */
#ifndef TIMPI_STDC_HEADERS
#define TIMPI_STDC_HEADERS 1
#endif
 
/* once: _SRC_UTILITIES_INCLUDE_TIMPI_TIMPI_CONFIG_H */
#endif
