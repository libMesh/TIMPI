
AM_CPPFLAGS  = $(timpi_optional_INCLUDES)
AM_CPPFLAGS += -I$(top_srcdir)/src/algorithms/include
AM_CPPFLAGS += -I$(top_srcdir)/src/parallel/include
AM_CPPFLAGS += -I$(top_srcdir)/src/utilities/include
AM_CPPFLAGS += -I$(top_builddir)/src/utilities/include #timpi_version.h

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

  dispatch_to_packed_unit_dbg_SOURCES = dispatch_to_packed_unit.C
  dispatch_to_packed_unit_dbg_LDFLAGS = $(top_builddir)/src/libtimpi_dbg.la
  dispatch_to_packed_unit_dbg_CPPFLAGS = $(CPPFLAGS_DBG) $(AM_CPPFLAGS)
  dispatch_to_packed_unit_dbg_CXXFLAGS = $(CXXFLAGS_DBG)

  check_PROGRAMS += message_tag_unit-dbg
  check_PROGRAMS += packed_range_unit-dbg
  check_PROGRAMS += parallel_sync_unit-dbg
  check_PROGRAMS += parallel_unit-dbg
  check_PROGRAMS += dispatch_to_packed_unit-dbg
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

  dispatch_to_packed_unit_devel_SOURCES = dispatch_to_packed_unit.C
  dispatch_to_packed_unit_devel_LDFLAGS = $(top_builddir)/src/libtimpi_devel.la
  dispatch_to_packed_unit_devel_CPPFLAGS = $(CPPFLAGS_DEVEL) $(AM_CPPFLAGS)
  dispatch_to_packed_unit_devel_CXXFLAGS = $(CXXFLAGS_DEVEL)

  check_PROGRAMS += message_tag_unit-devel
  check_PROGRAMS += packed_range_unit-devel
  check_PROGRAMS += parallel_sync_unit-devel
  check_PROGRAMS += parallel_unit-devel
  check_PROGRAMS += dispatch_to_packed_unit-devel
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

  dispatch_to_packed_unit_opt_SOURCES = dispatch_to_packed_unit.C
  dispatch_to_packed_unit_opt_LDFLAGS = $(top_builddir)/src/libtimpi_opt.la
  dispatch_to_packed_unit_opt_CPPFLAGS = $(CPPFLAGS_OPT) $(AM_CPPFLAGS)
  dispatch_to_packed_unit_opt_CXXFLAGS = $(CXXFLAGS_OPT)

  check_PROGRAMS += message_tag_unit-opt
  check_PROGRAMS += packed_range_unit-opt
  check_PROGRAMS += parallel_sync_unit-opt
  check_PROGRAMS += parallel_unit-opt
  check_PROGRAMS += dispatch_to_packed_unit-opt
endif

# Required for AX_AM_MACROS
###@INC_AMINCLUDE@
