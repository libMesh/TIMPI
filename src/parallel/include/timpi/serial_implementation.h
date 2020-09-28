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


#ifndef TIMPI_SERIAL_IMPLEMENTATION_H
#define TIMPI_SERIAL_IMPLEMENTATION_H

// TIMPI includes
#include "timpi/attributes.h"
#include "timpi/communicator.h"
#include "timpi/data_type.h"
#include "timpi/timpi_call_mpi.h"
#include "timpi/message_tag.h"
#include "timpi/op_function.h"
#include "timpi/packing.h"
#include "timpi/timpi_assert.h"
#include "timpi/post_wait_copy_buffer.h"
#include "timpi/post_wait_delete_buffer.h"
#include "timpi/post_wait_dereference_shared_ptr.h"
#include "timpi/post_wait_dereference_tag.h"
#include "timpi/post_wait_free_buffer.h"
#include "timpi/post_wait_unpack_buffer.h"
#include "timpi/post_wait_unpack_nested_buffer.h"
#include "timpi/post_wait_work.h"
#include "timpi/request.h"
#include "timpi/status.h"
#include "timpi/standard_type.h"

// Disable libMesh logging until we decide how to port it best
// #include "libmesh/libmesh_logging.h"
#define TIMPI_LOG_SCOPE(f,c)

// C++ includes
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>
#include <type_traits>

#ifndef TIMPI_HAVE_MPI

namespace TIMPI {

using libMesh::Parallel::Packing;

/**
 * We do not currently support sends on one processor without MPI.
 */
template <typename T>
inline void Communicator::send (const unsigned int,
                                const T &,
                                const MessageTag &) const
{ timpi_not_implemented(); }

template <typename T>
inline void Communicator::send (const unsigned int,
                                const T &,
                                Request &,
                                const MessageTag &) const
{ timpi_not_implemented(); }

template <typename T>
inline void Communicator::send (const unsigned int,
                                const T &,
                                const DataType &,
                                const MessageTag &) const
{ timpi_not_implemented(); }

template <typename T>
inline void Communicator::send (const unsigned int,
                                const T &,
                                const DataType &,
                                Request &,
                                const MessageTag &) const
{ timpi_not_implemented(); }

template <typename T>
inline void Communicator::send (const unsigned int,
                                const T &,
                                const NotADataType &,
                                Request &,
                                const MessageTag &) const
{ timpi_not_implemented(); }

template <typename Context, typename Iter>
inline void Communicator::send_packed_range(const unsigned int,
                                            const Context *,
                                            Iter,
                                            const Iter,
                                            const MessageTag &,
                                            std::size_t) const
{ timpi_not_implemented(); }

template <typename Context, typename Iter>
inline void Communicator::send_packed_range (const unsigned int,
                                             const Context *,
                                             Iter,
                                             const Iter,
                                             Request &,
                                             const MessageTag &,
                                             std::size_t) const
{ timpi_not_implemented(); }

template <typename Context, typename Iter>
inline void Communicator::nonblocking_send_packed_range (const unsigned int,
                                                         const Context *,
                                                         Iter,
                                                         const Iter,
                                                         Request &,
                                                         const MessageTag &) const
{ timpi_not_implemented(); }

template <typename Context, typename OutputIter, typename T>
inline void Communicator::nonblocking_receive_packed_range (const unsigned int,
                                                            Context *,
                                                            OutputIter,
                                                            const T *,
                                                            Request &,
                                                            Status &,
                                                            const MessageTag &) const
{ timpi_not_implemented(); }

/**
 * We do not currently support receives on one processor without MPI.
 */
template <typename T>
inline Status Communicator::receive (const unsigned int,
                                     T &,
                                     const MessageTag &) const
{ timpi_not_implemented(); return Status(); }

template <typename T>
inline void Communicator::receive(const unsigned int,
                                  T &,
                                  Request &,
                                  const MessageTag &) const
{ timpi_not_implemented(); }

template <typename T>
inline Status Communicator::receive(const unsigned int,
                                    T &,
                                    const DataType &,
                                    const MessageTag &) const
{ timpi_not_implemented(); return Status(); }

template <typename T>
inline void Communicator::receive(const unsigned int,
                                  T &,
                                  const DataType &,
                                  Request &,
                                  const MessageTag &) const
{ timpi_not_implemented(); }

template <typename Context, typename OutputIter, typename T>
inline void
Communicator::receive_packed_range(const unsigned int,
                                   Context *,
                                   OutputIter,
                                   const T *,
                                   const MessageTag &) const
{ timpi_not_implemented(); }

// template <typename Context, typename OutputIter>
// inline void Communicator::receive_packed_range(const unsigned int, Context *, OutputIter, Request &, const MessageTag &) const
// { timpi_not_implemented(); }

/**
 * Send-receive data from one processor.
 */
template <typename T1, typename T2>
inline void Communicator::send_receive (const unsigned int timpi_dbg_var(send_tgt),
                                        const T1 & send_val,
                                        const unsigned int timpi_dbg_var(recv_source),
                                        T2 & recv_val,
                                        const MessageTag &,
                                        const MessageTag &) const
{
  timpi_assert_equal_to (send_tgt, 0);
  timpi_assert_equal_to (recv_source, 0);
  recv_val = send_val;
}

/**
 * Send-receive range-of-pointers from one processor.
 *
 * If you call this without MPI you might be making a mistake, but
 * we'll support it.
 */
template <typename Context1, typename RangeIter,
          typename Context2, typename OutputIter, typename T>
inline void
Communicator::send_receive_packed_range
  (const unsigned int timpi_dbg_var(dest_processor_id),
   const Context1 * context1,
   RangeIter send_begin,
   const RangeIter send_end,
   const unsigned int timpi_dbg_var(source_processor_id),
   Context2 * context2,
   OutputIter out_iter,
   const T * output_type,
   const MessageTag &,
   const MessageTag &,
   std::size_t) const
{
  // This makes no sense on one processor unless we're deliberately
  // sending to ourself.
  timpi_assert_equal_to(dest_processor_id, 0);
  timpi_assert_equal_to(source_processor_id, 0);

  // On one processor, we just need to pack the range and then unpack
  // it again.
  typedef typename std::iterator_traits<RangeIter>::value_type T1;
  typedef typename Packing<T1>::buffer_type buffer_t;

  while (send_begin != send_end)
    {
      timpi_assert_greater (std::distance(send_begin, send_end), 0);

      // We will serialize variable size objects from *range_begin to
      // *range_end as a sequence of ints in this buffer
      std::vector<buffer_t> buffer;

      const RangeIter next_send_begin = pack_range
        (context1, send_begin, send_end, buffer);

      timpi_assert_greater (std::distance(send_begin, next_send_begin), 0);

      send_begin = next_send_begin;

      unpack_range
        (buffer, context2, out_iter, output_type);
    }
}



template <typename T, typename A,
          typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type>
inline bool Communicator::possibly_receive (unsigned int &,
                                            std::vector<T,A> &,
                                            const DataType &,
                                            Request &,
                                            const MessageTag &) const
{
  // Non-blocking I/O from self to self?
  timpi_not_implemented();
}

template <typename T, typename A,
          typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type>
inline bool Communicator::possibly_receive (unsigned int &,
                                            std::vector<T,A> &,
                                            const NotADataType &,
                                            Request &,
                                            const MessageTag &) const
{
  // Non-blocking I/O from self to self?
  timpi_not_implemented();
}

template <typename T, typename A1, typename A2>
inline
bool Communicator::possibly_receive (unsigned int &,
                                     std::vector<std::vector<T,A1>,A2> &,
                                     const DataType &,
                                     Request &,
                                     const MessageTag &) const
{
  // Non-blocking I/O from self to self?
  timpi_not_implemented();
}

} // namespace TIMPI

#endif // ifndef TIMPI_HAVE_MPI

#endif // TIMPI_SERIAL_IMPLEMENTATION_H
