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


#ifndef TIMPI_STANDARD_TYPE_H
#define TIMPI_STANDARD_TYPE_H

// TIMPI includes
#include "timpi/data_type.h"
#include "timpi/timpi_config.h"
#include "timpi/standard_type_forward.h"
#include "timpi/semipermanent.h"

// C/C++ includes
#ifdef TIMPI_HAVE_MPI
#  include "timpi/ignore_warnings.h"
#  include "mpi.h"
#  include "timpi/restore_warnings.h"
#endif // TIMPI_HAVE_MPI

// Boost include if necessary for float128
#ifdef TIMPI_DEFAULT_QUADRUPLE_PRECISION
# include <boost/multiprecision/float128.hpp>
#endif

#include <array>
#include <complex>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace TIMPI
{

//-------------------------------------------------------------------

// Templated helper class to be used with static_assert.
template<typename T>
struct standardtype_dependent_false : std::false_type
{};

/**
 * Templated class to provide the appropriate MPI datatype for use with built-in
 * C types or simple C++ constructions. Note that the unspecialized class
 * inherits from \p NotADataType. Users defining their own \p StandardType and
 * defining their own MPI dataype will want to inherit from \p DataType, which
 * includes an \p MPI_Datatype member (when we have MPI). A user <em>may
 * also</em> want to define a \p StandardType that inherits from \p NotADataType
 * if they are defining a \p Packing specialization for the same type \p T. This
 * will enable them to call non-packed-range code with their dynamically sized
 * data and have automatic dispatch to packed range methods when required. Note
 * that a user wishing to make use of automatic dispatch to packed range methods
 * will need to define a public constructor in order for their code to
 * compile. Normal MPI-typeable \p StandardType specializations will obviously
 * need to also define public constructors.
 *
 * More complicated data types may need to provide a pointer-to-T so
 * that we can use MPI_Address without constructing a new T.
 */
template <typename T, typename Enable>
class StandardType : public NotADataType
{
  /*
   * The unspecialized class is useless, so we make its constructor
   * private to catch mistakes at compile-time rather than link-time.
   * Specializations should have a public constructor of the same
   * form.
   */
private:
  StandardType(const T * example = nullptr);
};


/*
 * Template metaprogramming to make build_standard_type work nicely
 * with nested containers
 */
template <typename T>
struct InnermostType
{
  typedef T type;
};


template <typename T, typename A>
struct InnermostType<std::vector<T, A>>
{
  typedef typename InnermostType<T>::type type;
};


template <typename T, typename A>
struct InnermostType<std::list<T, A>>
{
  typedef typename InnermostType<T>::type type;
};


template <typename K, typename T, typename C, typename A>
struct InnermostType<std::map<K, T, C, A>>
{
  typedef typename InnermostType<std::pair<const K,T>>::type type;
};


template <typename K, typename T, typename C, typename A>
struct InnermostType<std::multimap<K, T, C, A>>
{
  typedef typename InnermostType<std::pair<const K,T>>::type type;
};


template <typename T, typename C, typename A>
struct InnermostType<std::multiset<T, C, A>>
{
  typedef typename InnermostType<T>::type type;
};


template <typename T, typename C, typename A>
struct InnermostType<std::set<T, C, A>>
{
  typedef typename InnermostType<T>::type type;
};


template <typename K, typename T, typename C, typename A>
struct InnermostType<std::unordered_map<K, T, C, A>>
{
  typedef typename InnermostType<std::pair<const K,T>>::type type;
};


template <typename K, typename T, typename C, typename A>
struct InnermostType<std::unordered_multimap<K, T, C, A>>
{
  typedef typename InnermostType<std::pair<const K,T>>::type type;
};


template <typename T, typename C, typename A>
struct InnermostType<std::unordered_multiset<T, C, A>>
{
  typedef typename InnermostType<T>::type type;
};


template <typename T, typename C, typename A>
struct InnermostType<std::unordered_set<T, C, A>>
{
  typedef typename InnermostType<T>::type type;
};


/*
 * Returns a StandardType suitable for use with the example data.
 */
template <typename T>
StandardType<T> build_standard_type(const T * example = nullptr)
{
  StandardType<T> returnval(example);
  return returnval;
}



/*
 * Returns a StandardType suitable for use with the data in the
 * example container.
 */
template <typename T, typename A>
StandardType<typename InnermostType<T>::type>
build_standard_type(const std::vector<T, A> * example = nullptr)
{
  const T * inner_example = (example && !example->empty()) ? &(*example)[0] : nullptr;
  return build_standard_type(inner_example);
}



// ------------------------------------------------------------
// Declare StandardType specializations for C++ built-in types

#ifdef TIMPI_HAVE_MPI

#define TIMPI_STANDARD_TYPE(cxxtype,mpitype)                      \
  template<>                                                         \
  class StandardType<cxxtype> : public DataType                      \
  {                                                                  \
  public:                                                            \
    explicit                                                         \
      StandardType(const cxxtype * = nullptr) : DataType(mpitype) {} \
                                                                     \
    static const bool is_fixed_type = true;                          \
  }

#else

#define TIMPI_STANDARD_TYPE(cxxtype,mpitype)               \
  template<>                                                  \
  class StandardType<cxxtype> : public DataType               \
  {                                                           \
  public:                                                     \
    explicit                                                  \
      StandardType(const cxxtype * = nullptr) : DataType() {} \
                                                              \
    static const bool is_fixed_type = true;                   \
  }

#endif

TIMPI_STANDARD_TYPE(char,MPI_CHAR);
TIMPI_STANDARD_TYPE(signed char,MPI_SIGNED_CHAR);
TIMPI_STANDARD_TYPE(unsigned char,MPI_UNSIGNED_CHAR);
TIMPI_STANDARD_TYPE(short int,MPI_SHORT);
TIMPI_STANDARD_TYPE(unsigned short int,MPI_UNSIGNED_SHORT);
TIMPI_STANDARD_TYPE(int,MPI_INT);
TIMPI_STANDARD_TYPE(unsigned int,MPI_UNSIGNED);
TIMPI_STANDARD_TYPE(long,MPI_LONG);
TIMPI_STANDARD_TYPE(long long,MPI_LONG_LONG_INT);
TIMPI_STANDARD_TYPE(unsigned long,MPI_UNSIGNED_LONG);
TIMPI_STANDARD_TYPE(unsigned long long,MPI_UNSIGNED_LONG_LONG);
TIMPI_STANDARD_TYPE(float,MPI_FLOAT);
TIMPI_STANDARD_TYPE(double,MPI_DOUBLE);
TIMPI_STANDARD_TYPE(long double,MPI_LONG_DOUBLE);

#ifdef TIMPI_HAVE_MPI

// For non-default data types, we like to be able to construct them on
// the fly, but we don't like to repeatedly destroy and reconstruct
// them and we don't like to leak them, so let's keep them until TIMPI
// exits via SemiPermanent
class ManageType : public SemiPermanent
{
public:
  ManageType(data_type uncommitted_type) :
    _type(uncommitted_type) {
    MPI_Type_commit (&uncommitted_type);
  }

  virtual ~ManageType() override {
    MPI_Type_free(&_type);
  }
private:
  data_type _type;
};


// Quad and float128 types aren't standard C++, so only work with them
// if configure and PETSc encapsulated the non-standard issues.
# ifdef TIMPI_DEFAULT_QUADRUPLE_PRECISION
  template<>
  class StandardType<TIMPI_DEFAULT_SCALAR_TYPE> : public DataType
  {
  public:
    explicit
      StandardType(const TIMPI_DEFAULT_SCALAR_TYPE * = nullptr) : DataType() {
        static data_type static_type = MPI_DATATYPE_NULL;
        if (static_type == MPI_DATATYPE_NULL)
          {
            timpi_call_mpi(MPI_Type_contiguous(2, MPI_DOUBLE, &static_type));
            SemiPermanent::add
              (std::make_unique<ManageType>(static_type));
          }
        _datatype = static_type;
      }

    StandardType(const StandardType<TIMPI_DEFAULT_SCALAR_TYPE> & t) : DataType() {
      _datatype = t._datatype;
    }

    StandardType & operator=(StandardType & t)
    {
      _datatype = t._datatype;
      return *this;
    }

    static const bool is_fixed_type = true;
  };
# endif
#else
# ifdef TIMPI_DEFAULT_QUADRUPLE_PRECISION
  TIMPI_STANDARD_TYPE(TIMPI_DEFAULT_SCALAR_TYPE,);
# endif
#endif

// using remove_const here so our packing code can see a
// `StandardType<pair<const K,T>>::is_fixed_type` and infer that it
// can do memcpy on them
template<typename T1, typename T2>
class StandardType<std::pair<T1, T2>,
                   typename std::enable_if<
                     StandardType<typename std::remove_const<T1>::type>::is_fixed_type &&
                     StandardType<T2>::is_fixed_type>::type> : public DataType
{
public:
  explicit
  StandardType(const std::pair<T1, T2> * example = nullptr)
    : DataType()
  {
#ifdef TIMPI_HAVE_MPI
    static data_type static_type = MPI_DATATYPE_NULL;
    if (static_type == MPI_DATATYPE_NULL)
      {
        // We need an example for MPI_Address to use
        static const std::pair<T1, T2> p;
        if (!example)
          example = &p;

        // Get the sub-data-types, and make sure they live long enough
        // to construct the derived type
        StandardType<typename std::remove_const<T1>::type>
          d1(const_cast<typename std::remove_const<T1>::type *>
               (&example->first));
        StandardType<T2> d2(&example->second);

        MPI_Datatype types[] = { (data_type)d1, (data_type)d2 };
        int blocklengths[] = {1,1};
        MPI_Aint displs[2], start;

        timpi_call_mpi
          (MPI_Get_address (const_cast<std::pair<T1,T2> *>(example),
                            &start));
        timpi_call_mpi
          (MPI_Get_address (const_cast<T1*>(&example->first),
                            &displs[0]));
        timpi_call_mpi
          (MPI_Get_address (const_cast<T2*>(&example->second),
                            &displs[1]));
        displs[0] -= start;
        displs[1] -= start;

        // create a prototype structure
        MPI_Datatype tmptype;
        timpi_call_mpi
          (MPI_Type_create_struct (2, blocklengths, displs, types,
                                   &tmptype));
        timpi_call_mpi
          (MPI_Type_commit (&tmptype));

        // resize the structure type to account for padding, if any
        timpi_call_mpi
          (MPI_Type_create_resized (tmptype, 0,
                                    sizeof(std::pair<T1,T2>),
                                    &static_type));
        timpi_call_mpi
          (MPI_Type_free (&tmptype));

        SemiPermanent::add
          (std::make_unique<ManageType>(static_type));
      }
    _datatype = static_type;
#else
    timpi_ignore(example);
#endif // TIMPI_HAVE_MPI
  }

  StandardType(const StandardType<std::pair<T1, T2>> & t)
    : DataType()
  {
    _datatype = t._datatype;
  }

  StandardType & operator=(StandardType & t)
  {
    _datatype = t._datatype;
    return *this;
  }

  static const bool is_fixed_type = true;
};



template<typename T, std::size_t N>
class StandardType<std::array<T, N>,
                   typename std::enable_if<
                     StandardType<T>::is_fixed_type>::type> : public DataType
{
public:
  explicit
  StandardType(const std::array<T, N> * example = nullptr)
    : DataType()
  {
#ifdef TIMPI_HAVE_MPI
    static data_type static_type = MPI_DATATYPE_NULL;
    if (static_type == MPI_DATATYPE_NULL)
      {
        // We need an example for MPI_Address to use
        std::array<T, N> * ex;
        std::unique_ptr<std::array<T, N>> temp;
        if (example)
          ex = const_cast<std::array<T, N> *>(example);
        else
          {
            temp.reset(new std::array<T, N>());
            ex = temp.get();
          }

        static_assert(N > 0, "Zero-length std::array is not supported by TIMPI");
        StandardType<T> T_type(&((*ex)[0]));

        int blocklength = N;
        MPI_Aint displs, start;
        MPI_Datatype tmptype, type = T_type;

        timpi_call_mpi
          (MPI_Get_address (ex, &start));
        timpi_call_mpi
          (MPI_Get_address (&((*ex)[0]), &displs));

        // subtract off offset to first value from the beginning of the structure
        displs -= start;

        // create a prototype structure
        timpi_call_mpi
          (MPI_Type_create_struct (1, &blocklength, &displs, &type,
                                   &tmptype));
        timpi_call_mpi
          (MPI_Type_commit (&tmptype));

        // resize the structure type to account for padding, if any
        timpi_call_mpi
          (MPI_Type_create_resized (tmptype, 0, sizeof(std::array<T,N>),
                                    &static_type));

        timpi_call_mpi
          (MPI_Type_free (&tmptype));

        SemiPermanent::add
          (std::make_unique<ManageType>(static_type));
      }
    _datatype = static_type;
#else // #ifdef TIMPI_HAVE_MPI
    timpi_ignore(example);
#endif
  }

  StandardType(const StandardType<std::array<T, N>> & t)
    : DataType()
  {
    _datatype = t._datatype;
  }

  StandardType & operator=(StandardType & t)
  {
    _datatype = t._datatype;
    return *this;
  }

  static const bool is_fixed_type = true;
};


// Helper functions for creating type/displacement arrays for tuples
//
// These are classes since we can't partially specialize functions
template<std::size_t n_minus_i>
struct BuildStandardTypeVector
{
  template<typename... Types>
  static void build(std::vector<std::unique_ptr<DataType>> & out_vec,
                    const std::tuple<Types...> & example);
};

template <>
struct BuildStandardTypeVector<0>
{
  template<typename... Types>
  static void build(std::vector<std::unique_ptr<DataType>> & /*out_vec*/,
                    const std::tuple<Types...> & /*example*/) {}
};

template<std::size_t n_minus_i>
template<typename... Types>
void BuildStandardTypeVector<n_minus_i>::build
  (std::vector<std::unique_ptr<DataType>> & out_vec,
   const std::tuple<Types...> & example)
{
  typedef typename
    std::tuple_element<sizeof...(Types)-n_minus_i, std::tuple<Types...>>::type
    ith_type;

  out_vec.emplace_back
    (std::make_unique<StandardType<ith_type>>
     (&std::get<sizeof...(Types)-n_minus_i>(example)));

  BuildStandardTypeVector<n_minus_i-1>::build(out_vec, example);
}


template<std::size_t n_minus_i>
struct FillDisplacementArray
{
  template <typename OutArray, class... Types>
  static void fill(OutArray & out,
                   const std::tuple<Types...> & example);
};

template<>
struct FillDisplacementArray<0>
{
  template <typename OutArray, typename... Types>
  static void fill(OutArray & /*out*/,
                   const std::tuple<Types...> & /*example*/) {}
};


template<std::size_t n_minus_i>
template<typename OutArray, typename... Types>
void FillDisplacementArray<n_minus_i>::fill
  (OutArray & out_vec,
   const std::tuple<Types...> & example)
{
  timpi_call_mpi
    (MPI_Get_address
      (&std::get<sizeof...(Types)-n_minus_i>(example),
       &out_vec[sizeof...(Types)-n_minus_i]));

  FillDisplacementArray<n_minus_i-1>::fill(out_vec, example);
}


template <typename Head, typename... Tail>
struct CheckAllFixedTypes
{
  static const bool is_fixed_type = StandardType<Head>::is_fixed_type &&
                                    CheckAllFixedTypes<Tail...>::is_fixed_type;
};

template <typename Head>
struct CheckAllFixedTypes<Head>
{
  static const bool is_fixed_type = StandardType<Head>::is_fixed_type;
};

template<typename... Types>
class StandardType<std::tuple<Types...>,
                   typename std::enable_if<
                     CheckAllFixedTypes<Types...>::is_fixed_type>::type> : public DataType
{
public:
  explicit
  StandardType(const std::tuple<Types...> * example = nullptr)
    : DataType()
  {
#ifdef TIMPI_HAVE_MPI
    static data_type static_type = MPI_DATATYPE_NULL;
    if (static_type == MPI_DATATYPE_NULL)
      {
        // We need an example for MPI_Address to use
        static const std::tuple<Types...> t;
        if (!example)
          example = &t;

        MPI_Aint start;

        timpi_call_mpi
          (MPI_Get_address (example, &start));

        const std::size_t tuplesize = sizeof...(Types);

        std::vector<std::unique_ptr<DataType>> subtypes;
        BuildStandardTypeVector<sizeof...(Types)>::build(subtypes, *example);

        std::array<MPI_Aint, sizeof...(Types)> displs;
        FillDisplacementArray<sizeof...(Types)>::fill(displs, *example);

        std::array<MPI_Datatype, sizeof...(Types)> types;
        std::array<int, sizeof...(Types)> blocklengths;

        for (std::size_t i = 0; i != tuplesize; ++i)
        {
          displs[i] -= start;
          types[i] = (data_type)(*subtypes[i]);
          blocklengths[i] = 1;
        }

        // create a prototype structure
        MPI_Datatype tmptype;
        timpi_call_mpi
          (MPI_Type_create_struct (tuplesize, blocklengths.data(), displs.data(), types.data(),
                                   &tmptype));
        timpi_call_mpi
          (MPI_Type_commit (&tmptype));

        // resize the structure type to account for padding, if any
        timpi_call_mpi
          (MPI_Type_create_resized (tmptype, 0,
                                    sizeof(std::tuple<Types...>),
                                    &static_type));
        timpi_call_mpi
          (MPI_Type_free (&tmptype));

        SemiPermanent::add
          (std::make_unique<ManageType>(static_type));
      }
    _datatype = static_type;
#else // #ifdef TIMPI_HAVE_MPI
    timpi_ignore(example);
#endif // TIMPI_HAVE_MPI
  }

  StandardType(const StandardType<std::tuple<Types...>> & t)
    : DataType()
  {
    _datatype = t._datatype;
  }

  StandardType & operator=(StandardType & t)
  {
    _datatype = t._datatype;
    return *this;
  }

  static const bool is_fixed_type = CheckAllFixedTypes<Types...>::is_fixed_type;
};


template<typename T>
class StandardType<std::complex<T>> : public DataType
{
public:
  explicit
  StandardType(const std::complex<T> * /*example*/ = nullptr) :
    DataType(StandardType<T>(nullptr), 2) {}

  ~StandardType() { this->free(); }

  static const bool is_fixed_type = StandardType<T>::is_fixed_type;
};

} // namespace TIMPI

#endif // TIMPI_STANDARD_TYPE_H
