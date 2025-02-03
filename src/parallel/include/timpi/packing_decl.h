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


#ifndef TIMPI_PACKING_DECL_H
#define TIMPI_PACKING_DECL_H

// TIMPI Includes
#include "timpi/packing_forward.h"
#include "timpi/standard_type.h"

// C++ includes
#include <array>
#include <list>
#include <map>
#include <set>
#include <tuple>
#include <type_traits> // enable_if
#include <unordered_map>
#include <unordered_set>
#include <utility>     // pair
#include <vector>


// FIXME: This *should* be in TIMPI namespace but we have libMesh
// users which already partially specialized it
namespace libMesh
{

namespace Parallel
{

template <typename T, typename Enable>
class Packing;

// Idiom taken from https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Member_Detector
//
// We need this in _decl because we're using it for SFINAE in
// template declarations
template <typename T>
class Has_buffer_type
{
  using Yes = char[2];
  using No = char[1];

  struct Fallback {
    struct buffer_type {};
  };
  struct Derived : T, Fallback {};

  template <typename U> static Yes &test(U *);

  // this template must be more specialized in general than the Yes version because it involves a
  // type-dependent expression...?
  template <typename U> static No &test(typename U::buffer_type *);

public:
  static constexpr bool value = sizeof(test<Derived>(nullptr)) == sizeof(Yes);
};


// specialization for std::pair
template <typename T1, typename T2>
struct PairHasPacking
{
  typedef typename std::remove_const<T1>::type cT1;

  static const bool value =
    !TIMPI::StandardType<std::pair<cT1, T2>>::is_fixed_type &&
    (Has_buffer_type<Packing<cT1>>::value ||
     TIMPI::StandardType<cT1>::is_fixed_type) &&
    (Has_buffer_type<Packing<T2>>::value ||
     TIMPI::StandardType<T2>::is_fixed_type);
};


template <typename T1, typename T2>
class Packing<std::pair<T1, T2>,
              typename std::enable_if<PairHasPacking<T1,T2>::value>::type>;


// specializations for std::tuple

template <typename... Types>
struct TupleHasPacking;

template <>
struct TupleHasPacking<>
{
  static const bool value = true;
};

template <typename T, typename... Types>
struct TupleHasPacking<T, Types...>
{
  static const bool value =
    !TIMPI::StandardType<std::tuple<T, Types...>>::is_fixed_type &&
    (Has_buffer_type<Packing<T>>::value ||
     TIMPI::StandardType<T>::is_fixed_type) &&
    (TupleHasPacking<Types...>::value ||
     TIMPI::StandardType<std::tuple<Types...>>::is_fixed_type);
};


template <typename Enable>
class Packing<std::tuple<>, Enable>;

template <typename T, typename... Types>
class Packing<std::tuple<T, Types...>,
              typename std::enable_if<TupleHasPacking<T, Types...>::value>::type>;


// specialization for std::array
template <typename T, std::size_t N>
class Packing<std::array<T, N>,
              typename std::enable_if<Has_buffer_type<Packing<T>>::value>::type>;


// helper class for any homogeneous-type variable-size containers
// which define the usual iterator ranges, value_type, etc.
template <typename Container>
class PackingRange;


#define TIMPI_DECL_PACKING_RANGE_SUBCLASS(Container) \
class Packing<Container,                             \
              typename std::enable_if<Has_buffer_type<Packing<typename Container::value_type>>::value || \
                                      TIMPI::StandardType<typename Container::value_type>::is_fixed_type>::type>

#define TIMPI_P_COMMA ,

template <typename T, typename A>
TIMPI_DECL_PACKING_RANGE_SUBCLASS(std::vector<T TIMPI_P_COMMA A>);

template <typename T, typename A>
TIMPI_DECL_PACKING_RANGE_SUBCLASS(std::list<T TIMPI_P_COMMA A>);

template <typename K, typename T, typename C, typename A>
TIMPI_DECL_PACKING_RANGE_SUBCLASS(std::map<K TIMPI_P_COMMA T TIMPI_P_COMMA C TIMPI_P_COMMA A>);

template <typename K, typename T, typename C, typename A>
TIMPI_DECL_PACKING_RANGE_SUBCLASS(std::multimap<K TIMPI_P_COMMA T TIMPI_P_COMMA C TIMPI_P_COMMA A>);

template <typename K, typename C, typename A>
TIMPI_DECL_PACKING_RANGE_SUBCLASS(std::multiset<K TIMPI_P_COMMA C TIMPI_P_COMMA A>);

template <typename K, typename C, typename A>
TIMPI_DECL_PACKING_RANGE_SUBCLASS(std::set<K TIMPI_P_COMMA C TIMPI_P_COMMA A>);

template <typename K, typename T, typename H, typename KE, typename A>
TIMPI_DECL_PACKING_RANGE_SUBCLASS(std::unordered_map<K TIMPI_P_COMMA T TIMPI_P_COMMA H TIMPI_P_COMMA KE TIMPI_P_COMMA A>);

template <typename K, typename T, typename H, typename KE, typename A>
TIMPI_DECL_PACKING_RANGE_SUBCLASS(std::unordered_multimap<K TIMPI_P_COMMA T TIMPI_P_COMMA H TIMPI_P_COMMA KE TIMPI_P_COMMA A>);

template <typename K, typename H, typename KE, typename A>
TIMPI_DECL_PACKING_RANGE_SUBCLASS(std::unordered_multiset<K TIMPI_P_COMMA H TIMPI_P_COMMA KE TIMPI_P_COMMA A>);

template <typename K, typename H, typename KE, typename A>
TIMPI_DECL_PACKING_RANGE_SUBCLASS(std::unordered_set<K TIMPI_P_COMMA H TIMPI_P_COMMA KE TIMPI_P_COMMA A>);



#define TIMPI_HAVE_STRING_PACKING

template <typename T>
class Packing<std::basic_string<T>,
              typename std::enable_if<TIMPI::StandardType<T>::is_fixed_type>::type>;

} // namespace Parallel

} // namespace libMesh


namespace TIMPI {

using libMesh::Parallel::Packing;

/**
 * Decode a range of potentially-variable-size objects from a data
 * array.
 *
 * We take \p out_iter by value for maximum compatibility, but we
 * return it afterward for the use of code that needs to unpack
 * multiple buffers to the same output iterator.
 */
template <typename Context, typename buffertype,
          typename OutputIter, typename T>
inline OutputIter unpack_range (const typename std::vector<buffertype> & buffer,
                                Context * context,
                                OutputIter out_iter,
                                const T * output_type /* used only to infer T */);

/**
 * Encode a range of potentially-variable-size objects to a data
 * array.
 *
 * The data will be buffered in vectors with lengths that do not
 * exceed the sum of \p approx_buffer_size and the size of an
 * individual packed object.
 */
template <typename Context, typename buffertype, typename Iter>
inline Iter pack_range (const Context * context,
                        Iter range_begin,
                        const Iter range_end,
                        typename std::vector<buffertype> & buffer,
                        std::size_t approx_buffer_size = 1000000);

/**
 * Return the total buffer size needed to encode a range of
 * potentially-variable-size objects to a data array.
 */
template <typename Context, typename Iter>
inline std::size_t packed_range_size (const Context * context,
                                      Iter range_begin,
                                      const Iter range_end);

} // namespace TIMPI

#endif // TIMPI_PACKING_H
