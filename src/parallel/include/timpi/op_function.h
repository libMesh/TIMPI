// The TIMPI Message-Passing Parallelism Library.
// Copyright (C) 2002-2019 Benjamin S. Kirk, John W. Peterson, Roy H. Stogner

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


#ifndef TIMPI_OP_FUNCTION_H
#define TIMPI_OP_FUNCTION_H

#include "timpi/timpi_config.h"

#include "timpi/semipermanent.h"
#include "timpi/timpi_init.h"

#ifdef TIMPI_HAVE_MPI
#  include "timpi/ignore_warnings.h"
#  include "mpi.h"
#  include "timpi/restore_warnings.h"
#endif // TIMPI_HAVE_MPI

// Boost include if necessary for float128
#ifdef TIMPI_DEFAULT_QUADRUPLE_PRECISION
# include <boost/multiprecision/float128.hpp>
#endif

// C++ includes
#include <functional>
#include <type_traits>



namespace TIMPI
{
#ifdef TIMPI_DEFAULT_QUADRUPLE_PRECISION
# ifdef TIMPI_HAVE_MPI
# define TIMPI_MPI_QUAD_BINARY(funcname) \
inline void \
timpi_mpi_quad_##funcname(void * a, void * b, int * len, MPI_Datatype *) \
{ \
  const int size = *len; \
 \
  TIMPI_DEFAULT_SCALAR_TYPE *in = static_cast<TIMPI_DEFAULT_SCALAR_TYPE*>(a); \
  TIMPI_DEFAULT_SCALAR_TYPE *inout = static_cast<TIMPI_DEFAULT_SCALAR_TYPE*>(b); \
  for (int i=0; i != size; ++i) \
    inout[i] = std::funcname(in[i],inout[i]); \
}

# define TIMPI_MPI_QUAD_LOCATOR(funcname) \
inline void \
timpi_mpi_quad_##funcname##_location(void * a, void * b, int * len, MPI_Datatype *) \
{ \
  const int size = *len; \
 \
  typedef std::pair<TIMPI_DEFAULT_SCALAR_TYPE, int> dtype; \
 \
  dtype *in = static_cast<dtype*>(a); \
  dtype *inout = static_cast<dtype*>(b); \
  for (int i=0; i != size; ++i) \
    { \
      TIMPI_DEFAULT_SCALAR_TYPE old_inout = inout[i].first; \
      inout[i].first = std::funcname(in[i].first,inout[i].first); \
      if (old_inout != inout[i].first) \
        inout[i].second = in[i].second; \
    } \
}


# define TIMPI_MPI_QUAD_BINARY_FUNCTOR(funcname) \
inline void \
timpi_mpi_quad_##funcname(void * a, void * b, int * len, MPI_Datatype *) \
{ \
  const int size = *len; \
 \
  TIMPI_DEFAULT_SCALAR_TYPE *in = static_cast<TIMPI_DEFAULT_SCALAR_TYPE*>(a); \
  TIMPI_DEFAULT_SCALAR_TYPE *inout = static_cast<TIMPI_DEFAULT_SCALAR_TYPE*>(b); \
  for (int i=0; i != size; ++i) \
    inout[i] = std::funcname<TIMPI_DEFAULT_SCALAR_TYPE>()(in[i],inout[i]); \
}


TIMPI_MPI_QUAD_BINARY(max)
TIMPI_MPI_QUAD_BINARY(min)
TIMPI_MPI_QUAD_LOCATOR(max)
TIMPI_MPI_QUAD_LOCATOR(min)
TIMPI_MPI_QUAD_BINARY_FUNCTOR(plus)
TIMPI_MPI_QUAD_BINARY_FUNCTOR(multiplies)

# endif // TIMPI_HAVE_MPI
#endif // TIMPI_DEFAULT_QUADRUPLE_PRECISION


//-------------------------------------------------------------------

// Templated helper class to be used with static_assert.
template<typename T>
struct opfunction_dependent_false : std::false_type
{};

/**
 * Templated class to provide the appropriate MPI reduction operations
 * for use with built-in C types or simple C++ constructions.
 *
 * More complicated data types may need to provide a pointer-to-T so
 * that we can use MPI_Address without constructing a new T.
 */
template <typename T>
class OpFunction
{
  // Get a slightly better compiler diagnostic if we have C++11
  static_assert(opfunction_dependent_false<T>::value,
                "Only specializations of OpFunction may be used, did you forget to include a header file (e.g. parallel_algebra.h)?");

  /*
   * The unspecialized class defines none of these functions;
   * specializations will need to define any functions that need to be
   * usable.
   *
   * Most specializations will just return MPI_MIN, etc, but we'll use
   * a whitelist rather than a default implementation, so that any
   * attempt to perform a reduction on an unspecialized type will be a
   * compile-time rather than a run-time failure.
   */
  // static MPI_Op max();
  // static MPI_Op min();
  // static MPI_Op sum();
  // static MPI_Op product();
  // static MPI_Op logical_and();
  // static MPI_Op bitwise_and();
  // static MPI_Op logical_or();
  // static MPI_Op bitwise_or();
  // static MPI_Op logical_xor();
  // static MPI_Op bitwise_xor();
  // static MPI_Op max_loc();
  // static MPI_Op min_loc();
};



// ------------------------------------------------------------
// Declare OpFunction specializations for C++ built-in types

#ifdef TIMPI_HAVE_MPI

#define TIMPI_PARALLEL_INTEGER_OPS(cxxtype)          \
  template<>                                            \
  class OpFunction<cxxtype>                             \
  {                                                     \
  public:                                               \
    static MPI_Op max()          { return MPI_MAX; }    \
    static MPI_Op min()          { return MPI_MIN; }    \
    static MPI_Op sum()          { return MPI_SUM; }    \
    static MPI_Op product()      { return MPI_PROD; }   \
    static MPI_Op logical_and()  { return MPI_LAND; }   \
    static MPI_Op bitwise_and()  { return MPI_BAND; }   \
    static MPI_Op logical_or()   { return MPI_LOR; }    \
    static MPI_Op bitwise_or()   { return MPI_BOR; }    \
    static MPI_Op logical_xor()  { return MPI_LXOR; }   \
    static MPI_Op bitwise_xor()  { return MPI_BXOR; }   \
    static MPI_Op max_location() { return MPI_MAXLOC; } \
    static MPI_Op min_location() { return MPI_MINLOC; } \
  }

#define TIMPI_PARALLEL_FLOAT_OPS(cxxtype)            \
  template<>                                            \
  class OpFunction<cxxtype>                             \
  {                                                     \
  public:                                               \
    static MPI_Op max()          { return MPI_MAX; }    \
    static MPI_Op min()          { return MPI_MIN; }    \
    static MPI_Op sum()          { return MPI_SUM; }    \
    static MPI_Op product()      { return MPI_PROD; }   \
    static MPI_Op max_location() { return MPI_MAXLOC; } \
    static MPI_Op min_location() { return MPI_MINLOC; } \
  }

#else

#define TIMPI_PARALLEL_INTEGER_OPS(cxxtype)  \
  template<>                                    \
  class OpFunction<cxxtype>                     \
  {                                             \
  }

#define TIMPI_PARALLEL_FLOAT_OPS(cxxtype)    \
  template<>                                    \
  class OpFunction<cxxtype>                     \
  {                                             \
  }

#endif

TIMPI_PARALLEL_INTEGER_OPS(char);
TIMPI_PARALLEL_INTEGER_OPS(signed char);
TIMPI_PARALLEL_INTEGER_OPS(unsigned char);
TIMPI_PARALLEL_INTEGER_OPS(short int);
TIMPI_PARALLEL_INTEGER_OPS(unsigned short int);
TIMPI_PARALLEL_INTEGER_OPS(int);
TIMPI_PARALLEL_INTEGER_OPS(unsigned int);
TIMPI_PARALLEL_INTEGER_OPS(long);
TIMPI_PARALLEL_INTEGER_OPS(long long);
TIMPI_PARALLEL_INTEGER_OPS(unsigned long);
TIMPI_PARALLEL_INTEGER_OPS(unsigned long long);

TIMPI_PARALLEL_FLOAT_OPS(float);
TIMPI_PARALLEL_FLOAT_OPS(double);
TIMPI_PARALLEL_FLOAT_OPS(long double);

#ifdef TIMPI_HAVE_MPI
// Helper class to avoid leaking MPI_Op when TIMPI exits
class FreeOp : public SemiPermanent
{
public:
  FreeOp(MPI_Op * op) : _op(op) {}
  virtual ~FreeOp() override {
    MPI_Op_free(_op);
  }
private:
  MPI_Op * _op;
};
#endif

#define TIMPI_MPI_OPFUNCTION(mpiname, funcname) \
  static MPI_Op mpiname() { \
    static MPI_Op TIMPI_MPI_##mpiname = MPI_OP_NULL; \
    if (TIMPI_MPI_##mpiname == MPI_OP_NULL) \
      { \
        timpi_call_mpi \
          (MPI_Op_create(timpi_mpi_##funcname, true, &TIMPI_MPI_##mpiname)); \
        TIMPIInit::add_semipermanent(std::make_unique<FreeOp>(&TIMPI_MPI_##mpiname)); \
      } \
    return TIMPI_MPI_##mpiname;  \
  }

#ifdef TIMPI_DEFAULT_QUADRUPLE_PRECISION
# ifdef TIMPI_HAVE_MPI
  template<>
  class OpFunction<TIMPI_DEFAULT_SCALAR_TYPE>
  {
  public:
    TIMPI_MPI_OPFUNCTION(max, quad_max)
    TIMPI_MPI_OPFUNCTION(min, quad_min)
    TIMPI_MPI_OPFUNCTION(sum, quad_plus)
    TIMPI_MPI_OPFUNCTION(product, quad_multiplies)

    TIMPI_MPI_OPFUNCTION(max_location, quad_max_location)
    TIMPI_MPI_OPFUNCTION(min_location, quad_min_location)
  };

# else
  TIMPI_PARALLEL_FLOAT_OPS(TIMPI_DEFAULT_SCALAR_TYPE);
# endif
#endif // TIMPI_DEFAULT_QUADRUPLE_PRECISION

#ifdef TIMPI_HAVE_MPI

# define TIMPI_MPI_PAIR_BINARY(funcname) \
static inline void \
timpi_mpi_pair_##funcname(void * a, void * b, int * len, MPI_Datatype *) \
{ \
  const int size = *len; \
 \
  const std::pair<T,U> * in = static_cast<std::pair<T,U> *>(a); \
  std::pair<T,U> * inout = static_cast<std::pair<T,U> *>(b); \
  for (int i=0; i != size; ++i) \
    { \
      inout[i].first = std::funcname(in[i].first,inout[i].first); \
      inout[i].second = std::funcname(in[i].second,inout[i].second); \
    } \
}

# define TIMPI_MPI_PAIR_LOCATOR(funcname) \
static inline void \
timpi_mpi_pair_##funcname##_location(void * a, void * b, int * len, MPI_Datatype *) \
{ \
  const int size = *len; \
 \
  typedef std::pair<std::pair<T,U>, int> dtype; \
 \
  dtype *in = static_cast<dtype*>(a); \
  dtype *inout = static_cast<dtype*>(b); \
  for (int i=0; i != size; ++i) \
    { \
      std::pair<T,U> old_inout = inout[i].first; \
      inout[i].first.first  = std::funcname(in[i].first.first, inout[i].first.first); \
      inout[i].first.second = std::funcname(in[i].first.second,inout[i].first.second); \
      if (old_inout != inout[i].first) \
        inout[i].second = in[i].second; \
    } \
}


# define TIMPI_MPI_PAIR_BINARY_FUNCTOR(funcname) \
static inline void \
timpi_mpi_pair_##funcname(void * a, void * b, int * len, MPI_Datatype *) \
{ \
  const int size = *len; \
 \
  const std::pair<T,U> * in = static_cast<std::pair<T,U> *>(a); \
  std::pair<T,U> * inout = static_cast<std::pair<T,U> *>(b); \
  for (int i=0; i != size; ++i) \
    { \
      inout[i].first  = std::funcname<T>()(in[i].first, inout[i].first); \
      inout[i].second = std::funcname<T>()(in[i].second,inout[i].second); \
    } \
}


  template<typename T, typename U>
  class OpFunction<std::pair<T,U>>
  {
    TIMPI_MPI_PAIR_BINARY(max)
    TIMPI_MPI_PAIR_BINARY(min)
    TIMPI_MPI_PAIR_LOCATOR(max)
    TIMPI_MPI_PAIR_LOCATOR(min)
    TIMPI_MPI_PAIR_BINARY_FUNCTOR(plus)
    TIMPI_MPI_PAIR_BINARY_FUNCTOR(multiplies)

  public:
    TIMPI_MPI_OPFUNCTION(max, pair_max)
    TIMPI_MPI_OPFUNCTION(min, pair_min)
    TIMPI_MPI_OPFUNCTION(sum, pair_plus)
    TIMPI_MPI_OPFUNCTION(product, pair_multiplies)

    TIMPI_MPI_OPFUNCTION(max_location, pair_max_location)
    TIMPI_MPI_OPFUNCTION(min_location, pair_min_location)
  };
# else // TIMPI_HAVE_MPI
  template<typename T, typename U>
  class OpFunction<std::pair<T,U>> {};
#endif

} // namespace TIMPI

#endif // TIMPI_OP_FUNCTION_H
