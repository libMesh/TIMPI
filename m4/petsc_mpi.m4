# -------------------------------------------------------------
# PETSc MPI detection
# -------------------------------------------------------------
AC_DEFUN([CONFIGURE_PETSC_MPI],
[
  AC_ARG_ENABLE(petsc,
                AS_HELP_STRING([--disable-petsc],
                               [build without trying to use PETSc MPI]),
                [AS_CASE("${enableval}",
                         [yes], [enablepetsc_mpi=yes],
                         [no],  [enablepetsc_mpi=no],
                         [AC_MSG_ERROR(bad value ${enableval} for --enable-petsc)])],
                [enablepetsc_mpi=yes])

  AC_ARG_VAR([PETSC_DIR],  [path to PETSc installation])
  AC_ARG_VAR([PETSC_ARCH], [PETSc build architecture])

  AS_IF([test "$enablepetsc_mpi" !=  no],
        [
    AS_ECHO(["Trying to find MPI via PETSc configuration"])

    # Let's use a C compiler for the AC_CHECK_HEADER test, although this is
    # not strictly necessary...
    AC_LANG_PUSH(C)
    AC_CHECK_HEADER($PETSC_DIR/include/petscversion.h,
                    [enablepetsc_mpi=yes],
                    [enablepetsc_mpi=no])
    AC_LANG_POP

    # Grab PETSc version and substitute into Makefile.
    # If version 2.x, also check that PETSC_ARCH is set
    AS_IF([test "$enablepetsc_mpi" !=  no],
          [
            dnl Some tricks to discover the version of petsc.
            dnl You have to have grep and sed for this to work.
            petscmajor=`grep "define PETSC_VERSION_MAJOR" $PETSC_DIR/include/petscversion.h | sed -e "s/#define PETSC_VERSION_MAJOR[ ]*//g"`
            petscminor=`grep "define PETSC_VERSION_MINOR" $PETSC_DIR/include/petscversion.h | sed -e "s/#define PETSC_VERSION_MINOR[ ]*//g"`
            petscsubminor=`grep "define PETSC_VERSION_SUBMINOR" $PETSC_DIR/include/petscversion.h | sed -e "s/#define PETSC_VERSION_SUBMINOR[ ]*//g"`
            petscrelease=`grep "define PETSC_VERSION_RELEASE" $PETSC_DIR/include/petscversion.h | sed -e "s/#define PETSC_VERSION_RELEASE[ ]*//g"`
            petscversion=$petscmajor.$petscminor.$petscsubminor
            petscmajorminor=$petscmajor.$petscminor.x

            AS_IF([test "$petscmajor" = "2" && test "x$PETSC_ARCH" = "x"],
                  [
                    dnl PETSc config failed.
                    enablepetsc_mpi=no
                    AC_MSG_RESULT([<<< PETSc 2.x detected and "\$PETSC_ARCH" not set.  PETSc disabled. >>>])
                  ])
          ],
          [enablepetsc_mpi=no])

    # If we haven't been disabled yet, carry on!
    AS_IF([test $enablepetsc_mpi != no],
          [
        dnl Check for snoopable MPI
        AS_IF([test -r $PETSC_DIR/bmake/$PETSC_ARCH/petscconf], dnl 2.3.x
              [PETSC_MPI=`grep MPIEXEC $PETSC_DIR/bmake/$PETSC_ARCH/petscconf | grep -v mpiexec.uni`],
              [test -r $PETSC_DIR/$PETSC_ARCH/conf/petscvariables], dnl 3.0.x
              [PETSC_MPI=`grep MPIEXEC $PETSC_DIR/$PETSC_ARCH/conf/petscvariables | grep -v mpiexec.uni`],
              [test -r $PETSC_DIR/conf/petscvariables], dnl 3.0.x
              [PETSC_MPI=`grep MPIEXEC $PETSC_DIR/conf/petscvariables | grep -v mpiexec.uni`],
              [test -r $PETSC_DIR/$PETSC_ARCH/lib/petsc/conf/petscvariables], dnl 3.6.x
              [PETSC_MPI=`grep MPIEXEC $PETSC_DIR/$PETSC_ARCH/lib/petsc/conf/petscvariables | grep -v mpiexec.uni`],
              [test -r $PETSC_DIR/lib/petsc/conf/petscvariables], dnl 3.6.x
              [PETSC_MPI=`grep MPIEXEC $PETSC_DIR/lib/petsc/conf/petscvariables | grep -v mpiexec.uni`])

        AS_IF([test "x$PETSC_MPI" != x],
              [
                AC_MSG_RESULT(<<< Attempting to configure library with MPI from PETSC config... >>>)
              ],
              [enablepetsc_mpi=no])

        AS_IF([test "$enablepetsc_mpi" != no],
              [

        # Print informative message about the version of PETSc we detected
        AC_MSG_RESULT([<<< Found PETSc $petscversion installation in $PETSC_DIR ... >>>])

        # Figure out whether this PETSC_DIR is a PETSc source tree or an installed PETSc.

        AS_IF(dnl pre-3.6.0 non-installed PETSc
              [test -r ${PETSC_DIR}/makefile && test -r ${PETSC_DIR}/${PETSC_ARCH}/conf/variables],
              [PREFIX_INSTALLED_PETSC=no
               PETSC_VARS_FILE=${PETSC_DIR}/${PETSC_ARCH}/conf/variables],

              dnl 3.6.0+ non-installed PETSc
              [test -r ${PETSC_DIR}/makefile && test -r ${PETSC_DIR}/${PETSC_ARCH}/lib/petsc/conf/variables],
              [PREFIX_INSTALLED_PETSC=no
               PETSC_VARS_FILE=${PETSC_DIR}/${PETSC_ARCH}/lib/petsc/conf/variables],

              dnl pre 3.6.0 prefix-installed PETSc
              [test -r ${PETSC_DIR}/conf/variables],
              [PREFIX_INSTALLED_PETSC=yes
               PETSC_VARS_FILE=${PETSC_DIR}/conf/variables],

              dnl 3.6.0 prefix-installed PETSc
              [test -r ${PETSC_DIR}/lib/petsc/conf/variables],
              [PREFIX_INSTALLED_PETSC=yes
               PETSC_VARS_FILE=${PETSC_DIR}/lib/petsc/conf/variables],

              dnl Support having a non-prefix-installed PETSc with an
              dnl *incorrectly* set PETSC_ARCH environment variable.  This is
              dnl a less desirable configuration, but we need to support it
              dnl for backwards compatibility.

              dnl pre-3.6.0 non-installed PETSc with invalid $PETSC_ARCH
              [test -r ${PETSC_DIR}/makefile && test -r ${PETSC_DIR}/conf/variables],
              [PREFIX_INSTALLED_PETSC=no
               PETSC_VARS_FILE=${PETSC_DIR}/conf/variables],

              dnl 3.6.0+ non-installed PETSc with invalid $PETSC_ARCH
              [test -r ${PETSC_DIR}/makefile && test -r ${PETSC_DIR}/lib/petsc/conf/variables],
              [PREFIX_INSTALLED_PETSC=no
               PETSC_VARS_FILE=${PETSC_DIR}/lib/petsc/conf/variables],

              dnl If nothing else matched
              [AC_MSG_RESULT([<<< Could not find a viable PETSc Makefile to determine PETSC_CC_INCLUDES, etc. >>>])
               enablepetsc_mpi=no]) dnl AS_IF(petsc version checks)

              ]) dnl AS_IF(enable_petsc)

        AS_IF([test "$enablepetsc_mpi" != no],
              [

        dnl Set some include and link variables by building and running temporary Makefiles.
        AS_IF([test "$PREFIX_INSTALLED_PETSC" = "no"],
              [
                PETSC_CXX=`make -s -C $PETSC_DIR getcxxcompiler`
                PETSC_MPI_INCLUDE_DIRS=`make -s -C $PETSC_DIR getmpiincludedirs`
                PETSC_MPI_LINK_LIBS=`make -s -C $PETSC_DIR getmpilinklibs`
              ],
              [
                printf '%s\n' "include $PETSC_VARS_FILE" > Makefile_config_petsc
                printf '%s\n' "getcxxcompiler:" >> Makefile_config_petsc
                printf '\t%s\n' "echo \$(CXX)" >> Makefile_config_petsc
                printf '%s\n' "getmpiincludedirs:" >> Makefile_config_petsc
                printf '\t%s\n' "echo \$(MPI_INCLUDE)" >> Makefile_config_petsc
                printf '%s\n' "getmpilinklibs:" >> Makefile_config_petsc
                printf '\t%s\n' "echo \$(MPI_LIB)" >> Makefile_config_petsc
                PETSC_CXX=`make -s -f Makefile_config_petsc getcxxcompiler`
                PETSC_MPI_INCLUDE_DIRS=`make -s -f Makefile_config_petsc getmpiincludedirs`
                PETSC_MPI_LINK_LIBS=`make -s -f Makefile_config_petsc getmpilinklibs`
                rm -f Makefile_config_petsc
              ]) dnl AS_IF(test prefix_installed_petsc)
        ]) dnl AS_IF(enablepetsc_mpi)
    ])
  ])

  AS_IF([test "$enablepetsc_mpi" != no],
        [
          PETSC_HAVE_MPI=1
        ])
])
