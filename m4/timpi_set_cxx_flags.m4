# -------------------------------------------------------------
# Set C++ compiler flags to their default values. They will be
# modified according to other options in later steps of
# configuration
#
# CXXFLAGS_OPT    : flags for optimized mode
# CXXFLAGS_DEVEL  : flags for development mode
# CXXFLAGS_DBG    : flags for debug mode
# CPPFLAGS_OPT    : preprocessor flags for optimized mode
# CPPFLAGS_DEVEL  : preprocessor flags for development mode
# CPPFLAGS_DBG    : preprocessor flags for debug mode
# PROFILING_FLAGS : flags to enable code profiling
# ASSEMBLY_FLAGS  : flags to enable assembly language output
# WERROR_FLAGS    : flags to turn compiler warnings into errors
# PARANOID_FLAGS  : flags to turn on many more compiler warnings
#
# Usage: SET_CXX_FLAGS
#
# (Note the CXXFLAGS and the CPPFLAGS used for further tests may
#  be augmented)
# -------------------------------------------------------------
AC_DEFUN([TIMPI_SET_CXX_FLAGS],
[
  ACSM_SET_GLIBCXX_DEBUG_FLAGS
  ACSM_SET_CXX_FLAGS

  #-----------------------------------------------------
  # Add compiler flags to respect IEEE FPE behavior.
  # This probably doesn't affect TIMPI directly but I'd
  # hate to be surprised by it later.
  #-----------------------------------------------------
  ACSM_SET_FPE_SAFETY_FLAGS

  CXXFLAGS_OPT="$ACSM_CXXFLAGS_OPT"
  CXXFLAGS_DEVEL="$ACSM_CXXFLAGS_DEVEL"
  CXXFLAGS_DBG="$ACSM_CXXFLAGS_DBG"

  CPPFLAGS_OPT="$ACSM_CPPFLAGS_OPT"
  CPPFLAGS_DEVEL="$ACSM_CPPFLAGS_DEVEL"
  CPPFLAGS_DBG="$ACSM_CPPFLAGS_DBG"

  ASSEMBLY_FLAGS="$ACSM_ASSEMBLY_FLAGS"
  NODEPRECATEDFLAG="$ACSM_NODEPRECATEDFLAG"
  OPROFILE_FLAGS="$ACSM_OPROFILE_FLAGS"
  PARANOID_FLAGS="$ACSM_PARANOID_FLAGS"
  PROFILING_FLAGS="$ACSM_PROFILING_FLAGS"
  RPATHFLAG="$ACSM_RPATHFLAG"
  WERROR_FLAGS="$ACSM_WERROR_FLAGS"
]
)
