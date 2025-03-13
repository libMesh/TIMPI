
AM_CPPFLAGS  = $(timpi_optional_INCLUDES)
AM_CPPFLAGS += -I$(top_srcdir)/src/algorithms/include
AM_CPPFLAGS += -I$(top_srcdir)/src/parallel/include
AM_CPPFLAGS += -I$(top_srcdir)/src/utilities/include
AM_CPPFLAGS += -I$(top_builddir)/src/utilities/include #timpi_version.h, timpi_config.h

CXXFLAGS_DBG += $(ACSM_ANY_WERROR_FLAG) $(ACSM_ANY_PARANOID_FLAGS)
CXXFLAGS_DEVEL += $(ACSM_ANY_WERROR_FLAG) $(ACSM_ANY_PARANOID_FLAGS)
CXXFLAGS_OPROF += $(ACSM_ANY_WERROR_FLAG) $(ACSM_ANY_PARANOID_FLAGS)
CXXFLAGS_OPT += $(ACSM_ANY_WERROR_FLAG) $(ACSM_ANY_PARANOID_FLAGS)
CXXFLAGS_PROF += $(ACSM_ANY_WERROR_FLAG) $(ACSM_ANY_PARANOID_FLAGS)

LIBS         = $(timpi_optional_LIBS)

#Script to run underlying tests
TESTS = run_unit_tests.sh
check_PROGRAMS =

if BUILD_DBG_MODE
  message_tag_unit_dbg_SOURCES = message_tag_unit.C
  message_tag_unit_dbg_LDFLAGS = $(top_builddir)/src/libtimpi_dbg.la
  message_tag_unit_dbg_CPPFLAGS = $(CPPFLAGS_DBG) $(AM_CPPFLAGS)
  message_tag_unit_dbg_CXXFLAGS = $(CXXFLAGS_DBG)

  packed_range_unit_dbg_SOURCES = packed_range_unit.C
  packed_range_unit_dbg_LDFLAGS = $(top_builddir)/src/libtimpi_dbg.la
  packed_range_unit_dbg_CPPFLAGS = $(CPPFLAGS_DBG) $(AM_CPPFLAGS)
  packed_range_unit_dbg_CXXFLAGS = $(CXXFLAGS_DBG)

  parallel_sync_unit_dbg_SOURCES = parallel_sync_unit.C
  parallel_sync_unit_dbg_LDFLAGS = $(top_builddir)/src/libtimpi_dbg.la
  parallel_sync_unit_dbg_CPPFLAGS = $(CPPFLAGS_DBG) $(AM_CPPFLAGS)
  parallel_sync_unit_dbg_CXXFLAGS = $(CXXFLAGS_DBG)

  parallel_unit_dbg_SOURCES = parallel_unit.C
  parallel_unit_dbg_LDFLAGS = $(top_builddir)/src/libtimpi_dbg.la
  parallel_unit_dbg_CPPFLAGS = $(CPPFLAGS_DBG) $(AM_CPPFLAGS)
  parallel_unit_dbg_CXXFLAGS = $(CXXFLAGS_DBG)

  set_unit_dbg_SOURCES = set_unit.C
  set_unit_dbg_LDFLAGS = $(top_builddir)/src/libtimpi_dbg.la
  set_unit_dbg_CPPFLAGS = $(CPPFLAGS_DBG) $(AM_CPPFLAGS)
  set_unit_dbg_CXXFLAGS = $(CXXFLAGS_DBG)

  dispatch_to_packed_unit_dbg_SOURCES = dispatch_to_packed_unit.C
  dispatch_to_packed_unit_dbg_LDFLAGS = $(top_builddir)/src/libtimpi_dbg.la
  dispatch_to_packed_unit_dbg_CPPFLAGS = $(CPPFLAGS_DBG) $(AM_CPPFLAGS)
  dispatch_to_packed_unit_dbg_CXXFLAGS = $(CXXFLAGS_DBG)

  utility_unit_dbg_SOURCES = utility_unit.C
  utility_unit_dbg_LDFLAGS = $(top_builddir)/src/libtimpi_dbg.la
  utility_unit_dbg_CPPFLAGS = $(CPPFLAGS_DBG) $(AM_CPPFLAGS)
  utility_unit_dbg_CXXFLAGS = $(CXXFLAGS_DBG)

  check_PROGRAMS += message_tag_unit-dbg
  check_PROGRAMS += packed_range_unit-dbg
  check_PROGRAMS += parallel_sync_unit-dbg
  check_PROGRAMS += parallel_unit-dbg
  check_PROGRAMS += set_unit-dbg
  check_PROGRAMS += dispatch_to_packed_unit-dbg
  check_PROGRAMS += utility_unit-dbg
endif

if BUILD_DEVEL_MODE
  message_tag_unit_devel_SOURCES = message_tag_unit.C
  message_tag_unit_devel_LDFLAGS = $(top_builddir)/src/libtimpi_devel.la
  message_tag_unit_devel_CPPFLAGS = $(CPPFLAGS_DEVEL) $(AM_CPPFLAGS)
  message_tag_unit_devel_CXXFLAGS = $(CXXFLAGS_DEVEL)

  packed_range_unit_devel_SOURCES = packed_range_unit.C
  packed_range_unit_devel_LDFLAGS = $(top_builddir)/src/libtimpi_devel.la
  packed_range_unit_devel_CPPFLAGS = $(CPPFLAGS_DEVEL) $(AM_CPPFLAGS)
  packed_range_unit_devel_CXXFLAGS = $(CXXFLAGS_DEVEL)

  parallel_sync_unit_devel_SOURCES = parallel_sync_unit.C
  parallel_sync_unit_devel_LDFLAGS = $(top_builddir)/src/libtimpi_devel.la
  parallel_sync_unit_devel_CPPFLAGS = $(CPPFLAGS_DEVEL) $(AM_CPPFLAGS)
  parallel_sync_unit_devel_CXXFLAGS = $(CXXFLAGS_DEVEL)

  parallel_unit_devel_SOURCES = parallel_unit.C
  parallel_unit_devel_LDFLAGS = $(top_builddir)/src/libtimpi_devel.la
  parallel_unit_devel_CPPFLAGS = $(CPPFLAGS_DEVEL) $(AM_CPPFLAGS)
  parallel_unit_devel_CXXFLAGS = $(CXXFLAGS_DEVEL)

  set_unit_devel_SOURCES = set_unit.C
  set_unit_devel_LDFLAGS = $(top_builddir)/src/libtimpi_devel.la
  set_unit_devel_CPPFLAGS = $(CPPFLAGS_DEVEL) $(AM_CPPFLAGS)
  set_unit_devel_CXXFLAGS = $(CXXFLAGS_DEVEL)

  dispatch_to_packed_unit_devel_SOURCES = dispatch_to_packed_unit.C
  dispatch_to_packed_unit_devel_LDFLAGS = $(top_builddir)/src/libtimpi_devel.la
  dispatch_to_packed_unit_devel_CPPFLAGS = $(CPPFLAGS_DEVEL) $(AM_CPPFLAGS)
  dispatch_to_packed_unit_devel_CXXFLAGS = $(CXXFLAGS_DEVEL)

  utility_unit_devel_SOURCES = utility_unit.C
  utility_unit_devel_LDFLAGS = $(top_builddir)/src/libtimpi_devel.la
  utility_unit_devel_CPPFLAGS = $(CPPFLAGS_DEVEL) $(AM_CPPFLAGS)
  utility_unit_devel_CXXFLAGS = $(CXXFLAGS_DEVEL)

  check_PROGRAMS += message_tag_unit-devel
  check_PROGRAMS += packed_range_unit-devel
  check_PROGRAMS += parallel_sync_unit-devel
  check_PROGRAMS += parallel_unit-devel
  check_PROGRAMS += set_unit-devel
  check_PROGRAMS += dispatch_to_packed_unit-devel
  check_PROGRAMS += utility_unit-devel
endif

if BUILD_OPT_MODE
  message_tag_unit_opt_SOURCES = message_tag_unit.C
  message_tag_unit_opt_LDFLAGS = $(top_builddir)/src/libtimpi_opt.la
  message_tag_unit_opt_CPPFLAGS = $(CPPFLAGS_OPT) $(AM_CPPFLAGS)
  message_tag_unit_opt_CXXFLAGS = $(CXXFLAGS_OPT)

  packed_range_unit_opt_SOURCES = packed_range_unit.C
  packed_range_unit_opt_LDFLAGS = $(top_builddir)/src/libtimpi_opt.la
  packed_range_unit_opt_CPPFLAGS = $(CPPFLAGS_OPT) $(AM_CPPFLAGS)
  packed_range_unit_opt_CXXFLAGS = $(CXXFLAGS_OPT)

  parallel_sync_unit_opt_SOURCES = parallel_sync_unit.C
  parallel_sync_unit_opt_LDFLAGS = $(top_builddir)/src/libtimpi_opt.la
  parallel_sync_unit_opt_CPPFLAGS = $(CPPFLAGS_OPT) $(AM_CPPFLAGS)
  parallel_sync_unit_opt_CXXFLAGS = $(CXXFLAGS_OPT)

  parallel_unit_opt_SOURCES = parallel_unit.C
  parallel_unit_opt_LDFLAGS = $(top_builddir)/src/libtimpi_opt.la
  parallel_unit_opt_CPPFLAGS = $(CPPFLAGS_OPT) $(AM_CPPFLAGS)
  parallel_unit_opt_CXXFLAGS = $(CXXFLAGS_OPT)

  set_unit_opt_SOURCES = set_unit.C
  set_unit_opt_LDFLAGS = $(top_builddir)/src/libtimpi_opt.la
  set_unit_opt_CPPFLAGS = $(CPPFLAGS_OPT) $(AM_CPPFLAGS)
  set_unit_opt_CXXFLAGS = $(CXXFLAGS_OPT)

  dispatch_to_packed_unit_opt_SOURCES = dispatch_to_packed_unit.C
  dispatch_to_packed_unit_opt_LDFLAGS = $(top_builddir)/src/libtimpi_opt.la
  dispatch_to_packed_unit_opt_CPPFLAGS = $(CPPFLAGS_OPT) $(AM_CPPFLAGS)
  dispatch_to_packed_unit_opt_CXXFLAGS = $(CXXFLAGS_OPT)

  utility_unit_opt_SOURCES = utility_unit.C
  utility_unit_opt_LDFLAGS = $(top_builddir)/src/libtimpi_opt.la
  utility_unit_opt_CPPFLAGS = $(CPPFLAGS_OPT) $(AM_CPPFLAGS)
  utility_unit_opt_CXXFLAGS = $(CXXFLAGS_OPT)

  check_PROGRAMS += message_tag_unit-opt
  check_PROGRAMS += packed_range_unit-opt
  check_PROGRAMS += parallel_sync_unit-opt
  check_PROGRAMS += parallel_unit-opt
  check_PROGRAMS += set_unit-opt
  check_PROGRAMS += dispatch_to_packed_unit-opt
  check_PROGRAMS += utility_unit-opt
endif

if BUILD_OPROF_MODE
  message_tag_unit_oprof_SOURCES = message_tag_unit.C
  message_tag_unit_oprof_LDFLAGS = $(top_builddir)/src/libtimpi_oprof.la
  message_tag_unit_oprof_CPPFLAGS = $(CPPFLAGS_OPROF) $(AM_CPPFLAGS)
  message_tag_unit_oprof_CXXFLAGS = $(CXXFLAGS_OPROF)

  packed_range_unit_oprof_SOURCES = packed_range_unit.C
  packed_range_unit_oprof_LDFLAGS = $(top_builddir)/src/libtimpi_oprof.la
  packed_range_unit_oprof_CPPFLAGS = $(CPPFLAGS_OPROF) $(AM_CPPFLAGS)
  packed_range_unit_oprof_CXXFLAGS = $(CXXFLAGS_OPROF)

  parallel_sync_unit_oprof_SOURCES = parallel_sync_unit.C
  parallel_sync_unit_oprof_LDFLAGS = $(top_builddir)/src/libtimpi_oprof.la
  parallel_sync_unit_oprof_CPPFLAGS = $(CPPFLAGS_OPROF) $(AM_CPPFLAGS)
  parallel_sync_unit_oprof_CXXFLAGS = $(CXXFLAGS_OPROF)

  parallel_unit_oprof_SOURCES = parallel_unit.C
  parallel_unit_oprof_LDFLAGS = $(top_builddir)/src/libtimpi_oprof.la
  parallel_unit_oprof_CPPFLAGS = $(CPPFLAGS_OPROF) $(AM_CPPFLAGS)
  parallel_unit_oprof_CXXFLAGS = $(CXXFLAGS_OPROF)

  set_unit_oprof_SOURCES = set_unit.C
  set_unit_oprof_LDFLAGS = $(top_builddir)/src/libtimpi_oprof.la
  set_unit_oprof_CPPFLAGS = $(CPPFLAGS_OPROF) $(AM_CPPFLAGS)
  set_unit_oprof_CXXFLAGS = $(CXXFLAGS_OPROF)

  dispatch_to_packed_unit_oprof_SOURCES = dispatch_to_packed_unit.C
  dispatch_to_packed_unit_oprof_LDFLAGS = $(top_builddir)/src/libtimpi_oprof.la
  dispatch_to_packed_unit_oprof_CPPFLAGS = $(CPPFLAGS_OPROF) $(AM_CPPFLAGS)
  dispatch_to_packed_unit_oprof_CXXFLAGS = $(CXXFLAGS_OPROF)

  utility_unit_oprof_SOURCES = utility_unit.C
  utility_unit_oprof_LDFLAGS = $(top_builddir)/src/libtimpi_oprof.la
  utility_unit_oprof_CPPFLAGS = $(CPPFLAGS_OPROF) $(AM_CPPFLAGS)
  utility_unit_oprof_CXXFLAGS = $(CXXFLAGS_OPROF)

  check_PROGRAMS += message_tag_unit-oprof
  check_PROGRAMS += packed_range_unit-oprof
  check_PROGRAMS += parallel_sync_unit-oprof
  check_PROGRAMS += parallel_unit-oprof
  check_PROGRAMS += set_unit-oprof
  check_PROGRAMS += dispatch_to_packed_unit-oprof
  check_PROGRAMS += utility_unit-oprof
endif

if BUILD_PROF_MODE
  message_tag_unit_prof_SOURCES = message_tag_unit.C
  message_tag_unit_prof_LDFLAGS = $(top_builddir)/src/libtimpi_prof.la
  message_tag_unit_prof_CPPFLAGS = $(CPPFLAGS_PROF) $(AM_CPPFLAGS)
  message_tag_unit_prof_CXXFLAGS = $(CXXFLAGS_PROF)

  packed_range_unit_prof_SOURCES = packed_range_unit.C
  packed_range_unit_prof_LDFLAGS = $(top_builddir)/src/libtimpi_prof.la
  packed_range_unit_prof_CPPFLAGS = $(CPPFLAGS_PROF) $(AM_CPPFLAGS)
  packed_range_unit_prof_CXXFLAGS = $(CXXFLAGS_PROF)

  parallel_sync_unit_prof_SOURCES = parallel_sync_unit.C
  parallel_sync_unit_prof_LDFLAGS = $(top_builddir)/src/libtimpi_prof.la
  parallel_sync_unit_prof_CPPFLAGS = $(CPPFLAGS_PROF) $(AM_CPPFLAGS)
  parallel_sync_unit_prof_CXXFLAGS = $(CXXFLAGS_PROF)

  parallel_unit_prof_SOURCES = parallel_unit.C
  parallel_unit_prof_LDFLAGS = $(top_builddir)/src/libtimpi_prof.la
  parallel_unit_prof_CPPFLAGS = $(CPPFLAGS_PROF) $(AM_CPPFLAGS)
  parallel_unit_prof_CXXFLAGS = $(CXXFLAGS_PROF)

  set_unit_prof_SOURCES = set_unit.C
  set_unit_prof_LDFLAGS = $(top_builddir)/src/libtimpi_prof.la
  set_unit_prof_CPPFLAGS = $(CPPFLAGS_PROF) $(AM_CPPFLAGS)
  set_unit_prof_CXXFLAGS = $(CXXFLAGS_PROF)

  dispatch_to_packed_unit_prof_SOURCES = dispatch_to_packed_unit.C
  dispatch_to_packed_unit_prof_LDFLAGS = $(top_builddir)/src/libtimpi_prof.la
  dispatch_to_packed_unit_prof_CPPFLAGS = $(CPPFLAGS_PROF) $(AM_CPPFLAGS)
  dispatch_to_packed_unit_prof_CXXFLAGS = $(CXXFLAGS_PROF)

  utility_unit_prof_SOURCES = utility_unit.C
  utility_unit_prof_LDFLAGS = $(top_builddir)/src/libtimpi_prof.la
  utility_unit_prof_CPPFLAGS = $(CPPFLAGS_PROF) $(AM_CPPFLAGS)
  utility_unit_prof_CXXFLAGS = $(CXXFLAGS_PROF)

  check_PROGRAMS += message_tag_unit-prof
  check_PROGRAMS += packed_range_unit-prof
  check_PROGRAMS += parallel_sync_unit-prof
  check_PROGRAMS += parallel_unit-prof
  check_PROGRAMS += set_unit-prof
  check_PROGRAMS += dispatch_to_packed_unit-prof
  check_PROGRAMS += utility_unit-prof
endif

######################################################################
#
# Don't leave code coverage outputs lying around
if CODE_COVERAGE_ENABLED
  CLEANFILES = *.gcda *.gcno
endif

# Required for AX_AM_MACROS
###@INC_AMINCLUDE@
