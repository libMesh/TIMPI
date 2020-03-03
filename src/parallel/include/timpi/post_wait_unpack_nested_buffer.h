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


#ifndef TIMPI_POST_WAIT_UNPACK_NESTED_BUFFER_H
#define TIMPI_POST_WAIT_UNPACK_NESTED_BUFFER_H

// TIMPI includes
#include "timpi/communicator.h"
#include "timpi/post_wait_work.h"
#include "timpi/standard_type.h"

namespace TIMPI
{

// PostWaitWork specialization for MPI_Unpack of nested buffers.
// Container will most likely be vector<vector<T>>
template <typename Container>
struct PostWaitUnpackNestedBuffer : public PostWaitWork {
  PostWaitUnpackNestedBuffer(const std::vector<char> & buffer,
                             Container & out,
                             const DataType & T_type,
                             const Communicator & comm_in) :
    recvbuf(buffer), recv(out), comm(comm_in) {
      timpi_call_mpi(MPI_Type_dup(T_type, &(type.operator data_type &())));
    }

  ~PostWaitUnpackNestedBuffer() {
#ifdef TIMPI_HAVE_MPI
    // Not bothering with return type; we can't throw in a destructor
    MPI_Type_free(&(type.operator data_type &()));
#endif
  }

  virtual void run() override {
  // We should at least have one header datum, for outer vector size
  timpi_assert (!recvbuf.empty());

  // Unpack the received buffer
  int bufsize = cast_int<int>(recvbuf.size());
  int recvsize, pos=0;
  timpi_call_mpi
    (MPI_Unpack (recvbuf.data(), bufsize, &pos,
                 &recvsize, 1, StandardType<unsigned int>(),
                 comm.get()));

  // ... size the outer buffer
  recv.resize (recvsize);

  const std::size_t n_vecs = recvsize;
  for (std::size_t i = 0; i != n_vecs; ++i)
    {
      int subvec_size;

      timpi_call_mpi
        (MPI_Unpack (recvbuf.data(), bufsize, &pos,
                     &subvec_size, 1,
                     StandardType<unsigned int>(),
                     comm.get()));

      // ... size the inner buffer
      recv[i].resize (subvec_size);

      // ... unpack the inner buffer if it is not empty
      if (!recv[i].empty())
        timpi_call_mpi
          (MPI_Unpack (recvbuf.data(), bufsize, &pos, recv[i].data(),
                       subvec_size, type, comm.get()));
    }
  }

private:
  const std::vector<char> & recvbuf;
  Container & recv;
  DataType type;
  const Communicator & comm;
};

} // namespace TIMPI

#endif // TIMPI_POST_WAIT_UNPACK_NESTED_BUFFER_H
