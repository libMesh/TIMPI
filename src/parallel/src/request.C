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

// Local includes
#include "timpi/request.h"

// TIMPI includes
#include "timpi/timpi_call_mpi.h"
#include "timpi/timpi_assert.h"
#include "timpi/post_wait_work.h"
#include "timpi/status.h"

// Disable libMesh logging until we decide how to port it best
// #include "libmesh/libmesh_logging.h"
#define TIMPI_LOG_SCOPE(f,c)

// C++ includes
#include <memory>
#include <vector>
#include <utility>


namespace TIMPI
{

#ifdef TIMPI_HAVE_MPI
const request Request::null_request = MPI_REQUEST_NULL;
#else
const request Request::null_request = 0;
#endif

// ------------------------------------------------------------
// Request member functions
Request::Request () :
  _request(null_request),
  post_wait_work(nullptr)
{}

Request::Request (const request & r) :
  _request(r),
  post_wait_work(nullptr)
{}

Request::Request (const Request & other) :
  _request(other._request),
  post_wait_work(other.post_wait_work)
{
  if (other._prior_request.get())
    _prior_request = std::unique_ptr<Request>
      (new Request(*other._prior_request.get()));

  // operator= should behave like a shared pointer
  if (post_wait_work)
    post_wait_work->second++;
}

void Request::cleanup()
{
  if (post_wait_work)
    {
      // Decrement the use count
      post_wait_work->second--;

      if (!post_wait_work->second)
        {
#ifdef DEBUG
          // If we're done using this request, then we'd better have
          // done the work we waited for
          for (const auto & item : post_wait_work->first)
            timpi_assert(!item);
#endif
          delete post_wait_work;
          post_wait_work = nullptr;
        }
    }
}

Request & Request::operator = (const Request & other)
{
  this->cleanup();
  _request = other._request;
  post_wait_work = other.post_wait_work;

  if (other._prior_request.get())
    _prior_request = std::unique_ptr<Request>
      (new Request(*other._prior_request.get()));

  // operator= should behave like a shared pointer
  if (post_wait_work)
    post_wait_work->second++;

  return *this;
}

Request & Request::operator = (const request & r)
{
  this->cleanup();
  _request = r;
  post_wait_work = nullptr;
  return *this;
}

Request::~Request () {
  this->cleanup();
}

Status Request::wait ()
{
  TIMPI_LOG_SCOPE("wait()", "Request");

  if (_prior_request.get())
    {
      _prior_request->wait();
      _prior_request.reset(nullptr);
    }

  Status stat {};
#ifdef TIMPI_HAVE_MPI
  timpi_call_mpi
    (MPI_Wait (&_request, stat.get()));
#endif
  if (post_wait_work)
    {
      for (auto & item : post_wait_work->first)
        {
          // The user should never try to give us non-existent work or try
          // to wait() twice.
          timpi_assert (item);
          item->run();
          delete item;
          item = nullptr;
        }
      post_wait_work->first.clear();
    }

  return stat;
}

bool Request::test ()
{
#ifdef TIMPI_HAVE_MPI
  int val=0;

  timpi_call_mpi
    (MPI_Test (&_request, &val, MPI_STATUS_IGNORE));

  if (val)
    {
      timpi_assert          (_request == null_request);
      timpi_assert_equal_to (val, 1);
    }

  return val;
#else
  return true;
#endif
}

#ifdef TIMPI_HAVE_MPI
bool Request::test (status & stat)
{
  int val=0;

  timpi_call_mpi
    (MPI_Test (&_request, &val, &stat));

  return val;
}
#else
bool Request::test (status &)
{
  return true;
}
#endif

void Request::add_prior_request(const Request & req)
{
  // We're making a chain of prior requests, not a tree
  timpi_assert(!req._prior_request.get());

  Request * new_prior_req = new Request(req);

  // new_prior_req takes ownership of our existing _prior_request
  new_prior_req->_prior_request.reset(this->_prior_request.release());

  // Our _prior_request now manages the new resource we just set up
  this->_prior_request.reset(new_prior_req);
}

void Request::add_post_wait_work(PostWaitWork * work)
{
  if (!post_wait_work)
    post_wait_work = new
      std::pair<std::vector <PostWaitWork * >, unsigned int>
      (std::vector <PostWaitWork * >(), 1);
  post_wait_work->first.push_back(work);
}

void wait (std::vector<Request> & r)
{
  for (auto & req : r)
    req.wait();
}

std::size_t waitany (std::vector<Request> & r)
{
  timpi_assert(!r.empty());

  int r_size = cast_int<int>(r.size());
  std::vector<request> raw(r_size);
  int non_null = r_size;
  for (int i=0; i != r_size; ++i)
    {
      Request * root = &r[i];
      // If we have prior requests, we need to complete the first one
      // first
      while (root->_prior_request.get())
        root = root->_prior_request.get();
      raw[i] = *root->get();

      if (raw[i] != Request::null_request)
        non_null = std::min(non_null,i);
    }

  if (non_null == r_size)
    return std::size_t(-1);

  int index = non_null;

#ifdef TIMPI_HAVE_MPI
  bool only_priors_completed = false;
  Request * next;

  do
    {
      timpi_call_mpi
        (MPI_Waitany(r_size, raw.data(), &index, MPI_STATUS_IGNORE));

      timpi_assert_not_equal_to(index, MPI_UNDEFINED);

      timpi_assert_less(index, r_size);

      Request * completed = &r[index];
      next = completed;

      // If we completed a prior request, we're not really done yet,
      // so find the next in that line to try again.
      while (completed->_prior_request.get())
        {
          only_priors_completed = true;
          next = completed;
          completed = completed->_prior_request.get();
        }

      // MPI sets a completed MPI_Request to MPI_REQUEST_NULL; we want
      // to preserve that
      completed->_request = raw[index];

      // Do any post-wait work for the completed request
      if (completed->post_wait_work)
        for (auto & item : completed->post_wait_work->first)
          {
            // The user should never try to give us non-existent work or try
            // to wait() twice.
            timpi_assert (item);
            item->run();
            delete item;
            item = nullptr;
          }

      next->_prior_request.reset(nullptr);
      raw[index] = *next->get();

    } while(only_priors_completed);
#else
  r[index]._request = Request::null_request;
#endif

  return index;
}

} // namespace TIMPI
