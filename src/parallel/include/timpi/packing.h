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
#include "timpi/timpi_assert.h"
#include "timpi/packing_forward.h"
#include "timpi/standard_type.h"

// C++ includes
#include <cstring>     // memcpy
#include <iterator>
#include <type_traits> // enable_if, is_same
#include <utility>     // pair
#include <vector>


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
};

// Idiom taken from https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Member_Detector
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


// specialization for std::pair
template <typename T1, typename T2>
class Packing<std::pair<T1, T2>,
              typename std::enable_if<!TIMPI::StandardType<std::pair<T1, T2>>::is_fixed_type>::type>
{
public:
  typedef typename PairBufferTypeHelper<T1,
                                        Has_buffer_type<Packing<T1>>::value,
                                        T2,
                                        Has_buffer_type<Packing<T2>>::value>::buffer_type buffer_type;

  template <typename OutputIter, typename Context>
  static void pack(const std::pair<T1, T2> & pr, OutputIter data_out, const Context * context);

  template <typename Context>
  static unsigned int packable_size(const std::pair<T1, T2> & pr, const Context * context);

  template <typename BufferIter>
  static unsigned int packed_size(BufferIter iter);

  template <typename BufferIter, typename Context>
  static std::pair<T1, T2> unpack(BufferIter in, Context * ctx);

private:
  template <typename T3>
  struct IsFixed
  {
    static const bool value = TIMPI::StandardType<T3>::is_fixed_type;
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

  template <typename T3,
            typename Context,
            typename std::enable_if<!IsFixed<T3>::value, int>::type = 0>
  static unsigned int packable_size_comp(const T3 & comp, const Context * ctx)
  {
    return Packing<T3>::packable_size(comp, ctx);
  }

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

  template <typename T3,
            typename OutputIter,
            typename Context,
            typename std::enable_if<!IsFixed<T3>::value, int>::type = 0>
  static void pack_comp(const T3 & comp, OutputIter data_out, const Context * ctx)
  {
    Packing<T3>::pack(comp, data_out, ctx);
  }

  template <typename T3,
            typename BufferIter,
            typename Context,
            typename std::enable_if<IsFixed<T3>::value, int>::type = 0>
  static void unpack_comp(T3 & comp, BufferIter in, Context *)
  {
    std::memcpy(&comp, &(*in), sizeof(T3));
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

template <typename T1, typename T2>
template <typename Context>
unsigned int
Packing<std::pair<T1, T2>,
        typename std::enable_if<!TIMPI::StandardType<std::pair<T1, T2>>::is_fixed_type>::type>::
    packable_size(const std::pair<T1, T2> & pr, const Context * ctx)
{
  return 1 + packable_size_comp(pr.first, ctx) + packable_size_comp(pr.second, ctx);
}

template <typename T1, typename T2>
template <typename BufferIter>
unsigned int
Packing<std::pair<T1, T2>,
        typename std::enable_if<!TIMPI::StandardType<std::pair<T1, T2>>::is_fixed_type>::type>::
    packed_size(BufferIter iter)
{
  // We recorded the size in the first buffer entry
  return *iter;
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
  *data_out++ = TIMPI::cast_int<buffer_type>(size);

  // Now pack the data
  pack_comp(pr.first, data_out, ctx);

  // TIMPI uses a back_inserter for `pack_range` so we don't (and can't)
  // actually increment the iterator with operator+=. operator++ is a no-op
  //
  // data_out += packable_size_comp(pr.first, ctx);

  pack_comp(pr.second, data_out, ctx);
}

template <typename T1, typename T2>
template <typename BufferIter, typename Context>
std::pair<T1, T2>
Packing<std::pair<T1, T2>,
        typename std::enable_if<!TIMPI::StandardType<std::pair<T1, T2>>::is_fixed_type>::type>::
    unpack(BufferIter in, Context * ctx)
{
  std::pair<T1, T2> pr;

  // We don't care about the size
  in++;

  // Unpack the data
  unpack_comp(pr.first, in, ctx);

  // Make sure we increment the iterator
  in += packable_size_comp(pr.first, ctx);

  unpack_comp(pr.second, in, ctx);

  return pr;
}

#define TIMPI_HAVE_STRING_PACKING

template <typename T>
class Packing<std::basic_string<T>> {
public:

  static const unsigned int size_bytes = 4;

  typedef T buffer_type;

  static unsigned int
  get_string_len (typename std::vector<T>::const_iterator in)
  {
    unsigned int string_len = reinterpret_cast<const unsigned char &>(in[size_bytes-1]);
    for (signed int i=size_bytes-2; i >= 0; --i)
      {
        string_len *= 256;
        string_len += reinterpret_cast<const unsigned char &>(in[i]);
      }
    return string_len;
  }


  static unsigned int
  packed_size (typename std::vector<T>::const_iterator in)
  {
    return get_string_len(in) + size_bytes;
  }

  static unsigned int packable_size
  (const std::basic_string<T> & s,
   const void *)
  {
    return s.size() + size_bytes;
  }


  template <typename Iter>
  static void pack (const std::basic_string<T> & b, Iter data_out,
                    const void *)
  {
    unsigned int string_len = b.size();
    for (unsigned int i=0; i != size_bytes; ++i)
      {
        *data_out++ = (string_len % 256);
        string_len /= 256;
      }
    std::copy(b.begin(), b.end(), data_out);
  }

  static std::basic_string<T>
  unpack (typename std::vector<T>::const_iterator in, void *)
  {
    unsigned int string_len = get_string_len(in);

    std::ostringstream oss;
    for (unsigned int i = 0; i < string_len; ++i)
      oss << reinterpret_cast<const unsigned char &>(in[i+size_bytes]);

    in += size_bytes + string_len;

    return oss.str();
  }

};

} // namespace Parallel

} // namespace libMesh


namespace TIMPI {

using libMesh::Parallel::Packing;

/**
 * Decode a range of potentially-variable-size objects from a data
 * array.
 */
template <typename Context, typename buffertype,
          typename OutputIter, typename T>
inline void unpack_range (const typename std::vector<buffertype> & buffer,
                          Context * context,
                          OutputIter out,
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
