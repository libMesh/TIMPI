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


#ifndef TIMPI_PACKING_H
#define TIMPI_PACKING_H

// TIMPI Includes
#include "timpi/packing_decl.h"

#include "timpi/timpi_assert.h"

// C++ includes
#include <climits>     // CHAR_BIT
#include <cstring>     // memcpy
#include <iterator>
#include <type_traits> // is_same


// FIXME: This *should* be in TIMPI namespace but we have libMesh
// users which already partially specialized it
namespace libMesh
{

namespace Parallel
{

/**
 * Define data types and (un)serialization functions for use when
 * encoding a potentially-variable-size object of type T.
 *
 * Users will need to specialize this class for their particular data
 * types.
 */
template <typename T, typename Enable>
class Packing {
public:
  // Should be an MPI sendable type in specializations, e.g.
  // typedef char buffer_type;
  // typedef unsigned int buffer_type;

  // The following methods should be defined in every specialization,
  // but there's no good generic definition we can fall back on.
  // Leaving undefined generic methods declared here would turn
  // missing-header bugs into link-time failures rather than
  // compile-time failures - accordingly harder to diagnose.
#if 0
  // Should copy an encoding of the provided object into the provided
  // output iterator (which is of type buffer_type)
  template <typename OutputIter, typename Context>
  static void pack(const T & object,
                   OutputIter data_out,
                   const Context * context);

  // Should return the number of array entries (of type buffer_type)
  // required to encode the provided object
  template <typename Context>
  static unsigned int packable_size(const T & object,
                                    const Context * context);

  // Should return the number of array entries which were used to
  // encode the provided serialization of an object which begins at
  // \p iter
  template <typename BufferIter>
  static unsigned int packed_size(BufferIter iter);

  // Decode a potentially-variable-size object from a subsequence of a
  // data array, returning a heap-allocated pointer to the result.
  template <typename BufferIter, typename Context>
  static T unpack(BufferIter in, Context * ctx);
#endif
};


// Utility functions for encoding and decoding lengths into buffers
// with data types that may be too small to hold an unsigned int.  For
// MPI compatibility we assume that lengths do fit into an int.
template <typename buffer_type>
inline
constexpr int
get_packed_len_entries ()
{
  return
    (sizeof(unsigned int) + (sizeof(buffer_type)-1)) /
    sizeof(buffer_type);
}


template <typename buffer_type, typename Iter>
inline
void
put_packed_len (unsigned int len, Iter data_out)
{
  // I hoped decltype(*data_out) would always be buffer_type, but no dice

  // If we're using 2-byte or 1-byte buffer type then we have to split
  // into multiple entries
  constexpr int n_bits = (sizeof(buffer_type) * CHAR_BIT);

  // We may have a small signed buffer type into which we stuffed
  // an unsigned value
  if (n_bits < sizeof(unsigned int) * CHAR_BIT)
    {
      constexpr int size_entries = get_packed_len_entries<buffer_type>();

      const std::size_t max_entry = std::size_t(1) << n_bits;

      for (unsigned int i=0; i != size_entries; ++i)
        {
          *data_out++ = (len % max_entry);
          len /= max_entry;
        }

      return;
    }

  // With 32 bits or more this is trivial
  timpi_assert_equal_to(get_packed_len_entries<buffer_type>(), 1);
  *data_out++ = len;
}


template <typename buffer_type>
inline
unsigned int
get_packed_len (typename std::vector<buffer_type>::const_iterator in)
{
  // If we're using 2-byte or 1-byte buffer type then we have to split
  // into multiple entries
  constexpr int n_bits = (sizeof(buffer_type) * CHAR_BIT);

  // We may have a small signed buffer type into which we stuffed
  // an unsigned value
  if (n_bits < sizeof(unsigned int) * CHAR_BIT)
    {
      const int n_size_entries = get_packed_len_entries<buffer_type>();
      unsigned int packed_len = 0;

      for (signed int i = n_size_entries-1; i >= 0; --i)
        {
          packed_len <<= n_bits;

          const auto next_entry = in[i];

          if (next_entry < 0)
            packed_len += 1 << n_bits;

          packed_len += next_entry;
        }
      return packed_len;
    }

  // With 32 bits or more this is trivial

  timpi_assert_equal_to(get_packed_len_entries<buffer_type>(), 1);
  timpi_assert_greater_equal(*in, 0);

  return *in;
}


// Metafunction to get a value_type from map and unordered_map with
// non-const keys, so we can create a key/value pair more easily
template <typename ValueType>
struct DefaultValueType {
  typedef ValueType type;
};

template <typename K, typename V>
struct DefaultValueType<std::pair<const K, V>> {
  typedef std::pair<K, V> type;
};


// Superclass with utility methods for use with Packing partial
// specializations that mix fixed-size with Packing-required inner
// classes.
template <typename BufferType>
struct PackingMixedType
{
  typedef BufferType buffer_type;

  template <typename T3>
  struct IsFixed
  {
    static const bool value =
      TIMPI::StandardType
      <typename DefaultValueType<T3>::type>::is_fixed_type;
  };

  template <typename T3>
  struct BufferTypesPer
  {
    static const unsigned int value = (sizeof(T3) + sizeof(buffer_type) - 1) / sizeof(buffer_type);
  };

  template <typename T3,
            typename Context,
            typename std::enable_if<IsFixed<T3>::value, int>::type = 0>
  static unsigned int packable_size_comp(const T3 &, const Context *)
  {
    return BufferTypesPer<T3>::value;
  }

  // By not just doing memcpy here we can drop any padding
  template <typename T1, typename T2,
            typename Context,
            typename std::enable_if<IsFixed<std::pair<T1, T2>>::value, int>::type = 0>
  static unsigned int packable_size_comp(const std::pair<T1, T2> & comp, const Context * ctx)
  {
    return packable_size_comp(comp.first, ctx) +
           packable_size_comp(comp.second, ctx);
  }

  template <typename T3,
            typename Context,
            typename std::enable_if<!IsFixed<T3>::value, int>::type = 0>
  static unsigned int packable_size_comp(const T3 & comp, const Context * ctx)
  {
    return Packing<T3>::packable_size(comp, ctx);
  }

// g++ 11.2.0 gives "not protecting ... less than 8 bytes long" here,
// and that's more paranoid than we wanted --enable-paranoid-warnings
// to be...

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstack-protector"
#endif

  template <typename T3,
            typename OutputIter,
            typename Context,
            typename std::enable_if<IsFixed<T3>::value, int>::type = 0>
  static void pack_comp(const T3 & comp, OutputIter data_out, const Context *)
  {
    buffer_type T3_as_buffer_types[BufferTypesPer<T3>::value];
    std::memcpy(T3_as_buffer_types, &comp, sizeof(T3));
    for (unsigned int i = 0; i != BufferTypesPer<T3>::value; ++i)
      *data_out++ = T3_as_buffer_types[i];
  }

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

  // By not just doing memcpy here we can drop any padding
  template <typename T1, typename T2,
            typename OutputIter,
            typename Context,
            typename std::enable_if<IsFixed<std::pair<T1, T2>>::value, int>::type = 0>
  static void pack_comp(const std::pair<T1, T2> & comp, OutputIter data_out, const Context * ctx)
  {
    pack_comp(comp.first, data_out, ctx);
    pack_comp(comp.second, data_out, ctx);
  }

  template <typename T3,
            typename OutputIter,
            typename Context,
            typename std::enable_if<!IsFixed<T3>::value, int>::type = 0>
  static void pack_comp(const T3 & comp, OutputIter data_out, const Context * ctx)
  {
    Packing<T3>::pack(comp, data_out, ctx);
  }

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstack-protector"
#endif

  template <typename T3,
            typename BufferIter,
            typename Context,
            typename std::enable_if<IsFixed<T3>::value, int>::type = 0>
  static void unpack_comp(T3 & comp, BufferIter in, Context *)
  {
    // memcpy is only safe to use with classes that are trivial to
    // copy construct
    //
    // In this function overload, the enable_if<IsFixed> has already
    // determined that we're safe in that respect.
    //
    // But the C++ standards don't mandate that important types like
    // std::tuple ever satisfy is_trivially_copyable, and gcc goes so
    // far as to emit warnings based on is_trivial instead, so we need
    // to work around that here.
    // https://gcc.gnu.org/legacy-ml/gcc-patches/2017-07/msg00299.html
    char * comp_bytes = reinterpret_cast<char *>(&comp);
    std::memcpy(comp_bytes, &(*in), sizeof(T3));
  }

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

  template <typename T1, typename T2,
            typename BufferIter,
            typename Context,
            typename std::enable_if<IsFixed<std::pair<T1, T2>>::value, int>::type = 0>
  static void unpack_comp(std::pair<T1, T2> & comp, BufferIter in, Context * ctx)
  {
    unpack_comp(comp.first, in, ctx);

    in += packable_size_comp(comp.first, ctx);

    unpack_comp(comp.second, in, ctx);
  }


  template <typename T3,
            typename BufferIter,
            typename Context,
            typename std::enable_if<!IsFixed<T3>::value, int>::type = 0>
  static void unpack_comp(T3 & comp, BufferIter in, Context * ctx)
  {
    comp = Packing<T3>::unpack(in, ctx);
  }
};




// Utility metafunction for use in Packing<std::pair>
template <typename T1, bool T1_has_buffer_type, typename T2, bool T2_has_buffer_type>
struct PairBufferTypeHelper {};

template <typename T1, typename T2>
struct PairBufferTypeHelper<T1, true, T2, true>
{
  static_assert(std::is_same<typename Packing<T1>::buffer_type, typename Packing<T2>::buffer_type>::value,
                "For ease of use we cannot pack two types that use two different buffer types");

  typedef typename Packing<T1>::buffer_type buffer_type;
};

template <typename T1, typename T2>
struct PairBufferTypeHelper<T1, true, T2, false>
{
  typedef typename Packing<T1>::buffer_type buffer_type;
};

template <typename T1, typename T2>
struct PairBufferTypeHelper<T1, false, T2, true>
{
  typedef typename Packing<T2>::buffer_type buffer_type;
};

template <typename T1, typename T2>
struct PairBufferTypeHelper<T1, false, T2, false>
{
  typedef unsigned int buffer_type;
};


// specialization for std::pair
template <typename T1, typename T2>
class Packing<std::pair<T1, T2>,
              typename std::enable_if<!TIMPI::StandardType<std::pair<T1, T2>>::is_fixed_type>::type>
{
public:
  typedef typename PairBufferTypeHelper
      <T1, Has_buffer_type<Packing<T1>>::value,
       T2, Has_buffer_type<Packing<T2>>::value>::buffer_type buffer_type;

  typedef PackingMixedType<buffer_type> Mixed;

  template <typename OutputIter, typename Context>
  static void pack(const std::pair<T1, T2> & pr, OutputIter data_out, const Context * context);

  template <typename Context>
  static unsigned int packable_size(const std::pair<T1, T2> & pr, const Context * context);

  template <typename BufferIter>
  static unsigned int packed_size(BufferIter iter);

  template <typename BufferIter, typename Context>
  static std::pair<T1, T2> unpack(BufferIter in, Context * ctx);
};

template <typename T1, typename T2>
template <typename Context>
unsigned int
Packing<std::pair<T1, T2>,
        typename std::enable_if<!TIMPI::StandardType<std::pair<T1, T2>>::is_fixed_type>::type>::
    packable_size(const std::pair<T1, T2> & pr, const Context * ctx)
{
  return get_packed_len_entries<buffer_type>() +
    Mixed::packable_size_comp(pr.first, ctx) +
    Mixed::packable_size_comp(pr.second, ctx);
}

template <typename T1, typename T2>
template <typename BufferIter>
unsigned int
Packing<std::pair<T1, T2>,
        typename std::enable_if<!TIMPI::StandardType<std::pair<T1, T2>>::is_fixed_type>::type>::
    packed_size(BufferIter iter)
{
  // We recorded the size in the first buffer entries
  return get_packed_len<buffer_type>(iter);
}

template <typename T1, typename T2>
template <typename OutputIter, typename Context>
void
Packing<std::pair<T1, T2>,
        typename std::enable_if<!TIMPI::StandardType<std::pair<T1, T2>>::is_fixed_type>::type>::
    pack(const std::pair<T1, T2> & pr, OutputIter data_out, const Context * ctx)
{
  unsigned int size = packable_size(pr, ctx);

  // First write out info about the buffer size
  put_packed_len<buffer_type>(size, data_out);

  // Now pack the data
  Mixed::pack_comp(pr.first, data_out, ctx);

  // TIMPI uses a back_inserter for `pack_range` so we don't (and can't)
  // actually increment the iterator with operator+=. operator++ is a no-op
  //
  // data_out += packable_size_comp(pr.first, ctx);

  Mixed::pack_comp(pr.second, data_out, ctx);
}

template <typename T1, typename T2>
template <typename BufferIter, typename Context>
std::pair<T1, T2>
Packing<std::pair<T1, T2>,
        typename std::enable_if<!TIMPI::StandardType<std::pair<T1, T2>>::is_fixed_type>::type>::
    unpack(BufferIter in, Context * ctx)
{
  std::pair<T1, T2> pr;

  // We ignore total size here but we have to increment past it
  constexpr int size_bytes = get_packed_len_entries<buffer_type>();
  in += size_bytes;

  // Unpack the data
  Mixed::unpack_comp(pr.first, in, ctx);

  // Make sure we increment the iterator
  in += Mixed::packable_size_comp(pr.first, ctx);

  Mixed::unpack_comp(pr.second, in, ctx);

  return pr;
}




template <bool T1_has_buffer_type, bool MoreTypes_have_buffer_Type, typename T1, typename... MoreTypes>
struct TupleBufferTypeHelper {};

template <typename T1, bool MoreTypes_have_buffer_Type>
struct TupleBufferTypeHelper<true, MoreTypes_have_buffer_Type, T1> {
  typedef typename Packing<T1>::buffer_type buffer_type;
};

template <typename T1, typename... MoreTypes>
struct TupleBufferTypeHelper<true, true, T1, MoreTypes...> {
  static_assert(std::is_same<typename Packing<T1>::buffer_type,
                             typename Packing<std::tuple<MoreTypes...>>::buffer_type>::value,
                "For ease of use we cannot pack two types that use two different buffer types");

  typedef typename Packing<T1>::buffer_type buffer_type;
};

template <typename T1, typename... MoreTypes>
struct TupleBufferTypeHelper<true, false, T1, MoreTypes...> {
  typedef typename Packing<T1>::buffer_type buffer_type;
};

template <typename T1, typename... MoreTypes>
struct TupleBufferTypeHelper<false, true, T1, MoreTypes...> {
  typedef typename Packing<std::tuple<MoreTypes...>>::buffer_type buffer_type;
};

template <typename... Types>
struct TupleBufferType;

template <typename T1, typename... MoreTypes>
struct TupleBufferType<T1, MoreTypes...> {
  typedef typename
    TupleBufferTypeHelper<Has_buffer_type<Packing<T1>>::value,
                          Has_buffer_type<Packing<std::tuple<MoreTypes...>>>::value,
                          T1, MoreTypes...>::buffer_type buffer_type;
};



// specializations for std::tuple
template <typename Enable>
class Packing<std::tuple<>, Enable> {};

template <typename... Types>
class Packing<std::tuple<Types...>,
              typename std::enable_if<!TIMPI::StandardType<std::tuple<Types...>>::is_fixed_type>::type>
{
public:
  typedef typename TupleBufferType<Types...>::buffer_type buffer_type;

  typedef PackingMixedType<buffer_type> Mixed;

  template <typename OutputIter, typename Context>
  static void pack(const std::tuple<Types...> & tup, OutputIter data_out, const Context * context);

  template <typename Context>
  static unsigned int packable_size(const std::tuple<Types...> & tup, const Context * context);

  template <typename BufferIter>
  static unsigned int packed_size(BufferIter iter);

  template <typename BufferIter, typename Context>
  static std::tuple<Types...> unpack(BufferIter in, Context * ctx);

  template <typename Context,
            std::size_t I>
  static typename std::enable_if<I == sizeof...(Types), unsigned int>::type
  tail_packable_size(const std::tuple<Types...> &,
                     const Context *)
  { return 0; }

  template <typename Context,
            std::size_t I>
  static typename std::enable_if<I < sizeof...(Types), unsigned int>::type
  tail_packable_size(const std::tuple<Types...> &tup,
                     const Context * ctx)
  {
    return Mixed::packable_size_comp(std::get<I>(tup), ctx) +
      tail_packable_size<Context, I+1>(tup, ctx);
  }

  template <typename Context,
            typename OutputIter,
            std::size_t I>
  static typename std::enable_if<I == sizeof...(Types), void>::type
  tail_pack_comp(const std::tuple<Types...> &,
                 OutputIter,
                 const Context *) {}

  template <typename Context,
            typename OutputIter,
            std::size_t I>
  static typename std::enable_if<I < sizeof...(Types), void>::type
  tail_pack_comp(const std::tuple<Types...> &tup,
                 OutputIter data_out,
                 const Context * ctx)
  {
    Mixed::pack_comp(std::get<I>(tup), data_out, ctx);
    tail_pack_comp<Context, OutputIter, I+1>(tup, data_out, ctx);
  }

  template <typename Context,
            typename BufferIter,
            std::size_t I>
  static typename std::enable_if<I == sizeof...(Types), void>::type
  tail_unpack_comp(std::tuple<Types...> &,
                   BufferIter &,
                   Context *) {}

  template <typename Context,
            typename BufferIter,
            std::size_t I>
  static typename std::enable_if<I < sizeof...(Types), void>::type
  tail_unpack_comp(std::tuple<Types...> &tup,
                   BufferIter & in,
                   Context * ctx)
  {
    Mixed::unpack_comp(std::get<I>(tup), in, ctx);

    // Make sure we increment the iterator.  The last increment will
    // be unnecessary, since this is a copy of the iterator in the
    // higher level unpacking code, but the first N-1 are critical.
    in += Mixed::packable_size_comp(std::get<I>(tup), ctx);

    tail_unpack_comp<Context, BufferIter, I+1>(tup, in, ctx);
  }
};


template <typename... Types>
template <typename Context>
unsigned int
Packing<std::tuple<Types...>,
        typename std::enable_if<!TIMPI::StandardType<std::tuple<Types...>>::is_fixed_type>::type>::
    packable_size(const std::tuple<Types...> & tup, const Context * ctx)
{
  return get_packed_len_entries<buffer_type>() +
    tail_packable_size<Context, 0>(tup, ctx);
}

template <typename... Types>
template <typename BufferIter>
unsigned int
Packing<std::tuple<Types...>,
        typename std::enable_if<!TIMPI::StandardType<std::tuple<Types...>>::is_fixed_type>::type>::
    packed_size(BufferIter iter)
{
  // We recorded the size in the first buffer entries
  return get_packed_len<buffer_type>(iter);
}

template <typename... Types>
template <typename OutputIter, typename Context>
void
Packing<std::tuple<Types...>,
        typename std::enable_if<!TIMPI::StandardType<std::tuple<Types...>>::is_fixed_type>::type>::
    pack(const std::tuple<Types...> & tup, OutputIter data_out, const Context * ctx)
{
  unsigned int size = packable_size(tup, ctx);

  // First write out info about the buffer size
  put_packed_len<buffer_type>(size, data_out);

  // Now pack the data
  tail_pack_comp<Context, OutputIter, 0>(tup, data_out, ctx);
}

template <typename... Types>
template <typename BufferIter, typename Context>
std::tuple<Types...>
Packing<std::tuple<Types...>,
        typename std::enable_if<!TIMPI::StandardType<std::tuple<Types...>>::is_fixed_type>::type>::
    unpack(BufferIter in, Context * ctx)
{
  std::tuple<Types...> tup;

  // We ignore total size here but we have to increment past it
  constexpr int size_bytes = get_packed_len_entries<buffer_type>();
  in += size_bytes;

  // Unpack the data
  tail_unpack_comp<Context, BufferIter, 0>(tup, in, ctx);

  return tup;
}



// specialization for std::array
template <typename T, std::size_t N>
class Packing<std::array<T, N>,
              typename std::enable_if<!TIMPI::StandardType<T>::is_fixed_type>::type>
{
public:
  typedef typename Packing<T>::buffer_type buffer_type;

  typedef PackingMixedType<buffer_type> Mixed;

  template <typename OutputIter, typename Context>
  static void pack(const std::array<T, N> & a, OutputIter data_out, const Context * context);

  template <typename Context>
  static unsigned int packable_size(const std::array<T, N> & a, const Context * context);

  template <typename BufferIter>
  static unsigned int packed_size(BufferIter iter);

  template <typename BufferIter, typename Context>
  static std::array<T, N> unpack(BufferIter in, Context * ctx);
};

template <typename T, std::size_t N>
template <typename Context>
unsigned int
Packing<std::array<T, N>,
        typename std::enable_if<!TIMPI::StandardType<T>::is_fixed_type>::type>::
    packable_size(const std::array<T, N> & a, const Context * ctx)
{
  unsigned int returnval = get_packed_len_entries<buffer_type>(); // size
  for (const auto & entry : a)
    returnval += Mixed::packable_size_comp(entry, ctx);
  return returnval;
}

template <typename T, std::size_t N>
template <typename BufferIter>
unsigned int
Packing<std::array<T, N>,
        typename std::enable_if<!TIMPI::StandardType<T>::is_fixed_type>::type>::
    packed_size(BufferIter iter)
{
  // We recorded the size in the first buffer entries
  return get_packed_len<buffer_type>(iter);
}

template <typename T, std::size_t N>
template <typename OutputIter, typename Context>
void
Packing<std::array<T, N>,
        typename std::enable_if<!TIMPI::StandardType<T>::is_fixed_type>::type>::
    pack(const std::array<T, N> & a, OutputIter data_out, const Context * ctx)
{
  unsigned int size = packable_size(a, ctx);

  // First write out info about the buffer size
  put_packed_len<buffer_type>(size, data_out);

  // Now pack the data
  for (const auto & entry : a)
    Mixed::pack_comp(entry, data_out, ctx);
}

template <typename T, std::size_t N>
template <typename BufferIter, typename Context>
std::array<T, N>
Packing<std::array<T, N>,
        typename std::enable_if<!TIMPI::StandardType<T>::is_fixed_type>::type>::
    unpack(BufferIter in, Context * ctx)
{
  std::array<T, N> a;

  // We ignore total size here but we have to increment past it
  constexpr int size_bytes = get_packed_len_entries<buffer_type>();
  in += size_bytes;

  // Unpack the data
  for (auto & entry : a)
    {
      Mixed::unpack_comp(entry, in, ctx);

      // Make sure we increment the iterator
      in += Mixed::packable_size_comp(entry, ctx);
    }

  return a;
}



// Metafunction to choose buffer types: use a specified
// Packing<class>::buffer_type for any class that has one; use
// unsigned int otherwise.
template <typename T, typename Enable=void>
struct DefaultBufferType;

template <typename T>
struct DefaultBufferType <T, typename std::enable_if<Has_buffer_type<Packing<T>>::value>::type>
{
  typedef typename Packing<T>::buffer_type type;
};

template <typename T>
struct DefaultBufferType <T, typename std::enable_if<!Has_buffer_type<Packing<T>>::value>::type>
{
  typedef unsigned int type;
};


// helper class for any homogeneous-type variable-size containers
// which define the usual iterator ranges, value_type, etc.
template <typename Container>
class PackingRange
{
public:
  typedef typename
    DefaultBufferType<typename Container::value_type>::type
    buffer_type;

  typedef PackingMixedType<buffer_type> Mixed;

  template <typename OutputIter, typename Context>
  static void pack(const Container & a,
                   OutputIter data_out, const Context * context);

  template <typename Context>
  static unsigned int packable_size(const Container & a,
                                    const Context * context);

  template <typename BufferIter>
  static unsigned int packed_size(BufferIter iter);

  template <typename BufferIter, typename Context>
  static Container unpack(BufferIter in, Context * ctx);
};


template <typename Container>
template <typename Context>
unsigned int
PackingRange<Container>::packable_size(const Container & c, const Context * ctx)
{
  unsigned int returnval = get_packed_len_entries<buffer_type>(); // size
  for (const auto & entry : c)
    returnval += Mixed::packable_size_comp(entry, ctx);
  return returnval;
}

template <typename Container>
template <typename BufferIter>
unsigned int
PackingRange<Container>::packed_size(BufferIter iter)
{
  // We recorded the size in the first buffer entries
  return get_packed_len<buffer_type>(iter);
}

template <typename Container>
template <typename OutputIter, typename Context>
void
PackingRange<Container>::pack(const Container & c, OutputIter data_out, const Context * ctx)
{
  unsigned int size = packable_size(c, ctx);

  // First write out info about the buffer size
  put_packed_len<buffer_type>(size, data_out);

  // Now pack the data
  for (const auto & entry : c)
    Mixed::pack_comp(entry, data_out, ctx);
}

template <typename Container>
template <typename BufferIter, typename Context>
Container
PackingRange<Container>::unpack(BufferIter in, Context * ctx)
{
  Container c;

  unsigned int size = packed_size(in);

  timpi_assert_greater(size, 0);

  // Get the total size
  constexpr int size_bytes = get_packed_len_entries<buffer_type>();
  in += size_bytes;
  size -= size_bytes;

  // Unpack the data
  std::size_t unpacked_size = 0;
  while (unpacked_size < size)
    {
      typename DefaultValueType<typename Container::value_type>::type entry;
      Mixed::unpack_comp(entry, in, ctx);

      c.insert(c.end(), entry);

      // Make sure we increment the iterator
      const std::size_t unpacked_size_comp =
        Mixed::packable_size_comp(entry, ctx);
      in += unpacked_size_comp;
      unpacked_size += unpacked_size_comp;
    }

  // We should always finish at exactly the size we expected, not
  // proceed past it
  timpi_assert_equal_to(unpacked_size, size);

  return c;
}



#define TIMPI_PACKING_RANGE_SUBCLASS(Container)           \
class Packing<Container> : public PackingRange<Container> \
{                                                         \
public:                                                   \
  using typename PackingRange<Container>::buffer_type;    \
                                                          \
  using typename PackingRange<Container>::Mixed;          \
                                                          \
  using PackingRange<Container>::pack;                    \
  using PackingRange<Container>::packable_size;           \
  using PackingRange<Container>::packed_size;             \
  using PackingRange<Container>::unpack;                  \
}


template <typename T, typename A>
TIMPI_PACKING_RANGE_SUBCLASS(std::vector<T TIMPI_P_COMMA A>);

template <typename T, typename A>
TIMPI_PACKING_RANGE_SUBCLASS(std::list<T TIMPI_P_COMMA A>);

template <typename K, typename T, typename C, typename A>
TIMPI_PACKING_RANGE_SUBCLASS(std::map<K TIMPI_P_COMMA T TIMPI_P_COMMA C TIMPI_P_COMMA A>);

template <typename K, typename T, typename C, typename A>
TIMPI_PACKING_RANGE_SUBCLASS(std::multimap<K TIMPI_P_COMMA T TIMPI_P_COMMA C TIMPI_P_COMMA A>);

template <typename K, typename C, typename A>
TIMPI_PACKING_RANGE_SUBCLASS(std::multiset<K TIMPI_P_COMMA C TIMPI_P_COMMA A>);

template <typename K, typename C, typename A>
TIMPI_PACKING_RANGE_SUBCLASS(std::set<K TIMPI_P_COMMA C TIMPI_P_COMMA A>);

template <typename K, typename T, typename H, typename KE, typename A>
TIMPI_PACKING_RANGE_SUBCLASS(std::unordered_map<K TIMPI_P_COMMA T TIMPI_P_COMMA H TIMPI_P_COMMA KE TIMPI_P_COMMA A>);

template <typename K, typename T, typename H, typename KE, typename A>
TIMPI_PACKING_RANGE_SUBCLASS(std::unordered_multimap<K TIMPI_P_COMMA T TIMPI_P_COMMA H TIMPI_P_COMMA KE TIMPI_P_COMMA A>);

template <typename K, typename H, typename KE, typename A>
TIMPI_PACKING_RANGE_SUBCLASS(std::unordered_multiset<K TIMPI_P_COMMA H TIMPI_P_COMMA KE TIMPI_P_COMMA A>);

template <typename K, typename H, typename KE, typename A>
TIMPI_PACKING_RANGE_SUBCLASS(std::unordered_set<K TIMPI_P_COMMA H TIMPI_P_COMMA KE TIMPI_P_COMMA A>);


template <typename T>
class Packing<std::basic_string<T>> {
public:

  typedef unsigned int buffer_type;

  static_assert(sizeof(T) <= sizeof(buffer_type),
                "We don't support strings with larger characters than unsigned int");

  static constexpr int T_per_buffer_type = (sizeof(buffer_type) + sizeof(T) - 1)/sizeof(T);

  static unsigned int
  packed_size (typename std::vector<buffer_type>::const_iterator in)
  {
    // One entry for size, then pack the data tightly.
    return 1 + (*in+T_per_buffer_type-1)/T_per_buffer_type;
  }

  static unsigned int packable_size
  (const std::basic_string<T> & s,
   const void *)
  {
    return 1 + (s.size()+T_per_buffer_type-1)/T_per_buffer_type;
  }


  template <typename Iter>
  static void pack (const std::basic_string<T> & b, Iter data_out,
                    const void *)
  {
    *data_out++ = b.size();
    std::size_t i = 0;
    for (; i + T_per_buffer_type < b.size(); i += T_per_buffer_type)
      *data_out++ = *reinterpret_cast<const buffer_type *>(&b[i]);
    if (i != b.size())
      {
        T with_padding[T_per_buffer_type] = {};
        std::copy(b.begin()+i, b.end(), &with_padding[0]);
        *data_out++ = *reinterpret_cast<const buffer_type *>(&with_padding[0]);
      }
  }

  static std::basic_string<T>
  unpack (typename std::vector<buffer_type>::const_iterator in, void *)
  {
    const unsigned int string_len = *in++;

    const T * buf =
      reinterpret_cast<const T *>(&(*in));
    in += string_len / T_per_buffer_type;
    return {buf, buf+string_len};
  }

};

} // namespace Parallel

} // namespace libMesh


namespace TIMPI {

using libMesh::Parallel::Packing;

// ------------------------------------------------------------
// Packing member functions, global functions

/**
 * Helper function for range packing
 */
template <typename Context, typename Iter>
inline std::size_t packed_range_size (const Context * context,
                                      Iter range_begin,
                                      const Iter range_end)
{
  typedef typename std::iterator_traits<Iter>::value_type T;

  std::size_t buffer_size = 0;
  for (Iter range_count = range_begin;
       range_count != range_end;
       ++range_count)
    {
      buffer_size += Packing<T>::packable_size(*range_count, context);
    }
  return buffer_size;
}


/**
 * Helper function for range packing
 */
template <typename Context, typename buffertype, typename Iter>
inline Iter pack_range (const Context * context,
                        Iter range_begin,
                        const Iter range_end,
                        std::vector<buffertype> & buffer,
                        // When we serialize into buffers, we need to use large buffers to optimize MPI
                        // bandwidth, but not so large as to risk allocation failures.  max_buffer_size
                        // is measured in number of buffer type entries; number of bytes may be 4 or 8
                        // times larger depending on configuration.
                        std::size_t approx_buffer_size)
{
  typedef typename std::iterator_traits<Iter>::value_type T;

  // Count the total size of and preallocate buffer for efficiency.
  // Prepare to stop early if the buffer would be too large.
  std::size_t buffer_size = 0;
  Iter range_stop = range_begin;
  for (; range_stop != range_end && buffer_size < approx_buffer_size;
       ++range_stop)
    {
      std::size_t next_buffer_size =
        Packing<T>::packable_size(*range_stop, context);
      buffer_size += next_buffer_size;
    }
  buffer.reserve(buffer.size() + buffer_size);

  // Pack the objects into the buffer
  for (; range_begin != range_stop; ++range_begin)
    {
#ifndef NDEBUG
      std::size_t old_size = buffer.size();
#endif

      Packing<T>::pack
        (*range_begin, std::back_inserter(buffer), context);

#ifndef NDEBUG
      unsigned int my_packable_size =
        Packing<T>::packable_size(*range_begin, context);
      unsigned int my_packed_size =
        Packing<T>::packed_size (buffer.begin() + old_size);
      timpi_assert_equal_to (my_packable_size, my_packed_size);
      timpi_assert_equal_to (buffer.size(), old_size + my_packable_size);
#endif
    }

  return range_stop;
}



/**
 * Helper function for range unpacking
 */
template <typename Context, typename buffertype,
          typename OutputIter, typename T>
inline void unpack_range (const std::vector<buffertype> & buffer,
                          Context * context,
                          OutputIter out_iter,
                          const T * /* output_type */)
{
  // Loop through the buffer and unpack each object, returning the
  // object pointer via the output iterator
  typename std::vector<buffertype>::const_iterator
    next_object_start = buffer.begin();

  while (next_object_start < buffer.end())
    {
      *out_iter++ = Packing<T>::unpack(next_object_start, context);
      next_object_start +=
        Packing<T>::packed_size(next_object_start);
    }

  // We should have used up the exact amount of data in the buffer
  timpi_assert (next_object_start == buffer.end());
}

} // namespace TIMPI

#endif // TIMPI_PACKING_H
