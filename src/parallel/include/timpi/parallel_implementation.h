// The TIMPI Message-Passing Parallelism Library.
// Copyright (C) 2002-2025 Benjamin S. Kirk, John W. Peterson, Roy H. Stogner

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


#ifndef TIMPI_PARALLEL_IMPLEMENTATION_H
#define TIMPI_PARALLEL_IMPLEMENTATION_H

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

#ifndef TIMPI_HAVE_MPI
#include "timpi/serial_implementation.h"
#endif

// Boost include if necessary for float128
#ifdef TIMPI_DEFAULT_QUADRUPLE_PRECISION
# include <boost/multiprecision/float128.hpp>
#endif

// Disable libMesh logging until we decide how to port it best
// #include "libmesh/libmesh_logging.h"
#define TIMPI_LOG_SCOPE(f,c)

// C++ includes
#include <complex>
#include <cstddef>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <type_traits>

namespace TIMPI {

using libMesh::Parallel::Packing;

#ifdef TIMPI_HAVE_MPI

/**
 * Templated function to return the appropriate MPI datatype
 * for use with built-in C types when combined with an int,
 * or MPI_DATATYPE_NULL for types which have no predefined datatype.
 */
template <typename T>
inline data_type dataplusint_type() { return MPI_DATATYPE_NULL; }

#endif // TIMPI_HAVE_MPI

/**
 * Types combined with an int
 */
template <typename T>
class DataPlusInt
{
public:
  T val;
  int rank;
};

} // namespace Parallel


// Anonymous namespace for helper functions
namespace {

// Internal helper function to create vector<something_usable> from
// vector<bool> for compatibility with MPI bitwise operations
template <typename T, typename A1, typename A2>
inline void pack_vector_bool(const std::vector<bool,A1> & vec_in,
                             std::vector<T,A2> & vec_out)
{
  unsigned int data_bits = 8*sizeof(T);
  std::size_t in_size = vec_in.size();
  std::size_t out_size = in_size/data_bits + ((in_size%data_bits)?1:0);
  vec_out.clear();
  vec_out.resize(out_size);
  for (std::size_t i=0; i != in_size; ++i)
    {
      std::size_t index = i/data_bits;
      std::size_t offset = i%data_bits;
      vec_out[index] += (vec_in[i]?1u:0u) << offset;
    }
}

// Internal helper function to create vector<bool> from
// vector<something usable> for compatibility with MPI byte
// operations
template <typename T, typename A1, typename A2>
inline void unpack_vector_bool(const std::vector<T,A1> & vec_in,
                               std::vector<bool,A2> & vec_out)
{
  unsigned int data_bits = 8*sizeof(T);
  // We need the output vector to already be properly sized
  std::size_t out_size = vec_out.size();
  timpi_assert_equal_to
    (out_size/data_bits + (out_size%data_bits?1:0), vec_in.size());

  for (std::size_t i=0; i != out_size; ++i)
    {
      std::size_t index = i/data_bits;
      std::size_t offset = i%data_bits;
      vec_out[i] = (vec_in[index] >> offset) & 1;
    }
}


#ifdef TIMPI_HAVE_MPI
// We use a helper function here to avoid ambiguity when calling
// send_receive of (vector<vector<T>>,vector<vector<T>>)
template <typename T1, typename T2, typename A1, typename A2, typename A3, typename A4>
inline void send_receive_vec_of_vec(const unsigned int dest_processor_id,
                                    const std::vector<std::vector<T1,A1>,A2> & send_data,
                                    const unsigned int source_processor_id,
                                    std::vector<std::vector<T2,A3>,A4> & recv_data,
                                    const TIMPI::MessageTag & send_tag,
                                    const TIMPI::MessageTag & recv_tag,
                                    const TIMPI::Communicator & comm)
{
  TIMPI_LOG_SCOPE("send_receive()", "Parallel");

  if (dest_processor_id   == comm.rank() &&
      source_processor_id == comm.rank())
    {
      recv_data = send_data;
      return;
    }

  TIMPI::Request req;
  comm.send (dest_processor_id, send_data, req, send_tag);
  comm.receive (source_processor_id, recv_data, recv_tag);
  req.wait();
}

#endif // TIMPI_HAVE_MPI

} // Anonymous namespace



namespace TIMPI
{

#ifdef TIMPI_HAVE_MPI
template<>
inline data_type dataplusint_type<short int>() { return MPI_SHORT_INT; }

template<>
inline data_type dataplusint_type<int>() { return MPI_2INT; }

template<>
inline data_type dataplusint_type<long>() { return MPI_LONG_INT; }

template<>
inline data_type dataplusint_type<float>() { return MPI_FLOAT_INT; }

template<>
inline data_type dataplusint_type<double>() { return MPI_DOUBLE_INT; }

template<>
inline data_type dataplusint_type<long double>() { return MPI_LONG_DOUBLE_INT; }

template <typename T>
inline
std::pair<data_type, std::unique_ptr<StandardType<std::pair<T,int>>>>
dataplusint_type_acquire()
{
  std::pair<data_type, std::unique_ptr<StandardType<std::pair<T,int>>>> return_val;
  return_val.first = dataplusint_type<T>();
  if (return_val.first == MPI_DATATYPE_NULL)
    {
      return_val.second.reset(new StandardType<std::pair<T,int>>());
      return_val.first = *return_val.second;
    }
  return return_val;
}



#if MPI_VERSION > 3
  typedef MPI_Aint DispType;
#  define TIMPI_COUNT_TYPE MPI_COUNT
#  define TIMPI_PACK_SIZE MPI_Pack_size_c
#  define TIMPI_SEND MPI_Send_c
#  define TIMPI_SSEND MPI_Ssend_c
#  define TIMPI_ALLREDUCE MPI_Allreduce_c
#  define TIMPI_IALLREDUCE MPI_Iallreduce_c
#  define TIMPI_ISEND MPI_Isend_c
#  define TIMPI_ISSEND MPI_Issend_c
#  define TIMPI_PACK MPI_Pack_c
#  define TIMPI_UNPACK MPI_Unpack_c
#  define TIMPI_RECV MPI_Recv_c
#  define TIMPI_IRECV MPI_Irecv_c
#  define TIMPI_SENDRECV MPI_Sendrecv_c
#  define TIMPI_ALLGATHERV MPI_Allgatherv_c
#  define TIMPI_ALLGATHER MPI_Allgather_c
#  define TIMPI_BCAST MPI_Bcast_c
#  define TIMPI_GATHER MPI_Gather_c
#  define TIMPI_GATHERV MPI_Gatherv_c
#  define TIMPI_SCATTER MPI_Scatter_c
#  define TIMPI_SCATTERV MPI_Scatterv_c
#  define TIMPI_ALLTOALL MPI_Alltoall_c
#else
  typedef int DispType;
#  define TIMPI_COUNT_TYPE MPI_INT
#  define TIMPI_PACK_SIZE MPI_Pack_size
#  define TIMPI_SEND MPI_Send
#  define TIMPI_SSEND MPI_Ssend
#  define TIMPI_ALLREDUCE MPI_Allreduce
#  define TIMPI_IALLREDUCE MPI_Iallreduce
#  define TIMPI_ISEND MPI_Isend
#  define TIMPI_ISSEND MPI_Issend
#  define TIMPI_PACK MPI_Pack
#  define TIMPI_UNPACK MPI_Unpack
#  define TIMPI_RECV MPI_Recv
#  define TIMPI_IRECV MPI_Irecv
#  define TIMPI_SENDRECV MPI_Sendrecv
#  define TIMPI_ALLGATHERV MPI_Allgatherv
#  define TIMPI_ALLGATHER MPI_Allgather
#  define TIMPI_BCAST MPI_Bcast
#  define TIMPI_GATHER MPI_Gather
#  define TIMPI_GATHERV MPI_Gatherv
#  define TIMPI_SCATTER MPI_Scatter
#  define TIMPI_SCATTERV MPI_Scatterv
#  define TIMPI_ALLTOALL MPI_Alltoall
#endif



template <typename T, typename A1, typename A2>
std::size_t Communicator::packed_size_of(const std::vector<std::vector<T,A1>,A2> & buf,
                                         const DataType & type) const
{
  // Figure out how many bytes we need to pack all the data
  //
  // Start with the outer buffer size
  CountType packedsize=0;

  timpi_call_mpi
    (TIMPI_PACK_SIZE (1, TIMPI_COUNT_TYPE, this->get(), &packedsize));

  std::size_t sendsize = packedsize;

  const std::size_t n_vecs = buf.size();

  for (std::size_t i = 0; i != n_vecs; ++i)
    {
      // The size of the ith inner buffer
      timpi_call_mpi
        (TIMPI_PACK_SIZE (1, TIMPI_COUNT_TYPE, this->get(), &packedsize));

      sendsize += packedsize;

      // The data for each inner buffer
      timpi_call_mpi
        (TIMPI_PACK_SIZE (cast_int<CountType>(buf[i].size()),
                          type, this->get(), &packedsize));

      sendsize += packedsize;
    }

  timpi_assert (sendsize /* should at least be 1! */);
  return sendsize;
}


template<typename T>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::basic_string<T> & buf,
                                const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("send()", "Parallel");

  T * dataptr = buf.empty() ? nullptr : const_cast<T *>(buf.data());

  timpi_assert_less(dest_processor_id, this->size());

  timpi_call_mpi
    (((this->send_mode() == SYNCHRONOUS) ?
      TIMPI_SSEND : TIMPI_SEND)
        (dataptr, cast_int<CountType>(buf.size()),
         StandardType<T>(dataptr), dest_processor_id, tag.value(),
         this->get()));
}



template <typename T>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::basic_string<T> & buf,
                                Request & req,
                                const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("send()", "Parallel");

  T * dataptr = buf.empty() ? nullptr : const_cast<T *>(buf.data());

  timpi_assert_less(dest_processor_id, this->size());

  timpi_call_mpi
    (((this->send_mode() == SYNCHRONOUS) ?
      TIMPI_ISSEND : TIMPI_ISEND)
        (dataptr, cast_int<CountType>(buf.size()),
         StandardType<T>(dataptr), dest_processor_id, tag.value(),
         this->get(), req.get()));

  // The MessageTag should stay registered for the Request lifetime
  req.add_post_wait_work
    (new PostWaitDereferenceTag(tag));
}



template <typename T>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const T & buf,
                                const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("send()", "Parallel");

  T * dataptr = const_cast<T*> (&buf);

  timpi_assert_less(dest_processor_id, this->size());

  timpi_call_mpi
    (((this->send_mode() == SYNCHRONOUS) ?
      TIMPI_SSEND : TIMPI_SEND)
        (dataptr, 1, StandardType<T>(dataptr), dest_processor_id,
         tag.value(), this->get()));
}



template <typename T>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const T & buf,
                                Request & req,
                                const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("send()", "Parallel");

  T * dataptr = const_cast<T*>(&buf);

  timpi_assert_less(dest_processor_id, this->size());

  timpi_call_mpi
    (((this->send_mode() == SYNCHRONOUS) ?
      TIMPI_ISSEND : TIMPI_ISEND)
        (dataptr, 1, StandardType<T>(dataptr), dest_processor_id,
         tag.value(), this->get(), req.get()));

  // The MessageTag should stay registered for the Request lifetime
  req.add_post_wait_work
    (new PostWaitDereferenceTag(tag));
}



template <typename T, typename C, typename A>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::set<T,C,A> & buf,
                                const MessageTag & tag) const
{
  this->send(dest_processor_id, buf,
             StandardType<T>(buf.empty() ? nullptr : &(*buf.begin())), tag);
}



template <typename T, typename C, typename A>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::set<T,C,A> & buf,
                                Request & req,
                                const MessageTag & tag) const
{
  this->send(dest_processor_id, buf,
             StandardType<T>(buf.empty() ? nullptr : &(*buf.begin())), req, tag);
}



template <typename T, typename C, typename A>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::set<T,C,A> & buf,
                                const DataType & type,
                                const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("send()", "Parallel");

  std::vector<T> vecbuf(buf.begin(), buf.end());
  this->send(dest_processor_id, vecbuf, type, tag);
}



template <typename T, typename C, typename A>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::set<T,C,A> & buf,
                                const DataType & type,
                                Request & req,
                                const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("send()", "Parallel");

  // Allocate temporary buffer on the heap so it lives until after
  // the non-blocking send completes
  std::vector<T> * vecbuf =
    new std::vector<T,A>(buf.begin(), buf.end());

  // Make the Request::wait() handle deleting the buffer
  req.add_post_wait_work
    (new PostWaitDeleteBuffer<std::vector<T,A>>(vecbuf));

  this->send(dest_processor_id, *vecbuf, type, req, tag);
}



template <typename T, typename A>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::vector<T,A> & buf,
                                const MessageTag & tag) const
{
  this->send(dest_processor_id, buf,
             StandardType<T>(buf.empty() ? nullptr : &buf.front()), tag);
}



template <typename T, typename A,
          typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::vector<T,A> & buf,
                                Request & req,
                                const MessageTag & tag) const
{
  this->send(dest_processor_id, buf,
             StandardType<T>(buf.empty() ? nullptr : &buf.front()), req, tag);
}

template <typename T, typename A,
          typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::vector<T,A> & buf,
                                Request & req,
                                const MessageTag & tag) const
{
  this->nonblocking_send_packed_range(dest_processor_id,
                                      (void *)(nullptr),
                                      buf.begin(),
                                      buf.end(),
                                      req,
                                      tag);
}


template <typename T, typename A>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::vector<T,A> & buf,
                                const DataType & type,
                                const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("send()", "Parallel");

  timpi_call_mpi
    (((this->send_mode() == SYNCHRONOUS) ?
      TIMPI_SSEND : TIMPI_SEND)
        (buf.empty() ? nullptr : const_cast<T*>(buf.data()),
         cast_int<CountType>(buf.size()), type, dest_processor_id,
         tag.value(), this->get()));
}



template <typename T, typename A, typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::vector<T,A> & buf,
                                const DataType & type,
                                Request & req,
                                const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("send()", "Parallel");

  timpi_assert_less(dest_processor_id, this->size());

  timpi_call_mpi
    (((this->send_mode() == SYNCHRONOUS) ?
      TIMPI_ISSEND : TIMPI_ISEND)
        (buf.empty() ? nullptr : const_cast<T*>(buf.data()),
         cast_int<CountType>(buf.size()), type, dest_processor_id,
         tag.value(), this->get(), req.get()));

  // The MessageTag should stay registered for the Request lifetime
  req.add_post_wait_work
    (new PostWaitDereferenceTag(tag));
}

template <typename T, typename A, typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::vector<T,A> & buf,
                                const NotADataType &,
                                Request & req,
                                const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("send()", "Parallel");

  timpi_assert_less(dest_processor_id, this->size());

  this->nonblocking_send_packed_range(dest_processor_id,
                                      (void *)(nullptr),
                                      buf.begin(),
                                      buf.end(),
                                      req,
                                      tag);
}



template <typename T, typename A1, typename A2>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::vector<std::vector<T,A1>,A2> & buf,
                                const MessageTag & tag) const
{
  this->send(dest_processor_id, buf,
             StandardType<T>((buf.empty() || buf.front().empty()) ?
                             nullptr : &(buf.front().front())), tag);
}



template <typename T, typename A1, typename A2>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::vector<std::vector<T,A1>,A2> & buf,
                                Request & req,
                                const MessageTag & tag) const
{
  this->send(dest_processor_id, buf,
             StandardType<T>((buf.empty() || buf.front().empty()) ?
                             nullptr : &(buf.front().front())), req, tag);
}



template <typename T, typename A1, typename A2>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::vector<std::vector<T,A1>,A2> & buf,
                                const DataType & type,
                                const MessageTag & tag) const
{
  // We'll avoid redundant code (at the cost of using heap rather
  // than stack buffer allocation) by reusing the non-blocking send
  Request req;
  this->send(dest_processor_id, buf, type, req, tag);
  req.wait();
}



template <typename T, typename A1, typename A2>
inline void Communicator::send (const unsigned int dest_processor_id,
                                const std::vector<std::vector<T,A1>,A2> & send_vecs,
                                const DataType & type,
                                Request & req,
                                const MessageTag & tag) const
{
  // figure out how many bytes we need to pack all the data
  const CountType sendsize =
    cast_int<CountType>(this->packed_size_of(send_vecs, type));

  // temporary buffer - this will be sized in bytes
  // and manipulated with MPI_Pack
  std::vector<char> * sendbuf = new std::vector<char>(sendsize);

  // Pack the send buffer
  CountType pos=0;

  // ... the size of the outer buffer
  const std::size_t n_vecs = send_vecs.size();
  const CountType mpi_n_vecs = cast_int<CountType>(n_vecs);

  timpi_call_mpi
    (TIMPI_PACK (&mpi_n_vecs, 1, TIMPI_COUNT_TYPE, sendbuf->data(),
                 sendsize, &pos, this->get()));

  for (std::size_t i = 0; i != n_vecs; ++i)
    {
      // ... the size of the ith inner buffer
      const CountType subvec_size =
        cast_int<CountType>(send_vecs[i].size());

      timpi_call_mpi
        (TIMPI_PACK (&subvec_size, 1, TIMPI_COUNT_TYPE,
                     sendbuf->data(), sendsize, &pos, this->get()));

      // ... the contents of the ith inner buffer
      if (!send_vecs[i].empty())
        timpi_call_mpi
          (TIMPI_PACK (const_cast<T*>(send_vecs[i].data()),
                       subvec_size, type, sendbuf->data(), sendsize,
                       &pos, this->get()));
    }

  timpi_assert_equal_to (pos, sendsize);

  req.add_post_wait_work
    (new PostWaitDeleteBuffer<std::vector<char>> (sendbuf));

  this->send (dest_processor_id, *sendbuf, MPI_PACKED, req, tag);
}


template <typename Context, typename Iter>
inline void Communicator::send_packed_range (const unsigned int dest_processor_id,
                                             const Context * context,
                                             Iter range_begin,
                                             const Iter range_end,
                                             const MessageTag & tag,
                                             std::size_t approx_buffer_size) const
{
  // We will serialize variable size objects from *range_begin to
  // *range_end as a sequence of plain data (e.g. ints) in this buffer
  typedef typename std::iterator_traits<Iter>::value_type T;

  std::size_t total_buffer_size =
    packed_range_size (context, range_begin, range_end);

  this->send(dest_processor_id, total_buffer_size, tag);

#ifdef DEBUG
  std::size_t used_buffer_size = 0;
#endif

  while (range_begin != range_end)
    {
      timpi_assert_greater (std::distance(range_begin, range_end), 0);

      std::vector<typename Packing<T>::buffer_type> buffer;

      const Iter next_range_begin = pack_range
        (context, range_begin, range_end, buffer, approx_buffer_size);

      timpi_assert_greater (std::distance(range_begin, next_range_begin), 0);

      range_begin = next_range_begin;

#ifdef DEBUG
      used_buffer_size += buffer.size();
#endif

      // Blocking send of the buffer
      this->send(dest_processor_id, buffer, tag);
    }

#ifdef DEBUG
  timpi_assert_equal_to(used_buffer_size, total_buffer_size);
#endif
}


template <typename Context, typename Iter>
inline void Communicator::send_packed_range (const unsigned int dest_processor_id,
                                             const Context * context,
                                             Iter range_begin,
                                             const Iter range_end,
                                             Request & req,
                                             const MessageTag & tag,
                                             std::size_t approx_buffer_size) const
{
  // Allocate a buffer on the heap so we don't have to free it until
  // after the Request::wait()
  typedef typename std::iterator_traits<Iter>::value_type T;
  typedef typename Packing<T>::buffer_type buffer_t;

  std::size_t total_buffer_size =
    packed_range_size (context, range_begin, range_end);

  // That local variable will be gone soon; we need a send buffer that
  // will stick around.  I heard you like buffering so I put a buffer
  // for your buffer size so you can buffer the size of your buffer.
  std::size_t * total_buffer_size_buffer = new std::size_t;
  *total_buffer_size_buffer = total_buffer_size;

  // Delete the buffer size's buffer when we're done
  Request intermediate_req = request();
  intermediate_req.add_post_wait_work
    (new PostWaitDeleteBuffer<std::size_t>(total_buffer_size_buffer));
  this->send(dest_processor_id, *total_buffer_size_buffer, intermediate_req, tag);

  // And don't finish up the full request until we're done with its
  // dependencies
  req.add_prior_request(intermediate_req);

#ifdef DEBUG
  std::size_t used_buffer_size = 0;
#endif

  while (range_begin != range_end)
    {
      timpi_assert_greater (std::distance(range_begin, range_end), 0);

      std::vector<buffer_t> * buffer = new std::vector<buffer_t>();

      const Iter next_range_begin = pack_range
        (context, range_begin, range_end, *buffer, approx_buffer_size);

      timpi_assert_greater (std::distance(range_begin, next_range_begin), 0);

      range_begin = next_range_begin;

#ifdef DEBUG
      used_buffer_size += buffer->size();
#endif

      Request next_intermediate_req;

      Request * my_req = (range_begin == range_end) ? &req : &next_intermediate_req;

      // Make the Request::wait() handle deleting the buffer
      my_req->add_post_wait_work
        (new PostWaitDeleteBuffer<std::vector<buffer_t>>
         (buffer));

      // Non-blocking send of the buffer
      this->send(dest_processor_id, *buffer, *my_req, tag);

      if (range_begin != range_end)
        req.add_prior_request(*my_req);
    }
}







template <typename Context, typename Iter>
inline void Communicator::nonblocking_send_packed_range (const unsigned int dest_processor_id,
                                                         const Context * context,
                                                         Iter range_begin,
                                                         const Iter range_end,
                                                         Request & req,
                                                         const MessageTag & tag) const
{
  // Allocate a buffer on the heap so we don't have to free it until
  // after the Request::wait()
  typedef typename std::iterator_traits<Iter>::value_type T;
  typedef typename Packing<T>::buffer_type buffer_t;

  if (range_begin != range_end)
    {
      std::vector<buffer_t> * buffer = new std::vector<buffer_t>();

      range_begin =
        pack_range(context,
                   range_begin,
                   range_end,
                   *buffer,
                   // MPI-2/3 can only use signed integers for size,
                   // and with this API we need to fit a non-blocking
                   // send into one buffer
                   std::numeric_limits<CountType>::max());

      if (range_begin != range_end)
        timpi_error_msg("Non-blocking packed range sends cannot exceed " << std::numeric_limits<CountType>::max() << "in size");

      // Make the Request::wait() handle deleting the buffer
      req.add_post_wait_work
        (new PostWaitDeleteBuffer<std::vector<buffer_t>>
         (buffer));

      // Non-blocking send of the buffer
      this->send(dest_processor_id, *buffer, req, tag);
    }
}


template <typename T>
inline Status Communicator::receive (const unsigned int src_processor_id,
                                     std::basic_string<T> & buf,
                                     const MessageTag & tag) const
{
  std::vector<T> tempbuf;  // Officially C++ won't let us get a
                           // modifiable array from a string

  Status stat = this->receive(src_processor_id, tempbuf, tag);
  buf.assign(tempbuf.begin(), tempbuf.end());
  return stat;
}



template <typename T>
inline void Communicator::receive (const unsigned int src_processor_id,
                                   std::basic_string<T> & buf,
                                   Request & req,
                                   const MessageTag & tag) const
{
  // Officially C++ won't let us get a modifiable array from a
  // string, and we can't even put one on the stack for the
  // non-blocking case.
  std::vector<T> * tempbuf = new std::vector<T>();

  // We can clear the string, but the Request::wait() will need to
  // handle copying our temporary buffer to it
  buf.clear();

  req.add_post_wait_work
    (new PostWaitCopyBuffer<std::vector<T>,
     std::back_insert_iterator<std::basic_string<T>>>
     (tempbuf, std::back_inserter(buf)));

  // Make the Request::wait() then handle deleting the buffer
  req.add_post_wait_work
    (new PostWaitDeleteBuffer<std::vector<T>>(tempbuf));

  this->receive(src_processor_id, tempbuf, req, tag);
}



template <typename T>
inline Status Communicator::receive (const unsigned int src_processor_id,
                                     T & buf,
                                     const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("receive()", "Parallel");

  // Get the status of the message, explicitly provide the
  // datatype so we can later query the size
  Status stat(this->probe(src_processor_id, tag), StandardType<T>(&buf));

  timpi_assert(src_processor_id < this->size() ||
                  src_processor_id == any_source);

  timpi_call_mpi
    (TIMPI_RECV (&buf, 1, StandardType<T>(&buf), src_processor_id,
                 tag.value(), this->get(), stat.get()));

  return stat;
}



template <typename T>
inline void Communicator::receive (const unsigned int src_processor_id,
                                   T & buf,
                                   Request & req,
                                   const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("receive()", "Parallel");

  timpi_assert(src_processor_id < this->size() ||
                  src_processor_id == any_source);

  timpi_call_mpi
    (TIMPI_IRECV (&buf, 1, StandardType<T>(&buf), src_processor_id,
                  tag.value(), this->get(), req.get()));

  // The MessageTag should stay registered for the Request lifetime
  req.add_post_wait_work
    (new PostWaitDereferenceTag(tag));
}



template <typename T, typename C, typename A>
inline Status Communicator::receive (const unsigned int src_processor_id,
                                     std::set<T,C,A> & buf,
                                     const MessageTag & tag) const
{
  return this->receive
    (src_processor_id, buf,
     StandardType<T>(buf.empty() ? nullptr : &(*buf.begin())), tag);
}



/*
 * No non-blocking receives of std::set until we figure out how to
 * resize the temporary buffer
 */
#if 0
template <typename T, typename C, typename A>
inline void Communicator::receive (const unsigned int src_processor_id,
                                   std::set<T,C,A> & buf,
                                   Request & req,
                                   const MessageTag & tag) const
{
  this->receive (src_processor_id, buf,
                 StandardType<T>(buf.empty() ? nullptr : &(*buf.begin())), req, tag);
}
#endif // 0



template <typename T, typename C, typename A>
inline Status Communicator::receive (const unsigned int src_processor_id,
                                     std::set<T,C,A> & buf,
                                     const DataType & type,
                                     const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("receive()", "Parallel");

  std::vector<T> vecbuf;
  Status stat = this->receive(src_processor_id, vecbuf, type, tag);
  buf.clear();
  buf.insert(vecbuf.begin(), vecbuf.end());

  return stat;
}



/*
 * No non-blocking receives of std::set until we figure out how to
 * resize the temporary buffer
 */
#if 0
template <typename T, typename C, typename A>
inline void Communicator::receive (const unsigned int src_processor_id,
                                   std::set<T,C,A> & buf,
                                   const DataType & type,
                                   Request & req,
                                   const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("receive()", "Parallel");

  // Allocate temporary buffer on the heap so it lives until after
  // the non-blocking send completes
  std::vector<T> * vecbuf = new std::vector<T>();

  // We can clear the set, but the Request::wait() will need to
  // handle copying our temporary buffer to it
  buf.clear();

  req.add_post_wait_work
    (new PostWaitCopyBuffer<std::vector<T>,
     std::insert_iterator<std::set<T,C,A>>>
     (*vecbuf, std::inserter(buf,buf.end())));

  // Make the Request::wait() then handle deleting the buffer
  req.add_post_wait_work
    (new PostWaitDeleteBuffer<std::vector<T>>(vecbuf));

  this->receive(src_processor_id, *vecbuf, type, req, tag);
}
#endif // 0



template <typename T, typename A>
inline Status Communicator::receive (const unsigned int src_processor_id,
                                     std::vector<T,A> & buf,
                                     const MessageTag & tag) const
{
  return this->receive
    (src_processor_id, buf,
     StandardType<T>(buf.empty() ? nullptr : &(*buf.begin())), tag);
}



template <typename T, typename A>
inline void Communicator::receive (const unsigned int src_processor_id,
                                   std::vector<T,A> & buf,
                                   Request & req,
                                   const MessageTag & tag) const
{
  this->receive (src_processor_id, buf,
                 StandardType<T>(buf.empty() ? nullptr : &(*buf.begin())), req, tag);
}



template <typename T, typename A>
inline Status Communicator::receive (const unsigned int src_processor_id,
                                     std::vector<T,A> & buf,
                                     const DataType & type,
                                     const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("receive()", "Parallel");

  // Get the status of the message, explicitly provide the
  // datatype so we can later query the size
  Status stat(this->probe(src_processor_id, tag), type);

  buf.resize(stat.size());

  timpi_assert(src_processor_id < this->size() ||
                  src_processor_id == any_source);

  // Use stat.source() and stat.tag() in the receive - if
  // src_processor_id is or tag is "any" then we want to be sure we
  // try to receive the same message we just probed.
  timpi_call_mpi
    (TIMPI_RECV (buf.empty() ? nullptr : buf.data(),
                 cast_int<CountType>(buf.size()), type, stat.source(),
                 stat.tag(), this->get(), stat.get()));

  timpi_assert_equal_to (cast_int<std::size_t>(stat.size()),
                         buf.size());

  return stat;
}



template <typename T, typename A,
          typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type>
Status Communicator::receive (const unsigned int src_processor_id,
                              std::vector<T,A> & buf,
                              const DataType & type,
                              const MessageTag & tag) const
{
  bool flag = false;
  Status stat;
  while (!flag)
    stat = this->packed_range_probe<T>(src_processor_id, tag, flag);

  Request req;
  this->nonblocking_receive_packed_range(src_processor_id, (void *)(nullptr),
    std::inserter(buf, buf.end()),
    type, req, stat, tag);
  req.wait();

  return stat;
}


template <typename T, typename A,
          typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type>
Status Communicator::receive (const unsigned int src_processor_id,
                              std::vector<T,A> & buf,
                              const NotADataType &,
                              const MessageTag & tag) const
{
  bool flag = false;
  Status stat;
  while (!flag)
    stat = this->packed_range_probe<T>(src_processor_id, tag, flag);

  Request req;
  this->nonblocking_receive_packed_range(src_processor_id, (void *)(nullptr),
    std::inserter(buf, buf.end()),
    buf.data(), req, stat, tag);
  req.wait();

  return stat;
}


template <typename T, typename A>
inline void Communicator::receive (const unsigned int src_processor_id,
                                   std::vector<T,A> & buf,
                                   const DataType & type,
                                   Request & req,
                                   const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("receive()", "Parallel");

  timpi_assert(src_processor_id < this->size() ||
                  src_processor_id == any_source);

  timpi_call_mpi
    (TIMPI_IRECV(buf.empty() ? nullptr : buf.data(),
                 cast_int<CountType>(buf.size()), type, src_processor_id,
                 tag.value(), this->get(), req.get()));

  // The MessageTag should stay registered for the Request lifetime
  req.add_post_wait_work
    (new PostWaitDereferenceTag(tag));
}



template <typename T, typename A1, typename A2>
inline Status Communicator::receive (const unsigned int src_processor_id,
                                     std::vector<std::vector<T,A1>,A2> & buf,
                                     const MessageTag & tag) const
{
  return this->receive
    (src_processor_id, buf,
     StandardType<T>((buf.empty() || buf.front().empty()) ?
                     nullptr : &(buf.front().front())), tag);
}



template <typename T, typename A1, typename A2>
inline void Communicator::receive (const unsigned int src_processor_id,
                                   std::vector<std::vector<T,A1>,A2> & buf,
                                   Request & req,
                                   const MessageTag & tag) const
{
  this->receive (src_processor_id, buf,
                 StandardType<T>((buf.empty() || buf.front().empty()) ?
                                 nullptr : &(buf.front().front())), req, tag);
}



template <typename T, typename A1, typename A2>
inline Status Communicator::receive (const unsigned int src_processor_id,
                                     std::vector<std::vector<T,A1>,A2> & recv,
                                     const DataType & type,
                                     const MessageTag & tag) const
{
  // temporary buffer - this will be sized in bytes
  // and manipulated with MPI_Unpack
  std::vector<char> recvbuf;

  Status stat = this->receive (src_processor_id, recvbuf, MPI_PACKED, tag);

  // We should at least have one header datum, for outer vector size
  timpi_assert (!recvbuf.empty());

  // Unpack the received buffer
  CountType bufsize = cast_int<CountType>(recvbuf.size());
  CountType recvsize, pos=0;
  timpi_call_mpi
    (TIMPI_UNPACK(recvbuf.data(), bufsize, &pos, &recvsize, 1,
                  TIMPI_COUNT_TYPE, this->get()));

  // ... size the outer buffer
  recv.resize (recvsize);

  const std::size_t n_vecs = recvsize;
  for (std::size_t i = 0; i != n_vecs; ++i)
    {
      CountType subvec_size;

      timpi_call_mpi
        (TIMPI_UNPACK (recvbuf.data(), bufsize, &pos, &subvec_size, 1,
                       TIMPI_COUNT_TYPE, this->get()));

      // ... size the inner buffer
      recv[i].resize (subvec_size);

      // ... unpack the inner buffer if it is not empty
      if (!recv[i].empty())
        timpi_call_mpi
          (TIMPI_UNPACK(recvbuf.data(), bufsize, &pos, recv[i].data(),
                        subvec_size, type, this->get()));
    }

  return stat;
}



template <typename T, typename A1, typename A2>
inline void Communicator::receive (const unsigned int src_processor_id,
                                   std::vector<std::vector<T,A1>,A2> & buf,
                                   const DataType & type,
                                   Request & req,
                                   const MessageTag & tag) const
{
  // figure out how many bytes we need to receive all the data into
  // our properly pre-sized buf
  const CountType sendsize =
    cast_int<CountType>(this->packed_size_of(buf, type));

  // temporary buffer - this will be sized in bytes
  // and manipulated with MPI_Unpack
  std::vector<char> * recvbuf = new std::vector<char>(sendsize);

  // Get ready to receive the temporary buffer
  this->receive (src_processor_id, *recvbuf, MPI_PACKED, req, tag);

  // When we wait on the receive, we'll unpack the temporary buffer
  req.add_post_wait_work
    (new PostWaitUnpackNestedBuffer<std::vector<std::vector<T,A1>,A2>>
       (*recvbuf, buf, type, *this));

  // And then we'll free the temporary buffer
  req.add_post_wait_work
    (new PostWaitDeleteBuffer<std::vector<char>>(recvbuf));

  // The MessageTag should stay registered for the Request lifetime
  req.add_post_wait_work
    (new PostWaitDereferenceTag(tag));
}


template <typename Context, typename OutputIter, typename T>
inline void Communicator::receive_packed_range (const unsigned int src_processor_id,
                                                Context * context,
                                                OutputIter out_iter,
                                                const T * output_type,
                                                const MessageTag & tag) const
{
  typedef typename Packing<T>::buffer_type buffer_t;

  // Receive serialized variable size objects as sequences of buffer_t
  std::size_t total_buffer_size = 0;
  Status stat = this->receive(src_processor_id, total_buffer_size, tag);

  // Use stat.source() and stat.tag() in subsequent receives - if
  // src_processor_id is or tag is "any" then we want to be sure we
  // try to receive messages all corresponding to the same send.

  std::size_t received_buffer_size = 0;

  // OutputIter might not have operator= implemented; for maximum
  // compatibility we'll rely on its copy constructor.
  std::unique_ptr<OutputIter> next_out_iter =
    std::make_unique<OutputIter>(out_iter);

  while (received_buffer_size < total_buffer_size)
    {
      std::vector<buffer_t> buffer;
      this->receive(stat.source(), buffer, MessageTag(stat.tag()));
      received_buffer_size += buffer.size();
      auto return_out_iter = unpack_range
        (buffer, context, *next_out_iter, output_type);
      next_out_iter = std::make_unique<OutputIter>(return_out_iter);
    }
}



// template <typename Context, typename OutputIter>
// inline void Communicator::receive_packed_range (const unsigned int src_processor_id,
//                                                 Context * context,
//                                                 OutputIter out_iter,
//                                                 Request & req,
//                                                 const MessageTag & tag) const
// {
//   typedef typename std::iterator_traits<OutputIter>::value_type T;
//   typedef typename Packing<T>::buffer_type buffer_t;
//
//   // Receive serialized variable size objects as a sequence of
//   // buffer_t.
//   // Allocate a buffer on the heap so we don't have to free it until
//   // after the Request::wait()
//   std::vector<buffer_t> * buffer = new std::vector<buffer_t>();
//   this->receive(src_processor_id, *buffer, req, tag);
//
//   // Make the Request::wait() handle unpacking the buffer
//   req.add_post_wait_work
//     (new PostWaitUnpackBuffer<std::vector<buffer_t>, Context, OutputIter>
//      (buffer, context, out_iter));
//
//   // Make the Request::wait() then handle deleting the buffer
//   req.add_post_wait_work
//     (new PostWaitDeleteBuffer<std::vector<buffer_t>>(buffer));
// }

template <typename Context, typename OutputIter, typename T>
inline void Communicator::nonblocking_receive_packed_range (const unsigned int src_processor_id,
                                                            Context * context,
                                                            OutputIter out,
                                                            const T * /* output_type */,
                                                            Request & req,
                                                            Status & stat,
                                                            const MessageTag & tag) const
{
  typedef typename Packing<T>::buffer_type buffer_t;

  // Receive serialized variable size objects as a sequence of
  // buffer_t.
  // Allocate a buffer on the heap so we don't have to free it until
  // after the Request::wait()
  std::vector<buffer_t> * buffer = new std::vector<buffer_t>(stat.size());
  this->receive(src_processor_id, *buffer, req, tag);

  // Make the Request::wait() handle unpacking the buffer
  req.add_post_wait_work
    (new PostWaitUnpackBuffer<std::vector<buffer_t>, Context, OutputIter, T>(*buffer, context, out));

  // Make the Request::wait() then handle deleting the buffer
  req.add_post_wait_work
    (new PostWaitDeleteBuffer<std::vector<buffer_t>>(buffer));

  // The MessageTag should stay registered for the Request lifetime
  req.add_post_wait_work
    (new PostWaitDereferenceTag(tag));
}



template <typename T1, typename T2, typename A1, typename A2>
inline void Communicator::send_receive(const unsigned int dest_processor_id,
                                       const std::vector<T1,A1> & sendvec,
                                       const DataType & type1,
                                       const unsigned int source_processor_id,
                                       std::vector<T2,A2> & recv,
                                       const DataType & type2,
                                       const MessageTag & send_tag,
                                       const MessageTag & recv_tag) const
{
  TIMPI_LOG_SCOPE("send_receive()", "Parallel");

  if (dest_processor_id   == this->rank() &&
      source_processor_id == this->rank())
    {
      recv = sendvec;
      return;
    }

  Request req;

  this->send (dest_processor_id, sendvec, type1, req, send_tag);

  this->receive (source_processor_id, recv, type2, recv_tag);

  req.wait();
}


template <typename T1, typename T2, typename A1, typename A2,
          typename std::enable_if<Has_buffer_type<Packing<T1>>::value &&
                                  Has_buffer_type<Packing<T2>>::value, int>::type>
inline
void
Communicator::send_receive(const unsigned int dest_processor_id,
                           const std::vector<T1,A1> & send_data,
                           const unsigned int source_processor_id,
                           std::vector<T2,A2> &recv_data,
                           const MessageTag &send_tag,
                           const MessageTag &recv_tag) const
{
  this->send_receive_packed_range(dest_processor_id, (void *)(nullptr),
                                  send_data.begin(), send_data.end(),
                                  source_processor_id, (void *)(nullptr),
                                  std::back_inserter(recv_data),
                                  (const T2 *)(nullptr),
                                  send_tag, recv_tag);
}


template <typename T, typename A,
          typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type>
inline
void
Communicator::send_receive(const unsigned int dest_processor_id,
                           const std::vector<T,A> & send_data,
                           const unsigned int source_processor_id,
                           std::vector<T,A> &recv_data,
                           const MessageTag &send_tag,
                           const MessageTag &recv_tag) const
{
  this->send_receive_packed_range(dest_processor_id, (void *)(nullptr),
                                  send_data.begin(), send_data.end(),
                                  source_processor_id, (void *)(nullptr),
                                  std::back_inserter(recv_data),
                                  (const T *)(nullptr),
                                  send_tag, recv_tag);
}



template <typename T1, typename T2,
          typename std::enable_if<std::is_base_of<DataType, StandardType<T1>>::value &&
                                  std::is_base_of<DataType, StandardType<T2>>::value,
                                  int>::type>
inline void Communicator::send_receive(const unsigned int dest_processor_id,
                                       const T1 & sendvec,
                                       const unsigned int source_processor_id,
                                       T2 & recv,
                                       const MessageTag & send_tag,
                                       const MessageTag & recv_tag) const
{
  TIMPI_LOG_SCOPE("send_receive()", "Parallel");

  if (dest_processor_id   == this->rank() &&
      source_processor_id == this->rank())
    {
      recv = sendvec;
      return;
    }

  timpi_assert_less(dest_processor_id, this->size());
  timpi_assert(source_processor_id < this->size() ||
                  source_processor_id == any_source);

  // MPI_STATUS_IGNORE is from MPI-2; using it with some versions of
  // MPICH may cause a crash:
  // https://bugzilla.mcs.anl.gov/globus/show_bug.cgi?id=1798
  timpi_call_mpi
    (TIMPI_SENDRECV(const_cast<T1*>(&sendvec), 1, StandardType<T1>(&sendvec),
                    dest_processor_id, send_tag.value(), &recv, 1,
                    StandardType<T2>(&recv), source_processor_id,
                    recv_tag.value(), this->get(), MPI_STATUS_IGNORE));
}



// This is both a declaration and definition for a new overloaded
// function template, so we have to re-specify the default
// arguments.
//
// We specialize on the T1==T2 case so that we can handle
// send_receive-to-self with a plain copy rather than going through
// MPI.
template <typename T, typename A,
          typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value,
                                  int>::type>
inline void Communicator::send_receive(const unsigned int dest_processor_id,
                                       const std::vector<T,A> & sendvec,
                                       const unsigned int source_processor_id,
                                       std::vector<T,A> & recv,
                                       const MessageTag & send_tag,
                                       const MessageTag & recv_tag) const
{
  if (dest_processor_id   == this->rank() &&
      source_processor_id == this->rank())
    {
      TIMPI_LOG_SCOPE("send_receive()", "Parallel");
      recv = sendvec;
      return;
    }

  const T* example = sendvec.empty() ?
    (recv.empty() ? nullptr : recv.data()) : sendvec.data();

  // Call the user-defined type version with automatic
  // type conversion based on template argument:
  this->send_receive (dest_processor_id, sendvec,
                      StandardType<T>(example),
                      source_processor_id, recv,
                      StandardType<T>(example),
                      send_tag, recv_tag);
}


template <typename T1, typename T2, typename A1, typename A2,
          typename std::enable_if<std::is_base_of<DataType, StandardType<T1>>::value &&
                                  std::is_base_of<DataType, StandardType<T2>>::value,
                                  int>::type>
inline void Communicator::send_receive(const unsigned int dest_processor_id,
                                       const std::vector<T1,A1> & sendvec,
                                       const unsigned int source_processor_id,
                                       std::vector<T2,A2> & recv,
                                       const MessageTag & send_tag,
                                       const MessageTag & recv_tag) const
{
  // Call the user-defined type version with automatic
  // type conversion based on template argument:
  this->send_receive (dest_processor_id, sendvec,
                      StandardType<T1>(sendvec.empty() ? nullptr : sendvec.data()),
                      source_processor_id, recv,
                      StandardType<T2>(recv.empty() ? nullptr : recv.data()),
                      send_tag, recv_tag);
}




template <typename T1, typename T2, typename A1, typename A2, typename A3, typename A4>
inline void Communicator::send_receive(const unsigned int dest_processor_id,
                                       const std::vector<std::vector<T1,A1>,A2> & sendvec,
                                       const unsigned int source_processor_id,
                                       std::vector<std::vector<T2,A3>,A4> & recv,
                                       const MessageTag & /* send_tag */,
                                       const MessageTag & /* recv_tag */) const
{
  // FIXME - why aren't we honoring send_tag and recv_tag here?
  send_receive_vec_of_vec
    (dest_processor_id, sendvec, source_processor_id, recv,
     no_tag, any_tag, *this);
}



// This is both a declaration and definition for a new overloaded
// function template, so we have to re-specify the default arguments
template <typename T, typename A1, typename A2>
inline void Communicator::send_receive(const unsigned int dest_processor_id,
                                       const std::vector<std::vector<T,A1>,A2> & sendvec,
                                       const unsigned int source_processor_id,
                                       std::vector<std::vector<T,A1>,A2> & recv,
                                       const MessageTag & send_tag,
                                       const MessageTag & recv_tag) const
{
  send_receive_vec_of_vec
    (dest_processor_id, sendvec, source_processor_id, recv,
     send_tag, recv_tag, *this);
}




template <typename Context1, typename RangeIter, typename Context2,
          typename OutputIter, typename T>
inline void
Communicator::send_receive_packed_range (const unsigned int dest_processor_id,
                                         const Context1 * context1,
                                         RangeIter send_begin,
                                         const RangeIter send_end,
                                         const unsigned int source_processor_id,
                                         Context2 * context2,
                                         OutputIter out_iter,
                                         const T * output_type,
                                         const MessageTag & send_tag,
                                         const MessageTag & recv_tag,
                                         std::size_t approx_buffer_size) const
{
  TIMPI_LOG_SCOPE("send_receive()", "Parallel");

  timpi_assert_equal_to
    ((dest_processor_id  == this->rank()),
     (source_processor_id == this->rank()));

  if (dest_processor_id   == this->rank() &&
      source_processor_id == this->rank())
    {
      // We need to pack and unpack, even if we don't need to
      // communicate the buffer, just in case user Packing
      // specializations have side effects

      // OutputIter might not have operator= implemented; for maximum
      // compatibility we'll rely on its copy constructor.
      std::unique_ptr<OutputIter> next_out_iter =
        std::make_unique<OutputIter>(out_iter);

      typedef typename Packing<T>::buffer_type buffer_t;
      while (send_begin != send_end)
        {
          std::vector<buffer_t> buffer;
          send_begin = pack_range
            (context1, send_begin, send_end, buffer, approx_buffer_size);
          auto return_out_iter = unpack_range
            (buffer, context2, *next_out_iter, output_type);
          next_out_iter = std::make_unique<OutputIter>(return_out_iter);
        }
      return;
    }

  Request req;

  this->send_packed_range (dest_processor_id, context1, send_begin, send_end,
                           req, send_tag, approx_buffer_size);

  this->receive_packed_range (source_processor_id, context2, out_iter,
                              output_type, recv_tag);

  req.wait();
}



template <typename Context, typename Iter>
inline void Communicator::nonblocking_send_packed_range (const unsigned int dest_processor_id,
                                                         const Context * context,
                                                         Iter range_begin,
                                                         const Iter range_end,
                                                         Request & req,
                                                         std::shared_ptr<std::vector<typename Packing<typename std::iterator_traits<Iter>::value_type>::buffer_type>> & buffer,
                                                         const MessageTag & tag) const
{
  // Allocate a buffer on the heap so we don't have to free it until
  // after the Request::wait()
  typedef typename std::iterator_traits<Iter>::value_type T;
  typedef typename Packing<T>::buffer_type buffer_t;

  if (range_begin != range_end)
    {
      if (buffer == nullptr)
        buffer = std::make_shared<std::vector<buffer_t>>();
      else
        buffer->clear();

      range_begin =
        pack_range(context,
                   range_begin,
                   range_end,
                   *buffer,
                   // MPI-2/3 can only use signed integers for size,
                   // and with this API we need to fit a non-blocking
                   // send into one buffer
                   std::numeric_limits<CountType>::max());

      if (range_begin != range_end)
        timpi_error_msg("Non-blocking packed range sends cannot exceed " << std::numeric_limits<CountType>::max() << "in size");

      // Make it dereference the shared pointer (possibly freeing the buffer)
      req.add_post_wait_work
        (new PostWaitDereferenceSharedPtr<std::vector<buffer_t>>(buffer));

      // Non-blocking send of the buffer
      this->send(dest_processor_id, *buffer, req, tag);
    }
}



template <typename T, typename A>
inline void Communicator::allgather(const std::basic_string<T> & sendval,
                                    std::vector<std::basic_string<T>,A> & recv,
                                    const bool identical_buffer_sizes) const
{
  TIMPI_LOG_SCOPE ("allgather()","Parallel");

  timpi_assert(this->size());
  recv.assign(this->size(), "");

  // serial case
  if (this->size() < 2)
    {
      recv.resize(1);
      recv[0] = sendval;
      return;
    }

  std::vector<CountType>
    sendlengths  (this->size(), 0);
  std::vector<DispType>
    displacements(this->size(), 0);

  const CountType mysize = cast_int<CountType>(sendval.size());

  if (identical_buffer_sizes)
    sendlengths.assign(this->size(), mysize);
  else
    // first comm step to determine buffer sizes from all processors
    this->allgather(mysize, sendlengths);

  // Find the total size of the final array and
  // set up the displacement offsets for each processor
  CountType globalsize = 0;
  for (unsigned int i=0; i != this->size(); ++i)
    {
      displacements[i] = globalsize;
      globalsize += sendlengths[i];
    }

  // Check for quick return
  if (globalsize == 0)
    return;

  // monolithic receive buffer
  std::basic_string<T> r(globalsize, 0);

  // and get the data from the remote processors.
  timpi_call_mpi
    (TIMPI_ALLGATHERV(const_cast<T*>(mysize ? sendval.data() : nullptr),
                      mysize, StandardType<T>(),
                      &r[0], sendlengths.data(), displacements.data(),
                      StandardType<T>(), this->get()));

  // slice receive buffer up
  for (unsigned int i=0; i != this->size(); ++i)
    recv[i] = r.substr(displacements[i], sendlengths[i]);
}



inline void Communicator::broadcast (bool & data,
                                     const unsigned int root_id,
                                     const bool /* identical_sizes */) const
{
  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      return;
    }

  timpi_assert_less (root_id, this->size());

  TIMPI_LOG_SCOPE("broadcast()", "Parallel");

  // We don't want to depend on MPI-2 or C++ MPI, so we don't have
  // MPI::BOOL available
  char char_data = data;

  timpi_assert_less(root_id, this->size());

  // Spread data to remote processors.
  timpi_call_mpi
    (TIMPI_BCAST (&char_data, 1, StandardType<char>(&char_data),
                  root_id, this->get()));

  data = char_data;
}


template <typename T>
inline void Communicator::broadcast (std::basic_string<T> & data,
                                     const unsigned int root_id,
                                     const bool identical_sizes) const
{
  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      return;
    }

  timpi_assert_less (root_id, this->size());
  timpi_assert (this->verify(identical_sizes));

  TIMPI_LOG_SCOPE("broadcast()", "Parallel");

  std::size_t data_size = data.size();

  if (identical_sizes)
    timpi_assert(this->verify(data_size));
  else
    this->broadcast(data_size, root_id);

  std::vector<T> data_c(data_size);
#ifndef NDEBUG
  std::basic_string<T> orig(data);
#endif

  if (this->rank() == root_id)
    for (std::size_t i=0; i<data.size(); i++)
      data_c[i] = data[i];

  this->broadcast (data_c, root_id, StandardType<T>::is_fixed_type);

  data.assign(data_c.begin(), data_c.end());

#ifndef NDEBUG
  if (this->rank() == root_id)
    timpi_assert_equal_to (data, orig);
#endif
}


template <typename T, typename A>
inline void Communicator::broadcast (std::vector<std::basic_string<T>,A> & data,
                                     const unsigned int root_id,
                                     const bool identical_sizes) const
{
  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      return;
    }

  timpi_assert_less (root_id, this->size());
  timpi_assert (this->verify(identical_sizes));

  TIMPI_LOG_SCOPE("broadcast()", "Parallel");

  std::size_t bufsize=0;
  if (root_id == this->rank() || identical_sizes)
    {
      for (std::size_t i=0; i<data.size(); ++i)
        bufsize += data[i].size() + 1;  // Add one for the string length word
    }

  if (identical_sizes)
    timpi_assert(this->verify(bufsize));
  else
    this->broadcast(bufsize, root_id);

  // Here we use unsigned int to store up to 32-bit characters
  std::vector<unsigned int> temp; temp.reserve(bufsize);
  // Pack the strings
  if (root_id == this->rank())
    {
      for (std::size_t i=0; i<data.size(); ++i)
        {
          temp.push_back(cast_int<unsigned int>(data[i].size()));
          for (std::size_t j=0; j != data[i].size(); ++j)
            /**
             * The strings will be packed in one long array with the size of each
             * string preceding the actual characters
             */
            temp.push_back(data[i][j]);
        }
    }
  else
    temp.resize(bufsize);

  // broad cast the packed strings
  this->broadcast(temp, root_id, true);

  // Unpack the strings
  if (root_id != this->rank())
    {
      data.clear();
      typename std::vector<unsigned int>::const_iterator iter = temp.begin();
      while (iter != temp.end())
        {
          std::size_t curr_len = *iter++;
          data.push_back(std::basic_string<T>(iter, iter+curr_len));
          iter += curr_len;
        }
    }
}



template <typename T, typename A1, typename A2>
inline void Communicator::broadcast (std::vector<std::vector<T,A1>,A2> & data,
                                     const unsigned int root_id,
                                     const bool identical_sizes) const
{
  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      return;
    }

  timpi_assert_less (root_id, this->size());
  timpi_assert (this->verify(identical_sizes));

  TIMPI_LOG_SCOPE("broadcast()", "Parallel");

  std::size_t size_sizes = data.size();
  if (identical_sizes)
    timpi_assert(this->verify(size_sizes));
  else
    this->broadcast(size_sizes, root_id);
  std::vector<std::size_t> sizes(size_sizes);

  if (root_id == this->rank() || identical_sizes)
    for (std::size_t i=0; i<size_sizes; ++i)
      sizes[i] = data[i].size();

  if (identical_sizes)
    timpi_assert(this->verify(sizes));
  else
    this->broadcast(sizes, root_id);

  std::size_t bufsize = 0;
  for (std::size_t i=0; i<size_sizes; ++i)
    bufsize += sizes[i];

  std::vector<T> temp; temp.reserve(bufsize);
  // Pack the vectors
  if (root_id == this->rank())
    {
      // The data will be packed in one long array
      for (std::size_t i=0; i<size_sizes; ++i)
        temp.insert(temp.end(), data[i].begin(), data[i].end());
    }
  else
    temp.resize(bufsize);

  // broad cast the packed data
  this->broadcast(temp, root_id, StandardType<T>::is_fixed_type);

  // Unpack the data
  if (root_id != this->rank())
    {
      data.clear();
      data.resize(size_sizes);
      typename std::vector<T>::const_iterator iter = temp.begin();
      for (std::size_t i=0; i<size_sizes; ++i)
        {
          data[i].insert(data[i].end(), iter, iter+sizes[i]);
          iter += sizes[i];
        }
    }
}




template <typename T, typename C, typename A>
inline void Communicator::broadcast (std::set<T,C,A> & data,
                                     const unsigned int root_id,
                                     const bool identical_sizes) const
{
  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      return;
    }

  timpi_assert_less (root_id, this->size());
  timpi_assert (this->verify(identical_sizes));

  TIMPI_LOG_SCOPE("broadcast()", "Parallel");

  std::vector<T> vecdata;
  if (this->rank() == root_id)
    vecdata.assign(data.begin(), data.end());

  std::size_t vecsize = vecdata.size();
  if (identical_sizes)
    timpi_assert(this->verify(vecsize));
  else
    this->broadcast(vecsize, root_id);
  if (this->rank() != root_id)
    vecdata.resize(vecsize);

  this->broadcast(vecdata, root_id, StandardType<T>::is_fixed_type);
  if (this->rank() != root_id)
    {
      data.clear();
      data.insert(vecdata.begin(), vecdata.end());
    }
}


template <typename Context, typename OutputIter, typename T>
inline void Communicator::nonblocking_receive_packed_range (const unsigned int src_processor_id,
                                                            Context * context,
                                                            OutputIter out,
                                                            const T * /*output_type*/,
                                                            Request & req,
                                                            Status & stat,
                                                            std::shared_ptr<std::vector<typename Packing<T>::buffer_type>> & buffer,
                                                            const MessageTag & tag) const
{
  // If they didn't pass in a buffer - let's make one
  if (buffer == nullptr)
    buffer = std::make_shared<std::vector<typename Packing<T>::buffer_type>>();
  else
    buffer->clear();

  // Receive serialized variable size objects as a sequence of
  // buffer_t.
  // Allocate a buffer on the heap so we don't have to free it until
  // after the Request::wait()
  buffer->resize(stat.size());
  this->receive(src_processor_id, *buffer, req, tag);

  // Make the Request::wait() handle unpacking the buffer
  req.add_post_wait_work
    (new PostWaitUnpackBuffer<std::vector<typename Packing<T>::buffer_type>, Context, OutputIter, T>(*buffer, context, out));

  // Make it dereference the shared pointer (possibly freeing the buffer)
  req.add_post_wait_work
    (new PostWaitDereferenceSharedPtr<std::vector<typename Packing<T>::buffer_type>>(buffer));
}



template <typename T, typename A, typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type>
inline bool Communicator::possibly_receive (unsigned int & src_processor_id,
                                            std::vector<T,A> & buf,
                                            const DataType & type,
                                            Request & req,
                                            const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("possibly_receive()", "Parallel");

  Status stat(type);

  int int_flag = 0;

  timpi_assert(src_processor_id < this->size() ||
                  src_processor_id == any_source);

  timpi_call_mpi(MPI_Iprobe(int(src_processor_id),
                            tag.value(),
                            this->get(),
                            &int_flag,
                            stat.get()));

  if (int_flag)
  {
    buf.resize(stat.size());

    src_processor_id = stat.source();

    timpi_call_mpi
      (TIMPI_IRECV(buf.data(), cast_int<CountType>(buf.size()), type,
                   src_processor_id, tag.value(), this->get(),
                   req.get()));

    // The MessageTag should stay registered for the Request lifetime
    req.add_post_wait_work
      (new PostWaitDereferenceTag(tag));
  }

  return int_flag;
}

template <typename T, typename A, typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type>
inline bool Communicator::possibly_receive (unsigned int & src_processor_id,
                                            std::vector<T,A> & buf,
                                            const NotADataType &,
                                            Request & req,
                                            const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("possibly_receive()", "Parallel");

  return this->possibly_receive_packed_range(src_processor_id,
                                             (void *)(nullptr),
                                             std::inserter(buf, buf.end()),
                                             (T *)(nullptr),
                                             req,
                                             tag);
}



template <typename T, typename A1, typename A2>
inline bool Communicator::possibly_receive (unsigned int & src_processor_id,
                                            std::vector<std::vector<T,A1>,A2> & buf,
                                            const DataType & type,
                                            Request & req,
                                            const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("possibly_receive()", "Parallel");

  Status stat(type);

  int int_flag = 0;

  timpi_assert(src_processor_id < this->size() ||
                  src_processor_id == any_source);

  timpi_call_mpi(MPI_Iprobe(int(src_processor_id),
                            tag.value(),
                            this->get(),
                            &int_flag,
                            stat.get()));

  if (int_flag)
  {
    src_processor_id = stat.source();

    std::vector<char> * recvbuf =
      new std::vector<char>(stat.size(StandardType<char>()));

    this->receive(src_processor_id, *recvbuf, MPI_PACKED, req, tag);

    // When we wait on the receive, we'll unpack the temporary buffer
    req.add_post_wait_work
      (new PostWaitUnpackNestedBuffer<std::vector<std::vector<T,A1>,A2>>
         (*recvbuf, buf, type, *this));

    // And then we'll free the temporary buffer
    req.add_post_wait_work
      (new PostWaitDeleteBuffer<std::vector<char>>(recvbuf));

    // The MessageTag should stay registered for the Request lifetime
    req.add_post_wait_work
      (new PostWaitDereferenceTag(tag));
  }

  return int_flag;
}

#else
  typedef int DispType;
#endif // TIMPI_HAVE_MPI


// Some of our methods are implemented indirectly via other
// MPI-encapsulated methods and the implementation works with or
// without MPI.
//
// Other methods have a "this->size() == 1" shortcut which still
// applies in the without-MPI case, and the timpi_call_mpi macro
// becomes a no-op so wrapped MPI methods still compile without MPI

template <typename T>
inline bool Communicator::verify(const T & r) const
{
  if (this->size() > 1 && Attributes<T>::has_min_max == true)
    {
      T tempmin = r, tempmax = r;
      this->min(tempmin);
      this->max(tempmax);
      bool verified = (r == tempmin) &&
        (r == tempmax);
      this->min(verified);
      return verified;
    }

  static_assert(Attributes<T>::has_min_max,
                "Tried to verify an unverifiable type");

  return true;
}

template <typename T>
inline bool Communicator::semiverify(const T * r) const
{
  if (this->size() > 1 && Attributes<T>::has_min_max == true)
    {
      T tempmin, tempmax;
      if (r)
        tempmin = tempmax = *r;
      else
        {
          Attributes<T>::set_highest(tempmin);
          Attributes<T>::set_lowest(tempmax);
        }
      this->min(tempmin);
      this->max(tempmax);
      bool invalid = r && ((*r != tempmin) ||
                           (*r != tempmax));
      this->max(invalid);
      return !invalid;
    }

  static_assert(Attributes<T>::has_min_max,
                "Tried to semiverify an unverifiable type");

  return true;
}



template <typename T, typename A>
inline bool Communicator::semiverify(const std::vector<T,A> * r) const
{
  if (this->size() > 1 && Attributes<T>::has_min_max == true)
    {
      std::size_t rsize = r ? r->size() : 0;
      std::size_t * psize = r ? &rsize : nullptr;

      if (!this->semiverify(psize))
        return false;

      this->max(rsize);

      std::vector<T,A> tempmin, tempmax;
      if (r)
        {
          tempmin = tempmax = *r;
        }
      else
        {
          tempmin.resize(rsize);
          tempmax.resize(rsize);
          Attributes<std::vector<T,A>>::set_highest(tempmin);
          Attributes<std::vector<T,A>>::set_lowest(tempmax);
        }
      this->min(tempmin);
      this->max(tempmax);
      bool invalid = r && ((*r != tempmin) ||
                           (*r != tempmax));
      this->max(invalid);
      return !invalid;
    }

  static_assert(Attributes<T>::has_min_max,
                "Tried to semiverify a vector of an unverifiable type");

  return true;
}




template <typename T>
inline void Communicator::min(const T & r,
                              T & o,
                              Request & req) const
{
  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE("min()", "Parallel");

      timpi_call_mpi
        (TIMPI_IALLREDUCE(&r, &o, 1, StandardType<T>(&r),
                          OpFunction<T>::min(), this->get(),
                          req.get()));
    }
  else
    {
      o = r;
      req = Request::null_request;
    }
}



template <typename T>
inline void Communicator::min(T & timpi_mpi_var(r)) const
{
  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE("min(scalar)", "Parallel");

      timpi_call_mpi
        (TIMPI_ALLREDUCE(MPI_IN_PLACE, &r, 1,
                         StandardType<T>(&r), OpFunction<T>::min(),
                         this->get()));
    }
}



template <typename T, typename A>
inline void Communicator::min(std::vector<T,A> & r) const
{
  if (this->size() > 1 && !r.empty())
    {
      TIMPI_LOG_SCOPE("min(vector)", "Parallel");

      timpi_assert(this->verify(r.size()));

      timpi_call_mpi
        (TIMPI_ALLREDUCE
          (MPI_IN_PLACE, r.data(), cast_int<CountType>(r.size()),
           StandardType<T>(r.data()), OpFunction<T>::min(),
           this->get()));
    }
}


template <typename A>
inline void Communicator::min(std::vector<bool,A> & r) const
{
  if (this->size() > 1 && !r.empty())
    {
      TIMPI_LOG_SCOPE("min(vector<bool>)", "Parallel");

      timpi_assert(this->verify(r.size()));

      std::vector<unsigned int> ruint;
      pack_vector_bool(r, ruint);
      std::vector<unsigned int> temp(ruint.size());
      timpi_call_mpi
        (TIMPI_ALLREDUCE
          (ruint.data(), temp.data(),
           cast_int<CountType>(ruint.size()),
           StandardType<unsigned int>(), MPI_BAND, this->get()));
      unpack_vector_bool(temp, r);
    }
}


template <typename T>
inline void Communicator::minloc(T & r,
                                 unsigned int & min_id) const
{
  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE("minloc(scalar)", "Parallel");

      DataPlusInt<T> data_in;
      ignore(data_in); // unused ifndef TIMPI_HAVE_MPI
      data_in.val = r;
      data_in.rank = this->rank();

      timpi_call_mpi
        (TIMPI_ALLREDUCE (MPI_IN_PLACE, &data_in, 1,
                          dataplusint_type_acquire<T>().first,
                          OpFunction<T>::min_location(), this->get()));
      r = data_in.val;
      min_id = data_in.rank;
    }
  else
    min_id = this->rank();
}


template <typename T, typename A1, typename A2>
inline void Communicator::minloc(std::vector<T,A1> & r,
                                 std::vector<unsigned int,A2> & min_id) const
{
  if (this->size() > 1 && !r.empty())
    {
      TIMPI_LOG_SCOPE("minloc(vector)", "Parallel");

      timpi_assert(this->verify(r.size()));

      std::vector<DataPlusInt<T>> data_in(r.size());
      for (std::size_t i=0; i != r.size(); ++i)
        {
          data_in[i].val  = r[i];
          data_in[i].rank = this->rank();
        }
      std::vector<DataPlusInt<T>> data_out(r.size());

      timpi_call_mpi
        (TIMPI_ALLREDUCE (data_in.data(), data_out.data(),
                          cast_int<CountType>(r.size()),
                          dataplusint_type_acquire<T>().first,
                          OpFunction<T>::min_location(), this->get()));
      for (std::size_t i=0; i != r.size(); ++i)
        {
          r[i]      = data_out[i].val;
          min_id[i] = data_out[i].rank;
        }
    }
  else if (!r.empty())
    {
      for (std::size_t i=0; i != r.size(); ++i)
        min_id[i] = this->rank();
    }
}


template <typename A1, typename A2>
inline void Communicator::minloc(std::vector<bool,A1> & r,
                                 std::vector<unsigned int,A2> & min_id) const
{
  if (this->size() > 1 && !r.empty())
    {
      TIMPI_LOG_SCOPE("minloc(vector<bool>)", "Parallel");

      timpi_assert(this->verify(r.size()));

      std::vector<DataPlusInt<int>> data_in(r.size());
      for (std::size_t i=0; i != r.size(); ++i)
        {
          data_in[i].val  = r[i];
          data_in[i].rank = this->rank();
        }
      std::vector<DataPlusInt<int>> data_out(r.size());
      timpi_call_mpi
        (TIMPI_ALLREDUCE
          (data_in.data(), data_out.data(),
           cast_int<CountType>(r.size()), StandardType<int>(),
           OpFunction<int>::min_location(), this->get()));
      for (std::size_t i=0; i != r.size(); ++i)
        {
          r[i]      = data_out[i].val;
          min_id[i] = data_out[i].rank;
        }
    }
  else if (!r.empty())
    {
      for (std::size_t i=0; i != r.size(); ++i)
        min_id[i] = this->rank();
    }
}


template <typename T>
inline void Communicator::max(const T & r,
                              T & o,
                              Request & req) const
{
  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE("max()", "Parallel");

      timpi_call_mpi
        (TIMPI_IALLREDUCE(&r, &o, 1, StandardType<T>(&r),
                          OpFunction<T>::max(), this->get(),
                          req.get()));
    }
  else
    {
      o = r;
      req = Request::null_request;
    }
}


template <typename T>
inline void Communicator::max(T & timpi_mpi_var(r)) const
{
  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE("max(scalar)", "Parallel");

      timpi_call_mpi
        (TIMPI_ALLREDUCE (MPI_IN_PLACE, &r, 1, StandardType<T>(&r),
                          OpFunction<T>::max(), this->get()));
    }
}


template <typename T, typename A>
inline void Communicator::max(std::vector<T,A> & r) const
{
  if (this->size() > 1 && !r.empty())
    {
      TIMPI_LOG_SCOPE("max(vector)", "Parallel");

      timpi_assert(this->verify(r.size()));

      timpi_call_mpi
        (TIMPI_ALLREDUCE (MPI_IN_PLACE, r.data(),
                          cast_int<CountType>(r.size()),
                          StandardType<T>(r.data()),
                          OpFunction<T>::max(), this->get()));
    }
}


template <typename A>
inline void Communicator::max(std::vector<bool,A> & r) const
{
  if (this->size() > 1 && !r.empty())
    {
      TIMPI_LOG_SCOPE("max(vector<bool>)", "Parallel");

      timpi_assert(this->verify(r.size()));

      std::vector<unsigned int> ruint;
      pack_vector_bool(r, ruint);
      std::vector<unsigned int> temp(ruint.size());
      timpi_call_mpi
        (TIMPI_ALLREDUCE (ruint.data(), temp.data(),
                          cast_int<CountType>(ruint.size()),
                          StandardType<unsigned int>(), MPI_BOR,
                          this->get()));
      unpack_vector_bool(temp, r);
    }
}



template <typename Map,
          typename std::enable_if<std::is_base_of<DataType, StandardType<typename Map::key_type>>::value &&
                                  std::is_base_of<DataType, StandardType<typename Map::mapped_type>>::value,
                                  int>::type>
void Communicator::map_max(Map & data) const
{
  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE("max(map)", "Parallel");

      // Since the input map may have different keys on different
      // processors, we first gather all the keys and values, then for
      // each key we choose the max value over all procs. We
      // initialize the max with the first value we encounter rather
      // than some "global" minimum, since the latter is difficult to
      // do generically.
      std::vector<std::pair<typename Map::key_type, typename Map::mapped_type>>
        vecdata(data.begin(), data.end());

      this->allgather(vecdata, /*identical_buffer_sizes=*/false);

      data.clear();

      for (const auto & pr : vecdata)
        {
          // Attempt to insert this value. If it works, then the value didn't
          // already exist and we can go on. If it fails, compute the std::max
          // between the current and existing values.
          auto result = data.insert(pr);

          bool inserted = result.second;

          if (!inserted)
            {
              auto it = result.first;
              it->second = std::max(it->second, pr.second);
            }
        }
    }
}



template <typename Map,
          typename std::enable_if<!(std::is_base_of<DataType, StandardType<typename Map::key_type>>::value &&
                                    std::is_base_of<DataType, StandardType<typename Map::mapped_type>>::value),
                                  int>::type>
void Communicator::map_max(Map & data) const
{
  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE("max(map)", "Parallel");

      // Since the input map may have different keys on different
      // processors, we first gather all the keys and values, then for
      // each key we choose the max value over all procs. We
      // initialize the max with the first value we encounter rather
      // than some "global" minimum, since the latter is difficult to
      // do generically.
      std::vector<typename Map::key_type> keys;
      std::vector<typename Map::mapped_type> vals;

      auto data_size = data.size();
      keys.reserve(data_size);
      vals.reserve(data_size);

      for (const auto & pr : data)
        {
          keys.push_back(pr.first);
          vals.push_back(pr.second);
        }

      this->allgather(keys, /*identical_buffer_sizes=*/false);
      this->allgather(vals, /*identical_buffer_sizes=*/false);

      data.clear();

      for (std::size_t i=0; i<keys.size(); ++i)
        {
          // Attempt to emplace this value. If it works, then the value didn't
          // already exist and we can go on. If it fails, compute the std::max
          // between the current and existing values.
          auto pr = data.emplace(keys[i], vals[i]);

          bool emplaced = pr.second;

          if (!emplaced)
            {
              auto it = pr.first;
              it->second = std::max(it->second, vals[i]);
            }
        }
    }
}



template <typename K, typename V, typename C, typename A>
inline
void Communicator::max(std::map<K,V,C,A> & data) const
{
  this->map_max(data);
}



template <typename K, typename V, typename H, typename E, typename A>
inline
void Communicator::max(std::unordered_map<K,V,H,E,A> & data) const
{
  this->map_max(data);
}



template <typename T>
inline void Communicator::maxloc(T & r,
                                 unsigned int & max_id) const
{
  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE("maxloc(scalar)", "Parallel");

      DataPlusInt<T> data_in;
      ignore(data_in); // unused ifndef TIMPI_HAVE_MPI
      data_in.val = r;
      data_in.rank = this->rank();

      timpi_call_mpi
        (TIMPI_ALLREDUCE (MPI_IN_PLACE, &data_in, 1,
                          dataplusint_type_acquire<T>().first,
                          OpFunction<T>::max_location(), this->get()));
      r = data_in.val;
      max_id = data_in.rank;
    }
  else
    max_id = this->rank();
}


template <typename T, typename A1, typename A2>
inline void Communicator::maxloc(std::vector<T,A1> & r,
                                 std::vector<unsigned int,A2> & max_id) const
{
  if (this->size() > 1 && !r.empty())
    {
      TIMPI_LOG_SCOPE("maxloc(vector)", "Parallel");

      timpi_assert(this->verify(r.size()));

      std::vector<DataPlusInt<T>> data_in(r.size());
      for (std::size_t i=0; i != r.size(); ++i)
        {
          data_in[i].val  = r[i];
          data_in[i].rank = this->rank();
        }
      std::vector<DataPlusInt<T>> data_out(r.size());

      timpi_call_mpi
        (TIMPI_ALLREDUCE(data_in.data(), data_out.data(),
                         cast_int<CountType>(r.size()),
                         dataplusint_type_acquire<T>().first,
                         OpFunction<T>::max_location(),
                         this->get()));
      for (std::size_t i=0; i != r.size(); ++i)
        {
          r[i]      = data_out[i].val;
          max_id[i] = data_out[i].rank;
        }
    }
  else if (!r.empty())
    {
      for (std::size_t i=0; i != r.size(); ++i)
        max_id[i] = this->rank();
    }
}


template <typename A1, typename A2>
inline void Communicator::maxloc(std::vector<bool,A1> & r,
                                 std::vector<unsigned int,A2> & max_id) const
{
  if (this->size() > 1 && !r.empty())
    {
      TIMPI_LOG_SCOPE("maxloc(vector<bool>)", "Parallel");

      timpi_assert(this->verify(r.size()));

      std::vector<DataPlusInt<int>> data_in(r.size());
      for (std::size_t i=0; i != r.size(); ++i)
        {
          data_in[i].val  = r[i];
          data_in[i].rank = this->rank();
        }
      std::vector<DataPlusInt<int>> data_out(r.size());
      timpi_call_mpi
        (TIMPI_ALLREDUCE(data_in.data(), data_out.data(),
                         cast_int<CountType>(r.size()),
                         StandardType<int>(),
                         OpFunction<int>::max_location(),
                         this->get()));
      for (std::size_t i=0; i != r.size(); ++i)
        {
          r[i]      = data_out[i].val;
          max_id[i] = data_out[i].rank;
        }
    }
  else if (!r.empty())
    {
      for (std::size_t i=0; i != r.size(); ++i)
        max_id[i] = this->rank();
    }
}


template <typename T>
inline void Communicator::sum(const T & r,
                              T & o,
                              Request & req) const
{
#ifdef TIMPI_HAVE_MPI
  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE("sum()", "Parallel");

      timpi_call_mpi
        (TIMPI_IALLREDUCE(&r, &o, 1, StandardType<T>(&r),
                          OpFunction<T>::sum(), this->get(),
                          req.get()));
    }
  else
#endif
    {
      o = r;
      req = Request::null_request;
    }
}


template <typename T>
inline void Communicator::sum(T & timpi_mpi_var(r)) const
{
  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE("sum()", "Parallel");

      timpi_call_mpi
        (TIMPI_ALLREDUCE(MPI_IN_PLACE, &r, 1,
                         StandardType<T>(&r),
                         OpFunction<T>::sum(),
                         this->get()));
    }
}


template <typename T, typename A>
inline void Communicator::sum(std::vector<T,A> & r) const
{
  if (this->size() > 1 && !r.empty())
    {
      TIMPI_LOG_SCOPE("sum()", "Parallel");

      timpi_assert(this->verify(r.size()));

      timpi_call_mpi
        (TIMPI_ALLREDUCE(MPI_IN_PLACE, r.data(),
                         cast_int<CountType>(r.size()),
                         StandardType<T>(r.data()),
                         OpFunction<T>::sum(),
                         this->get()));
    }
}


// We still do function overloading for complex sums - in a perfect
// world we'd have a StandardSumOp to go along with StandardType...
template <typename T>
inline void Communicator::sum(std::complex<T> & timpi_mpi_var(r)) const
{
  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE("sum()", "Parallel");

      timpi_call_mpi
        (TIMPI_ALLREDUCE(MPI_IN_PLACE, &r, 2,
                         StandardType<T>(),
                         OpFunction<T>::sum(),
                         this->get()));
    }
}


template <typename T, typename A>
inline void Communicator::sum(std::vector<std::complex<T>,A> & r) const
{
  if (this->size() > 1 && !r.empty())
    {
      TIMPI_LOG_SCOPE("sum()", "Parallel");

      timpi_assert(this->verify(r.size()));

      timpi_call_mpi
        (TIMPI_ALLREDUCE(MPI_IN_PLACE, r.data(),
                         cast_int<CountType>(r.size() * 2),
                         StandardType<T>(nullptr),
                         OpFunction<T>::sum(), this->get()));
    }
}



// Helper function for summing std::map and std::unordered_map with
// fixed type (key, value) pairs.
template <typename Map,
          typename std::enable_if<std::is_base_of<DataType, StandardType<typename Map::key_type>>::value &&
                                  std::is_base_of<DataType, StandardType<typename Map::mapped_type>>::value,
                                  int>::type>
inline void Communicator::map_sum(Map & data) const
{
  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE("sum(map)", "Parallel");

      // There may be different keys on different processors, so we
      // first gather all the (key, value) pairs and then insert
      // them, summing repeated keys, back into the map.
      //
      // Note: We don't simply use Map::value_type here because the
      // key type is const in that case and we don't have the proper
      // StandardType overloads for communicating const types.
      std::vector<std::pair<typename Map::key_type, typename Map::mapped_type>>
        vecdata(data.begin(), data.end());

      this->allgather(vecdata, /*identical_buffer_sizes=*/false);

      data.clear();
      for (const auto & pr : vecdata)
        data[pr.first] += pr.second;
    }
}



// Helper function for summing std::map and std::unordered_map with
// non-fixed-type (key, value) pairs.
template <typename Map,
          typename std::enable_if<!(std::is_base_of<DataType, StandardType<typename Map::key_type>>::value &&
                                    std::is_base_of<DataType, StandardType<typename Map::mapped_type>>::value),
                                  int>::type>
inline void Communicator::map_sum(Map & data) const
{
  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE("sum(map)", "Parallel");

      // There may be different keys on different processors, so we
      // first gather all the (key, value) pairs and then insert
      // them, summing repeated keys, back into the map.
      std::vector<typename Map::key_type> keys;
      std::vector<typename Map::mapped_type> vals;

      auto data_size = data.size();
      keys.reserve(data_size);
      vals.reserve(data_size);

      for (const auto & pr : data)
        {
          keys.push_back(pr.first);
          vals.push_back(pr.second);
        }

      this->allgather(keys, /*identical_buffer_sizes=*/false);
      this->allgather(vals, /*identical_buffer_sizes=*/false);

      data.clear();

      for (std::size_t i=0; i<keys.size(); ++i)
        data[keys[i]] += vals[i];
    }
}



template <typename K, typename V, typename C, typename A>
inline void Communicator::sum(std::map<K,V,C,A> & data) const
{
  return this->map_sum(data);
}



template <typename K, typename V, typename H, typename E, typename A>
inline void Communicator::sum(std::unordered_map<K,V,H,E,A> & data) const
{
  return this->map_sum(data);
}



template <typename T, typename A1, typename A2,
          typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type>
inline void Communicator::allgather(const std::vector<T,A1> & sendval,
                                    std::vector<std::vector<T,A1>,A2> & recv,
                                    const bool identical_buffer_sizes) const
{
  TIMPI_LOG_SCOPE ("allgather()","Parallel");

  timpi_assert(this->size());

  // serial case
  if (this->size() < 2)
    {
      recv.resize(1);
      recv[0] = sendval;
      return;
    }

  recv.clear();
  recv.resize(this->size());

  std::vector<CountType>
    sendlengths  (this->size(), 0);
  std::vector<DispType>
    displacements(this->size(), 0);

  const CountType mysize = cast_int<CountType>(sendval.size());

  if (identical_buffer_sizes)
    sendlengths.assign(this->size(), mysize);
  else
    // first comm step to determine buffer sizes from all processors
    this->allgather(mysize, sendlengths);

  // Find the total size of the final array and
  // set up the displacement offsets for each processor
  CountType globalsize = 0;
  for (unsigned int i=0; i != this->size(); ++i)
    {
      displacements[i] = globalsize;
      globalsize += sendlengths[i];
    }

  // Check for quick return
  if (globalsize == 0)
    return;

  // monolithic receive buffer
  std::vector<T,A1> r(globalsize, 0);

  // and get the data from the remote processors.
  timpi_call_mpi
    (TIMPI_ALLGATHERV(const_cast<T*>(mysize ? sendval.data() : nullptr),
                      mysize, StandardType<T>(),
                      &r[0], sendlengths.data(), displacements.data(),
                      StandardType<T>(), this->get()));

  // slice receive buffer up
  for (unsigned int i=0; i != this->size(); ++i)
    recv[i].assign(r.begin()+displacements[i],
                   r.begin()+displacements[i]+sendlengths[i]);
}



template <typename T, typename A1, typename A2,
          typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type>
inline void Communicator::allgather(const std::vector<T,A1> & sendval,
                                    std::vector<std::vector<T,A1>,A2> & recv,
                                    const bool /* identical_buffer_sizes */) const
{
  TIMPI_LOG_SCOPE ("allgather()","Parallel");

  typedef typename Packing<T>::buffer_type buffer_t;

  std::vector<buffer_t> buffer;
  auto next_iter = pack_range ((void *)nullptr, sendval.begin(),
                               sendval.end(), buffer,
                               std::numeric_limits<CountType>::max());

  if (next_iter != sendval.end())
    timpi_error_msg("Non-blocking packed range sends cannot exceed " << std::numeric_limits<CountType>::max() << "in size");

  std::vector<std::vector<buffer_t>> allbuffers;

  timpi_assert(this->size());
  recv.clear();
  recv.resize(this->size());

  // Even if our vector sizes were identical, the variable-sized
  // data's buffer sizes might not be.
  this->allgather(buffer, allbuffers, false);

  for (processor_id_type i=0; i != this->size(); ++i)
    unpack_range(allbuffers[i], (void *)nullptr,
                 std::back_inserter(recv[i]), (T*)nullptr);
}



template <typename T, typename C, typename A>
inline void Communicator::set_union(std::set<T,C,A> & data,
                                    const unsigned int root_id) const
{
  if (this->size() > 1)
    {
      std::vector<T> vecdata(data.begin(), data.end());
      this->gather(root_id, vecdata);
      if (this->rank() == root_id)
        data.insert(vecdata.begin(), vecdata.end());
    }
}



template <typename T, typename C, typename A>
inline void Communicator::set_union(std::set<T,C,A> & data) const
{
  if (this->size() > 1)
    {
      std::vector<T> vecdata(data.begin(), data.end());
      this->allgather(vecdata, false);
      data.insert(vecdata.begin(), vecdata.end());
    }
}



template <typename T, typename C, typename A>
inline void Communicator::set_union(std::multiset<T,C,A> & data,
                                    const unsigned int root_id) const
{
  if (this->size() > 1)
    {
      std::vector<T> vecdata(data.begin(), data.end());
      this->gather(root_id, vecdata);
      if (this->rank() == root_id)
        {
          // Clear first so the root's data doesn't get duplicated
          data.clear();
          data.insert(vecdata.begin(), vecdata.end());
        }
    }
}


template <typename T, typename C, typename A>
inline void Communicator::set_union(std::multiset<T,C,A> & data) const
{
  if (this->size() > 1)
    {
      std::vector<T> vecdata(data.begin(), data.end());
      this->allgather(vecdata, false);

      // Don't let our data duplicate itself
      data.clear();

      data.insert(vecdata.begin(), vecdata.end());
    }
}



template <typename T1, typename T2, typename C, typename A>
inline void Communicator::set_union(std::map<T1,T2,C,A> & data,
                                    const unsigned int root_id) const
{
  if (this->size() > 1)
    {
      std::vector<std::pair<T1,T2>> vecdata(data.begin(), data.end());
      this->gather(root_id, vecdata);

      if (this->rank() == root_id)
        {
          // If we have a non-zero root_id, we still want to let pid
          // 0's values take precedence in the event we have duplicate
          // keys
          data.clear();

          data.insert(vecdata.begin(), vecdata.end());
        }
    }
}



template <typename T1, typename T2, typename C, typename A>
inline void Communicator::set_union(std::map<T1,T2,C,A> & data) const
{
  if (this->size() > 1)
    {
      std::vector<std::pair<T1,T2>> vecdata(data.begin(), data.end());
      this->allgather(vecdata, false);

      // We want values on lower pids to take precedence in the event
      // we have duplicate keys
      data.clear();

      data.insert(vecdata.begin(), vecdata.end());
    }
}



template <typename T1, typename T2, typename C, typename A>
inline void Communicator::set_union(std::multimap<T1,T2,C,A> & data,
                                    const unsigned int root_id) const
{
  if (this->size() > 1)
    {
      std::vector<std::pair<T1,T2>> vecdata(data.begin(), data.end());
      this->gather(root_id, vecdata);

      if (this->rank() == root_id)
        {
          // Don't let root's data duplicate itself
          data.clear();

          data.insert(vecdata.begin(), vecdata.end());
        }
    }
}



template <typename T1, typename T2, typename C, typename A>
inline void Communicator::set_union(std::multimap<T1,T2,C,A> & data) const
{
  if (this->size() > 1)
    {
      std::vector<std::pair<T1,T2>> vecdata(data.begin(), data.end());
      this->allgather(vecdata, false);

      // Don't let our data duplicate itself
      data.clear();

      data.insert(vecdata.begin(), vecdata.end());
    }
}



template <typename K, typename H, typename KE, typename A>
inline void Communicator::set_union(std::unordered_set<K,H,KE,A> & data,
                                    const unsigned int root_id) const
{
  if (this->size() > 1)
    {
      std::vector<K> vecdata(data.begin(), data.end());
      this->gather(root_id, vecdata);
      if (this->rank() == root_id)
        data.insert(vecdata.begin(), vecdata.end());
    }
}



template <typename K, typename H, typename KE, typename A>
inline void Communicator::set_union(std::unordered_set<K,H,KE,A> & data) const
{
  if (this->size() > 1)
    {
      std::vector<K> vecdata(data.begin(), data.end());
      this->allgather(vecdata, false);
      data.insert(vecdata.begin(), vecdata.end());
    }
}



template <typename K, typename H, typename KE, typename A>
inline void Communicator::set_union(std::unordered_multiset<K,H,KE,A> & data,
                                    const unsigned int root_id) const
{
  if (this->size() > 1)
    {
      std::vector<K> vecdata(data.begin(), data.end());
      this->gather(root_id, vecdata);
      if (this->rank() == root_id)
        {
          // Don't let root's data duplicate itself
          data.clear();

          data.insert(vecdata.begin(), vecdata.end());
        }
    }
}



template <typename K, typename H, typename KE, typename A>
inline void Communicator::set_union(std::unordered_multiset<K,H,KE,A> & data) const
{
  if (this->size() > 1)
    {
      std::vector<K> vecdata(data.begin(), data.end());
      this->allgather(vecdata, false);

      // Don't let our data duplicate itself
      data.clear();

      data.insert(vecdata.begin(), vecdata.end());
    }
}



template <typename K, typename T, typename H, typename KE, typename A>
inline void Communicator::set_union(std::unordered_map<K,T,H,KE,A> & data,
                                    const unsigned int root_id) const
{
  if (this->size() > 1)
    {
      std::vector<std::pair<K,T>> vecdata(data.begin(), data.end());
      this->gather(root_id, vecdata);

      if (this->rank() == root_id)
        {
          // If we have a non-zero root_id, we still want to let pid
          // 0's values take precedence in the event we have duplicate
          // keys
          data.clear();

          data.insert(vecdata.begin(), vecdata.end());
        }
    }
}



template <typename K, typename T, typename H, typename KE, typename A>
inline void Communicator::set_union(std::unordered_map<K,T,H,KE,A> & data) const
{
  if (this->size() > 1)
    {
      std::vector<std::pair<K,T>> vecdata(data.begin(), data.end());
      this->allgather(vecdata, false);

      // We want values on lower pids to take precedence in the event
      // we have duplicate keys
      data.clear();

      data.insert(vecdata.begin(), vecdata.end());
    }
}



template <typename K, typename T, typename H, typename KE, typename A>
inline void Communicator::set_union(std::unordered_multimap<K,T,H,KE,A> & data,
                                    const unsigned int root_id) const
{
  if (this->size() > 1)
    {
      std::vector<std::pair<K,T>> vecdata(data.begin(), data.end());
      this->gather(root_id, vecdata);

      if (this->rank() == root_id)
        {
          // Don't let root's data duplicate itself
          data.clear();

          data.insert(vecdata.begin(), vecdata.end());
        }
    }
}



template <typename K, typename T, typename H, typename KE, typename A>
inline void Communicator::set_union(std::unordered_multimap<K,T,H,KE,A> & data) const
{
  if (this->size() > 1)
    {
      std::vector<std::pair<K,T>> vecdata(data.begin(), data.end());
      this->allgather(vecdata, false);

      // Don't let our data duplicate itself
      data.clear();

      data.insert(vecdata.begin(), vecdata.end());
    }
}



template <typename T, typename A>
inline void Communicator::gather(const unsigned int root_id,
                                 const T & sendval,
                                 std::vector<T,A> & recv) const
{
  timpi_assert_less (root_id, this->size());

  if (this->rank() == root_id)
    recv.resize(this->size());

  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE("gather()", "Parallel");

      StandardType<T> send_type(&sendval);

      timpi_assert_less(root_id, this->size());

      timpi_call_mpi
        (TIMPI_GATHER(const_cast<T*>(&sendval), 1, send_type,
                      recv.empty() ? nullptr : recv.data(), 1, send_type,
                      root_id, this->get()));
    }
  else
    recv[0] = sendval;
}



template <typename T, typename A,
          typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type>
inline void Communicator::gather(const unsigned int root_id,
                                 std::vector<T,A> & r) const
{
  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      return;
    }

  timpi_assert_less (root_id, this->size());

  std::vector<CountType>
    sendlengths  (this->size(), 0);
  std::vector<DispType>
    displacements(this->size(), 0);

  const CountType mysize = cast_int<CountType>(r.size());
  this->allgather(mysize, sendlengths);

  TIMPI_LOG_SCOPE("gather()", "Parallel");

  // Find the total size of the final array and
  // set up the displacement offsets for each processor.
  CountType globalsize = 0;
  for (unsigned int i=0; i != this->size(); ++i)
    {
      displacements[i] = globalsize;
      globalsize += sendlengths[i];
    }

  // Check for quick return
  if (globalsize == 0)
    return;

  // copy the input buffer
  std::vector<T,A> r_src(r);

  // now resize it to hold the global data
  // on the receiving processor
  if (root_id == this->rank())
    r.resize(globalsize);

  timpi_assert_less(root_id, this->size());

  // and get the data from the remote processors
  timpi_call_mpi
    (TIMPI_GATHERV(r_src.empty() ? nullptr : r_src.data(), mysize,
                   StandardType<T>(), r.empty() ? nullptr : r.data(),
                   sendlengths.data(), displacements.data(),
                   StandardType<T>(), root_id, this->get()));
}


template <typename T, typename A,
          typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type>
inline void Communicator::gather(const unsigned int root_id,
                                 std::vector<T,A> & r) const
{
  std::vector<T,A> gathered;
  this->gather_packed_range(root_id, (void *)(nullptr),
                            r.begin(), r.end(),
                            std::inserter(gathered, gathered.end()));

  gathered.swap(r);
}



template <typename T, typename A>
inline void Communicator::gather(const unsigned int root_id,
                                 const std::basic_string<T> & sendval,
                                 std::vector<std::basic_string<T>,A> & recv,
                                 const bool identical_buffer_sizes) const
{
  timpi_assert_less (root_id, this->size());

  if (this->rank() == root_id)
    recv.resize(this->size());

  if (this->size() > 1)
    {
      TIMPI_LOG_SCOPE ("gather()","Parallel");

      std::vector<CountType>
        sendlengths  (this->size(), 0);
      std::vector<DispType>
        displacements(this->size(), 0);

      const CountType mysize = cast_int<CountType>(sendval.size());

      if (identical_buffer_sizes)
        sendlengths.assign(this->size(), mysize);
      else
        // first comm step to determine buffer sizes from all processors
        this->gather(root_id, mysize, sendlengths);

      // Find the total size of the final array and
      // set up the displacement offsets for each processor
      CountType globalsize = 0;
      for (unsigned int i=0; i < this->size(); ++i)
        {
          displacements[i] = globalsize;
          globalsize += sendlengths[i];
        }

      // monolithic receive buffer
      std::basic_string<T> r;
      if (this->rank() == root_id)
        r.resize(globalsize, 0);

      timpi_assert_less(root_id, this->size());

      // and get the data from the remote processors.
      timpi_call_mpi
        (TIMPI_GATHERV(const_cast<T*>(sendval.data()),
                       mysize, StandardType<T>(),
                       this->rank() == root_id ? &r[0] : nullptr,
                       sendlengths.data(), displacements.data(),
                       StandardType<T>(), root_id, this->get()));

      // slice receive buffer up
      if (this->rank() == root_id)
        for (unsigned int i=0; i != this->size(); ++i)
          recv[i] = r.substr(displacements[i], sendlengths[i]);
    }
  else
    recv[0] = sendval;
}



template <typename T, typename A,
          typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type>
inline void Communicator::allgather(const T & sendval,
                                    std::vector<T,A> & recv) const
{
  TIMPI_LOG_SCOPE ("allgather()","Parallel");

  timpi_assert(this->size());
  recv.resize(this->size());

  const unsigned int comm_size = this->size();
  if (comm_size > 1)
    {
      StandardType<T> send_type(&sendval);

      timpi_call_mpi
        (TIMPI_ALLGATHER(const_cast<T*>(&sendval), 1, send_type, recv.data(), 1,
                         send_type, this->get()));
    }
  else if (comm_size > 0)
    recv[0] = sendval;
}

template <typename T, typename A,
          typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type>
inline void Communicator::allgather(const T & sendval,
                                    std::vector<T,A> & recv) const
{
  TIMPI_LOG_SCOPE ("allgather()","Parallel");

  timpi_assert(this->size());
  recv.resize(this->size());

  static const std::size_t approx_total_buffer_size = 1e8;
  const std::size_t approx_each_buffer_size =
    approx_total_buffer_size / this->size();

  unsigned int comm_size = this->size();
  if (comm_size > 1)
    {
      std::vector<T> range = {sendval};

      allgather_packed_range((void *)(nullptr), range.begin(), range.end(), recv.begin(),
                             approx_each_buffer_size);
    }
  else if (comm_size > 0)
    recv[0] = sendval;
}

template <typename T, typename A,
          typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type>
inline void Communicator::allgather(std::vector<T,A> & r,
                                    const bool identical_buffer_sizes) const
{
  if (this->size() < 2)
    return;

  TIMPI_LOG_SCOPE("allgather()", "Parallel");

  if (identical_buffer_sizes)
    {
      timpi_assert(this->verify(r.size()));
      if (r.empty())
        return;

      std::vector<T,A> r_src(r.size()*this->size());
      r_src.swap(r);
      StandardType<T> send_type(r_src.data());

      timpi_call_mpi
        (TIMPI_ALLGATHER(r_src.data(), cast_int<CountType>(r_src.size()),
                         send_type, r.data(), cast_int<CountType>(r_src.size()),
                         send_type, this->get()));
      // timpi_assert(this->verify(r));
      return;
    }

  std::vector<CountType>
    sendlengths  (this->size(), 0);
  std::vector<DispType>
    displacements(this->size(), 0);

  const CountType mysize = cast_int<CountType>(r.size());
  this->allgather(mysize, sendlengths);

  // Find the total size of the final array and
  // set up the displacement offsets for each processor.
  CountType globalsize = 0;
  for (unsigned int i=0; i != this->size(); ++i)
    {
      displacements[i] = globalsize;
      globalsize += sendlengths[i];
    }

  // Check for quick return
  if (globalsize == 0)
    return;

  // copy the input buffer
  std::vector<T,A> r_src(globalsize);
  r_src.swap(r);

  StandardType<T> send_type(r.data());

  // and get the data from the remote processors.
  // Pass nullptr if our vector is empty.
  timpi_call_mpi
    (TIMPI_ALLGATHERV(r_src.empty() ? nullptr : r_src.data(), mysize,
                      send_type, r.data(), sendlengths.data(),
                      displacements.data(), send_type, this->get()));
}

template <typename T, typename A,
          typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type>
inline void Communicator::allgather(std::vector<T,A> & r,
                                    const bool identical_buffer_sizes) const
{
  if (this->size() < 2)
    return;

  TIMPI_LOG_SCOPE("allgather()", "Parallel");

  if (identical_buffer_sizes)
    {
      timpi_assert(this->verify(r.size()));
      if (r.empty())
        return;


      std::vector<T,A> r_src(r.size()*this->size());
      r_src.swap(r);

      this->allgather_packed_range((void *)(nullptr),
                                   r_src.begin(),
                                   r_src.end(),
                                   r.begin());
      return;
    }

  std::vector<CountType>
    sendlengths  (this->size(), 0);
  std::vector<DispType>
    displacements(this->size(), 0);

  const CountType mysize = cast_int<CountType>(r.size());
  this->allgather(mysize, sendlengths);

  // Find the total size of the final array
  CountType globalsize = 0;
  for (unsigned int i=0; i != this->size(); ++i)
      globalsize += sendlengths[i];

  // Check for quick return
  if (globalsize == 0)
    return;

  // copy the input buffer
  std::vector<T,A> r_src(globalsize);
  r_src.swap(r);

  this->allgather_packed_range((void *)(nullptr),
                               r_src.begin(),
                               r_src.end(),
                               r.begin());
}

template <typename T, typename A>
inline void Communicator::allgather(std::vector<std::basic_string<T>,A> & r,
                                    const bool identical_buffer_sizes) const
{
  if (this->size() < 2)
    return;

  TIMPI_LOG_SCOPE("allgather()", "Parallel");

  if (identical_buffer_sizes)
    {
      timpi_assert(this->verify(r.size()));

      // identical_buffer_sizes doesn't buy us much since we have to
      // communicate the lengths of strings within each buffer anyway
      if (r.empty())
        return;
    }

  // Concatenate the input buffer into a send buffer, and keep track
  // of input string lengths
  std::vector<CountType> mystrlengths (r.size());
  std::vector<T> concat_src;

  CountType myconcatsize = 0;
  for (std::size_t i=0; i != r.size(); ++i)
    {
      CountType stringlen = cast_int<CountType>(r[i].size());
      mystrlengths[i] = stringlen;
      myconcatsize += stringlen;
    }
  concat_src.reserve(myconcatsize);
  for (std::size_t i=0; i != r.size(); ++i)
    concat_src.insert
      (concat_src.end(), r[i].begin(), r[i].end());

  // Get the string lengths from all other processors
  std::vector<CountType> strlengths = mystrlengths;
  this->allgather(strlengths, identical_buffer_sizes);

  // We now know how many strings we'll be receiving
  r.resize(strlengths.size());

  // Get the concatenated data sizes from all other processors
  std::vector<CountType> concat_sizes;
  this->allgather(myconcatsize, concat_sizes);

  // Find the total size of the final concatenated array and
  // set up the displacement offsets for each processor.
  std::vector<DispType> displacements(this->size(), 0);
  CountType globalsize = 0;
  for (unsigned int i=0; i != this->size(); ++i)
    {
      displacements[i] = globalsize;
      globalsize += concat_sizes[i];
    }

  // Check for quick return
  if (globalsize == 0)
    return;

  // Get the concatenated data from the remote processors.
  // Pass nullptr if our vector is empty.
  std::vector<T> concat(globalsize);

  // We may have concat_src.empty(), but we know concat has at least
  // one element we can use as an example for StandardType
  StandardType<T> send_type(concat.data());

  timpi_call_mpi
    (TIMPI_ALLGATHERV(concat_src.empty() ?
                      nullptr : concat_src.data(), myconcatsize,
                      send_type, concat.data(), concat_sizes.data(),
                      displacements.data(), send_type, this->get()));

  // Finally, split concatenated data into strings
  const T * begin = concat.data();
  for (std::size_t i=0; i != r.size(); ++i)
    {
      const T * end = begin + strlengths[i];
      r[i].assign(begin, end);
      begin = end;
    }
}



template <typename T, typename A>
void Communicator::scatter(const std::vector<T,A> & data,
                           T & recv,
                           const unsigned int root_id) const
{
  ignore(root_id); // Only needed for MPI and/or dbg/devel
  timpi_assert_less (root_id, this->size());

  // Do not allow the root_id to scatter a nullptr vector.
  // That would leave recv in an indeterminate state.
  timpi_assert (this->rank() != root_id || this->size() == data.size());

  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      recv = data[0];
      return;
    }

  TIMPI_LOG_SCOPE("scatter()", "Parallel");

  T * data_ptr = const_cast<T*>(data.empty() ? nullptr : data.data());
  ignore(data_ptr); // unused ifndef TIMPI_HAVE_MPI

  timpi_assert_less(root_id, this->size());

  timpi_call_mpi
    (TIMPI_SCATTER(data_ptr, 1, StandardType<T>(data_ptr),
                   &recv, 1, StandardType<T>(&recv), root_id, this->get()));
}



template <typename T, typename A>
void Communicator::scatter(const std::vector<T,A> & data,
                           std::vector<T,A> & recv,
                           const unsigned int root_id) const
{
  timpi_assert_less (root_id, this->size());

  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      recv.assign(data.begin(), data.end());
      return;
    }

  TIMPI_LOG_SCOPE("scatter()", "Parallel");

  std::size_t recv_buffer_size = 0;
  if (this->rank() == root_id)
    {
      timpi_assert(data.size() % this->size() == 0);
      recv_buffer_size = cast_int<std::size_t>(data.size() / this->size());
    }

  this->broadcast(recv_buffer_size);
  recv.resize(recv_buffer_size);

  T * data_ptr = const_cast<T*>(data.empty() ? nullptr : data.data());
  T * recv_ptr = recv.empty() ? nullptr : recv.data();
  ignore(data_ptr, recv_ptr); // unused ifndef TIMPI_HAVE_MPI

  timpi_assert_less(root_id, this->size());

  timpi_call_mpi
    (TIMPI_SCATTER(data_ptr, recv_buffer_size, StandardType<T>(data_ptr),
                   recv_ptr, recv_buffer_size, StandardType<T>(recv_ptr),
                   root_id, this->get()));
}



template <typename T, typename A1, typename A2>
void Communicator::scatter(const std::vector<T,A1> & data,
                           const std::vector<CountType,A2> counts,
                           std::vector<T,A1> & recv,
                           const unsigned int root_id) const
{
  timpi_assert_less (root_id, this->size());

  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      timpi_assert (counts.size() == this->size());
      recv.assign(data.begin(), data.begin() + counts[0]);
      return;
    }

  std::vector<DispType> displacements(this->size(), 0);
  if (root_id == this->rank())
    {
      timpi_assert(counts.size() == this->size());

      // Create a displacements vector from the incoming counts vector
      std::size_t globalsize = 0;
      for (unsigned int i=0; i < this->size(); ++i)
        {
          displacements[i] = globalsize;
          globalsize += counts[i];
        }

      timpi_assert(data.size() == globalsize);
    }

  TIMPI_LOG_SCOPE("scatter()", "Parallel");

  // Scatter the buffer sizes to size remote buffers
  CountType recv_buffer_size = 0;
  this->scatter(counts, recv_buffer_size, root_id);
  recv.resize(recv_buffer_size);

  T * data_ptr = const_cast<T*>(data.empty() ? nullptr : data.data());
  CountType * count_ptr = const_cast<CountType*>(counts.empty() ? nullptr : counts.data());
  T * recv_ptr = recv.empty() ? nullptr : recv.data();
  ignore(data_ptr, count_ptr, recv_ptr); // unused ifndef TIMPI_HAVE_MPI

  timpi_assert_less(root_id, this->size());

  // Scatter the non-uniform chunks
  timpi_call_mpi
    (TIMPI_SCATTERV(data_ptr, count_ptr, displacements.data(), StandardType<T>(data_ptr),
                    recv_ptr, recv_buffer_size, StandardType<T>(recv_ptr), root_id, this->get()));
}


#ifdef TIMPI_HAVE_MPI
#if MPI_VERSION > 3
  /**
   * vector<int> based scatter, for backwards compatibility
   */
template <typename T, typename A1, typename A2>
void Communicator::scatter(const std::vector<T,A1> & data,
                           const std::vector<int,A2> counts,
                           std::vector<T,A1> & recv,
                           const unsigned int root_id) const
{
  std::vector<CountType> full_counts(counts.begin(), counts.end());
  this->scatter(data, full_counts, recv, root_id);
}
#endif
#endif



template <typename T, typename A1, typename A2>
void Communicator::scatter(const std::vector<std::vector<T,A1>,A2> & data,
                           std::vector<T,A1> & recv,
                           const unsigned int root_id,
                           const bool identical_buffer_sizes) const
{
  timpi_assert_less (root_id, this->size());

  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      timpi_assert (data.size() == this->size());
      recv.assign(data[0].begin(), data[0].end());
      return;
    }

  std::vector<T,A1> stacked_data;
  std::vector<CountType> counts;

  if (root_id == this->rank())
    {
      timpi_assert (data.size() == this->size());

      if (!identical_buffer_sizes)
        counts.resize(this->size());

      for (std::size_t i=0; i < data.size(); ++i)
        {
          if (!identical_buffer_sizes)
            counts[i] = cast_int<CountType>(data[i].size());
#ifndef NDEBUG
          else
            // Check that buffer sizes are indeed equal
            timpi_assert(!i || data[i-1].size() == data[i].size());
#endif
          std::copy(data[i].begin(), data[i].end(), std::back_inserter(stacked_data));
        }
    }

  if (identical_buffer_sizes)
    this->scatter(stacked_data, recv, root_id);
  else
    this->scatter(stacked_data, counts, recv, root_id);
}



template <typename T, typename A>
inline void Communicator::alltoall(std::vector<T,A> & buf) const
{
  if (this->size() < 2 || buf.empty())
    return;

  TIMPI_LOG_SCOPE("alltoall()", "Parallel");

  // the per-processor size.  this is the same for all
  // processors using MPI_Alltoall, could be variable
  // using MPI_Alltoallv
  const CountType size_per_proc =
    cast_int<CountType>(buf.size()/this->size());
  ignore(size_per_proc);

  timpi_assert_equal_to (buf.size()%this->size(), 0);

  timpi_assert(this->verify(size_per_proc));

  StandardType<T> send_type(buf.data());

  timpi_call_mpi
    (TIMPI_ALLTOALL(MPI_IN_PLACE, size_per_proc, send_type, buf.data(),
                    size_per_proc, send_type, this->get()));
}



template <typename T
#ifdef TIMPI_HAVE_MPI
          ,
          typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type
#endif
          >
inline void Communicator::broadcast (T & timpi_mpi_var(data),
                                     const unsigned int root_id,
                                     const bool /* identical_sizes */) const
{
  ignore(root_id); // Only needed for MPI and/or dbg/devel
  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      return;
    }

  timpi_assert_less (root_id, this->size());

  TIMPI_LOG_SCOPE("broadcast()", "Parallel");

  // Spread data to remote processors.
  timpi_call_mpi
    (TIMPI_BCAST(&data, 1, StandardType<T>(&data), root_id,
                 this->get()));
}

#ifdef TIMPI_HAVE_MPI
template <typename T,
          typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type>
inline void Communicator::broadcast (T & data,
                                     const unsigned int root_id,
                                     const bool /* identical_sizes */) const
{
  ignore(root_id); // Only needed for MPI and/or dbg/devel
  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      return;
    }

  timpi_assert_less (root_id, this->size());

//   // If we don't have MPI, then we should be done, and calling the below can
//   // have the side effect of instantiating Packing<T> classes that are not
//   // defined. (Normally we would be calling a more specialized overload of
//   // broacast that would then call broadcast_packed_range with appropriate
//   // template arguments)
// #ifdef TIMPI_HAVE_MPI
  std::vector<T> range = {data};

  this->broadcast_packed_range((void *)(nullptr),
                               range.begin(),
                               range.end(),
                               (void *)(nullptr),
                               range.begin(),
                               root_id);

  data = range[0];
// #endif
}
#endif

template <typename T, typename A,
          typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type>
inline void Communicator::broadcast (std::vector<T,A> & timpi_mpi_var(data),
                                     const unsigned int root_id,
                                     const bool timpi_mpi_var(identical_sizes)) const
{
  ignore(root_id); // Only needed for MPI and/or dbg/devel
  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      return;
    }

#ifdef TIMPI_HAVE_MPI

  timpi_assert_less (root_id, this->size());
  timpi_assert (this->verify(identical_sizes));

  TIMPI_LOG_SCOPE("broadcast()", "Parallel");

  std::size_t data_size = data.size();

  if (identical_sizes)
    timpi_assert(this->verify(data_size));
  else
    this->broadcast(data_size, root_id);

  data.resize(data_size);

  // and get the data from the remote processors.
  // Pass nullptr if our vector is empty.
  T * data_ptr = data.empty() ? nullptr : data.data();

  timpi_assert_less(root_id, this->size());

  timpi_call_mpi
    (TIMPI_BCAST(data_ptr, cast_int<CountType>(data.size()),
                 StandardType<T>(data_ptr), root_id, this->get()));
#endif
}

template <typename T, typename A,
          typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type>
inline void Communicator::broadcast (std::vector<T,A> & data,
                                     const unsigned int root_id,
                                     const bool identical_sizes) const
{
  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      return;
    }

  timpi_assert_less (root_id, this->size());
  timpi_assert (this->verify(identical_sizes));

  TIMPI_LOG_SCOPE("broadcast()", "Parallel");

  std::size_t data_size = data.size();

  if (identical_sizes)
    timpi_assert(this->verify(data_size));
  else
    this->broadcast(data_size, root_id);

  data.resize(data_size);

  timpi_assert_less(root_id, this->size());

  this->broadcast_packed_range((void *)(nullptr),
                               data.begin(),
                               data.end(),
                               (void *)(nullptr),
                               data.begin(),
                               root_id);
}

template <typename Map,
          typename std::enable_if<std::is_base_of<DataType, StandardType<typename Map::key_type>>::value &&
                                  std::is_base_of<DataType, StandardType<typename Map::mapped_type>>::value,
                                  int>::type>
inline void Communicator::map_broadcast(Map & timpi_mpi_var(data),
                                        const unsigned int root_id,
                                        const bool timpi_mpi_var(identical_sizes)) const
{
  ignore(root_id); // Only needed for MPI and/or dbg/devel
  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      return;
    }

#ifdef TIMPI_HAVE_MPI
  timpi_assert_less (root_id, this->size());
  timpi_assert (this->verify(identical_sizes));

  TIMPI_LOG_SCOPE("broadcast(map)", "Parallel");

  std::size_t data_size=data.size();
  if (identical_sizes)
    timpi_assert(this->verify(data_size));
  else
    this->broadcast(data_size, root_id);

  std::vector<std::pair<typename Map::key_type,
                        typename Map::mapped_type>> comm_data;

  if (root_id == this->rank())
    comm_data.assign(data.begin(), data.end());
  else
    comm_data.resize(data_size);

  this->broadcast(comm_data, root_id, true);

  if (this->rank() != root_id)
    {
      data.clear();
      data.insert(comm_data.begin(), comm_data.end());
    }
#endif
}

template <typename Map,
          typename std::enable_if<!(std::is_base_of<DataType, StandardType<typename Map::key_type>>::value &&
                                    std::is_base_of<DataType, StandardType<typename Map::mapped_type>>::value),
                                  int>::type>
inline void Communicator::map_broadcast(Map & timpi_mpi_var(data),
                                        const unsigned int root_id,
                                        const bool timpi_mpi_var(identical_sizes)) const
{
  ignore(root_id); // Only needed for MPI and/or dbg/devel
  if (this->size() == 1)
    {
      timpi_assert (!this->rank());
      timpi_assert (!root_id);
      return;
    }

#ifdef TIMPI_HAVE_MPI
  timpi_assert_less (root_id, this->size());
  timpi_assert (this->verify(identical_sizes));

  TIMPI_LOG_SCOPE("broadcast()", "Parallel");

  std::size_t data_size=data.size();
  if (identical_sizes)
    timpi_assert(this->verify(data_size));
  else
    this->broadcast(data_size, root_id);

  std::vector<typename Map::key_type> pair_first; pair_first.reserve(data_size);
  std::vector<typename Map::mapped_type> pair_second; pair_first.reserve(data_size);

  if (root_id == this->rank())
    {
      for (const auto & pr : data)
        {
          pair_first.push_back(pr.first);
          pair_second.push_back(pr.second);
        }
    }
  else
    {
      pair_first.resize(data_size);
      pair_second.resize(data_size);
    }

  this->broadcast
    (pair_first, root_id,
     StandardType<typename Map::key_type>::is_fixed_type);
  this->broadcast
    (pair_second, root_id,
     StandardType<typename Map::mapped_type>::is_fixed_type);

  timpi_assert(pair_first.size() == pair_first.size());

  if (this->rank() != root_id)
    {
      data.clear();
      for (std::size_t i=0; i<pair_first.size(); ++i)
        data[pair_first[i]] = pair_second[i];
    }
#endif
}

template <typename T1, typename T2, typename C, typename A>
inline void Communicator::broadcast(std::map<T1,T2,C,A> & data,
                                    const unsigned int root_id,
                                    const bool identical_sizes) const
{
  this->map_broadcast(data, root_id, identical_sizes);
}



template <typename K, typename V, typename H, typename E, typename A>
inline void Communicator::broadcast(std::unordered_map<K,V,H,E,A> & data,
                                    const unsigned int root_id,
                                    const bool identical_sizes) const
{
  this->map_broadcast(data, root_id, identical_sizes);
}

template <typename Context, typename OutputContext,
          typename Iter, typename OutputIter>
inline void Communicator::broadcast_packed_range(const Context * context1,
                                                 Iter range_begin,
                                                 const Iter range_end,
                                                 OutputContext * context2,
                                                 OutputIter out_iter,
                                                 const unsigned int root_id,
                                                 std::size_t approx_buffer_size) const
{
  typedef typename std::iterator_traits<Iter>::value_type T;
  typedef typename Packing<T>::buffer_type buffer_t;

  if (this->size() == 1)
  {
    timpi_assert (!this->rank());
    timpi_assert (!root_id);
    return;
  }

  do
  {
    // We will serialize variable size objects from *range_begin to
    // *range_end as a sequence of ints in this buffer
    std::vector<buffer_t> buffer;

    if (this->rank() == root_id)
      range_begin = pack_range
        (context1, range_begin, range_end, buffer, approx_buffer_size);

    // this->broadcast(vector) requires the receiving vectors to
    // already be the appropriate size
    std::size_t buffer_size = buffer.size();
    this->broadcast (buffer_size, root_id);

    // We continue until there's nothing left to broadcast
    if (!buffer_size)
      break;

    buffer.resize(buffer_size);

    // Broadcast the packed data
    this->broadcast (buffer, root_id);

    // OutputIter might not have operator= implemented; for maximum
    // compatibility we'll rely on its copy constructor.
    std::unique_ptr<OutputIter> next_out_iter =
      std::make_unique<OutputIter>(out_iter);

    if (this->rank() != root_id)
      {
        auto return_out_iter = unpack_range
          (buffer, context2, *next_out_iter, (T*)nullptr);
        next_out_iter = std::make_unique<OutputIter>(return_out_iter);
      }
  } while (true);  // break above when we reach buffer_size==0
}


template <typename Context, typename Iter, typename OutputIter>
inline void Communicator::gather_packed_range(const unsigned int root_id,
                                              Context * context,
                                              Iter range_begin,
                                              const Iter range_end,
                                              OutputIter out_iter,
                                              std::size_t approx_buffer_size) const
{
  typedef typename std::iterator_traits<Iter>::value_type T;
  typedef typename Packing<T>::buffer_type buffer_t;

  bool nonempty_range = (range_begin != range_end);
  this->max(nonempty_range);

  // OutputIter might not have operator= implemented; for maximum
  // compatibility we'll rely on its copy constructor.
  std::unique_ptr<OutputIter> next_out_iter =
    std::make_unique<OutputIter>(out_iter);

  while (nonempty_range)
    {
      // We will serialize variable size objects from *range_begin to
      // *range_end as a sequence of ints in this buffer
      std::vector<buffer_t> buffer;

      range_begin = pack_range
        (context, range_begin, range_end, buffer, approx_buffer_size);

      this->gather(root_id, buffer);

      auto return_out_iter = unpack_range
        (buffer, context, *next_out_iter, (T*)(nullptr));
      next_out_iter = std::make_unique<OutputIter>(return_out_iter);

      nonempty_range = (range_begin != range_end);
      this->max(nonempty_range);
    }
}


template <typename Context, typename Iter, typename OutputIter>
inline void Communicator::allgather_packed_range(Context * context,
                                                 Iter range_begin,
                                                 const Iter range_end,
                                                 OutputIter out_iter,
                                                 std::size_t approx_buffer_size) const
{
  typedef typename std::iterator_traits<Iter>::value_type T;
  typedef typename Packing<T>::buffer_type buffer_t;

  bool nonempty_range = (range_begin != range_end);
  this->max(nonempty_range);

  // OutputIter might not have operator= implemented; for maximum
  // compatibility we'll rely on its copy constructor.
  std::unique_ptr<OutputIter> next_out_iter =
    std::make_unique<OutputIter>(out_iter);

  while (nonempty_range)
    {
      // We will serialize variable size objects from *range_begin to
      // *range_end as a sequence of ints in this buffer
      std::vector<buffer_t> buffer;

      range_begin = pack_range
        (context, range_begin, range_end, buffer, approx_buffer_size);

      this->allgather(buffer, false);

      timpi_assert(buffer.size());

      auto return_out_iter = unpack_range
        (buffer, context, *next_out_iter, (T*)nullptr);
      next_out_iter = std::make_unique<OutputIter>(return_out_iter);

      nonempty_range = (range_begin != range_end);
      this->max(nonempty_range);
    }
}



template<typename T>
inline Status Communicator::packed_range_probe (const unsigned int src_processor_id,
                                                const MessageTag & tag,
                                                bool & flag) const
{
  TIMPI_LOG_SCOPE("packed_range_probe()", "Parallel");

  ignore(src_processor_id, tag); // unused in opt mode w/o MPI

  Status stat((StandardType<typename Packing<T>::buffer_type>()));

  int int_flag = 0;

  timpi_assert(src_processor_id < this->size() ||
               src_processor_id == any_source);

  timpi_call_mpi(MPI_Iprobe(int(src_processor_id),
                            tag.value(),
                            this->get(),
                            &int_flag,
                            stat.get()));

  flag = int_flag;

  return stat;
}



template <typename T, typename A,
          typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type>
inline bool Communicator::possibly_receive (unsigned int & src_processor_id,
                                            std::vector<T,A> & buf,
                                            Request & req,
                                            const MessageTag & tag) const
{
  T * dataptr = buf.empty() ? nullptr : buf.data();

  return this->possibly_receive(src_processor_id, buf, StandardType<T>(dataptr), req, tag);
}

template <typename T, typename A,
          typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type>
inline bool Communicator::possibly_receive (unsigned int & src_processor_id,
                                            std::vector<T,A> & buf,
                                            Request & req,
                                            const MessageTag & tag) const
{
  return this->possibly_receive_packed_range(src_processor_id,
                                             (void *)(nullptr),
                                             buf.begin(),
                                             (T *)(nullptr),
                                             req,
                                             tag);
}



template <typename T, typename A1, typename A2>
inline bool Communicator::possibly_receive (unsigned int & src_processor_id,
                                            std::vector<std::vector<T,A1>,A2> & buf,
                                            Request & req,
                                            const MessageTag & tag) const
{
  T * dataptr = buf.empty() ? nullptr : (buf[0].empty() ? nullptr : buf[0].data());

  return this->possibly_receive(src_processor_id, buf, StandardType<T>(dataptr), req, tag);
}


template <typename Context, typename OutputIter, typename T>
inline bool Communicator::possibly_receive_packed_range (unsigned int & src_processor_id,
                                                         Context * context,
                                                         OutputIter out,
                                                         const T * type,
                                                         Request & req,
                                                         const MessageTag & tag) const
{
  TIMPI_LOG_SCOPE("possibly_receive_packed_range()", "Parallel");

  bool int_flag = 0;

  auto stat = packed_range_probe<T>(src_processor_id, tag, int_flag);

  if (int_flag)
  {
    src_processor_id = stat.source();

    nonblocking_receive_packed_range(src_processor_id,
                                     context,
                                     out,
                                     type,
                                     req,
                                     stat,
                                     tag);

     // The MessageTag should stay registered for the Request lifetime
     req.add_post_wait_work
       (new PostWaitDereferenceTag(tag));
  }

  timpi_assert(!int_flag || (int_flag &&
                             src_processor_id < this->size() &&
                             src_processor_id != any_source));

  return int_flag;
}


} // namespace TIMPI

#endif // TIMPI_PARALLEL_IMPLEMENTATION_H
