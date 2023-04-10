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



#ifndef TIMPI_PARALLEL_SYNC_H
#define TIMPI_PARALLEL_SYNC_H

// Local Includes
#include "timpi/parallel_implementation.h"

// C++ includes
#include <algorithm>   // max
#include <iterator>    // inserter
#include <list>
#include <map>         // map, multimap, pair
#include <type_traits> // remove_reference, remove_const
#include <utility>     // move
#include <vector>


namespace TIMPI {

//------------------------------------------------------------------------
/**
 * Send and receive and act on vectors of data.
 *
 * The \p data map is indexed by processor ids as keys, and for each
 * processor id in the map there should be a vector of data to send.
 * For processors to which no data should be sent, there should be no
 * map entry; this will avoid any unnecessary communication.
 *
 * Data which is received from other processors will be operated on by
 * act_on_data(processor_id_type pid, std::vector<datum> && data)
 *
 * If data exists for the local processor in the map, it will be acted
 * on directly, without any network operations.
 *
 * No guarantee about operation ordering is made - this function will
 * attempt to act on data in the order in which it is received.
 *
 * All receives and actions are completed before this function
 * returns.
 *
 * If you wish to use move semantics within the data received in \p
 * act_on_data, pass data itself as an rvalue reference.
 *
 * This overload should be automatically selected for data which has a
 * \p StandardType specialization defined, so that we can directly
 * send it without serializing it into buffers beforehand.
 */
 ///@{
template <typename MapToVectors,
          typename ActionFunctor,
          typename std::enable_if< std::is_base_of<DataType, StandardType<
            typename InnermostType<typename std::remove_const<
              typename std::remove_reference<MapToVectors>::type::mapped_type::value_type
            >::type>::type>>::value, int>::type = 0>
void push_parallel_vector_data(const Communicator & comm,
                               MapToVectors && data,
                               const ActionFunctor & act_on_data);
 ///@}

//------------------------------------------------------------------------
/**
 * Send and receive and act on vectors of data.
 *
 * This overload should be automatically selected for data which has a
 * \p Packing specialization defined, where we must serialize it into
 * buffers before sending.
 */
 ///@{
template <typename MapToVectors,
          typename ActionFunctor,
          typename std::enable_if<Has_buffer_type<Packing<
            typename InnermostType<typename std::remove_const<
              typename std::remove_reference<MapToVectors>::type::mapped_type::value_type
            >::type>::type>>::value, int>::type = 0>
void push_parallel_vector_data(const Communicator & comm,
                               MapToVectors && data,
                               const ActionFunctor & act_on_data);
 ///@}

/**
 * Send query vectors, receive and answer them with vectors of data,
 * then act on those answers.
 *
 * The \p data map is indexed by processor ids as keys, and for each
 * processor id in the map there should be a vector of query ids to send.
 * For processors to which no data should be sent, there should be no
 * map entry; this will avoid any unnecessary communication.
 *
 * Queries will be operated on by the queried processor by
 * gather_data(processor_id_type pid, const std::vector<id> & ids,
 *             std::vector<datum> & data)
 *
 * Answer data from each query will be operated on by
 * act_on_data(processor_id_type pid, const std::vector<id> & ids,
 *             std::vector<datum> && data);
 *
 * If a query vector exists for the local processor in the map,
 * gather_data will be called on it directly, and act_on_data will be
 * called on the response directly, without any network operations.
 *
 * The example pointer may be null; it merely needs to be of the
 * correct type.  It's just here because function overloading in C++
 * is easy, whereas SFINAE is hard and partial template specialization
 * of functions is impossible.
 *
 * No guarantee about operation ordering is made - this function will
 * attempt to act on data in the order in which it is received.
 *
 * All receives and actions are completed before this function
 * returns.
 */
template <typename datum,
          typename MapToVectors,
          typename GatherFunctor,
          typename ActionFunctor>
void pull_parallel_vector_data(const Communicator & comm,
                               const MapToVectors & queries,
                               GatherFunctor & gather_data,
                               const ActionFunctor & act_on_data,
                               const datum * example);

/**
 * Send and receive and act on vectors of data. Similar to
 * push_parallel_vector_data, except the vectors are packed and unpacked
 * using the Parallel::Packing routines.
 *
 * The \p data map is indexed by processor ids as keys, and for each
 * processor id in the map there should be a vector of data to send.
 * For processors to which no data should be sent, there should be no
 * map entry; this will avoid any unnecessary communication.
 *
 * Data which is received from other processors will be operated on by
 * act_on_data(processor_id_type pid, std::vector<datum> && data)
 *
 * If data exists for the local processor in the map, it will be acted
 * on directly, without any network operations.  This *also* avoids
 * packing and unpacking the data, so no side effects of those
 * operations should be assumed.
 *
 * No guarantee about operation ordering is made - this function will
 * attempt to act on data in the order in which it is received.
 *
 * All receives and actions are completed before this function
 * returns.
 *
 * If you wish to use move semantics within the data received in \p
 * act_on_data, pass data itself as an rvalue reference.
 *
 */
///@{
template <typename MapToVectors,
          typename ActionFunctor,
          typename Context>
void push_parallel_packed_range(const Communicator & comm,
                                MapToVectors && data,
                                Context * context,
                                const ActionFunctor & act_on_data);
///@}

//------------------------------------------------------------------------
// Parallel function overloads
//

/*
 * A specialization for types that are harder to non-blocking receive.
 */
template <typename datum,
          typename A,
          typename MapToVectors,
          typename GatherFunctor,
          typename ActionFunctor>
void pull_parallel_vector_data(const Communicator & comm,
                               const MapToVectors & queries,
                               GatherFunctor & gather_data,
                               ActionFunctor & act_on_data,
                               const std::vector<datum,A> * example);






//------------------------------------------------------------------------
// Parallel members
//

// Separate namespace for not-for-public-use helper functions
namespace detail {

template <typename MapToContainers,
          typename SendFunctor,
          typename PossiblyReceiveFunctor,
          typename ActionFunctor>
void
push_parallel_nbx_helper(const Communicator & comm,
                         MapToContainers && data,
                         const SendFunctor & send_functor,
                         const PossiblyReceiveFunctor & possibly_receive_functor,
                         const ActionFunctor & act_on_data)
{
  typedef typename std::remove_reference<MapToContainers>::type::value_type::second_type
    container_type;

  // This function must be run on all processors at once
  timpi_parallel_only(comm);

  // This function implements the "NBX" algorithm from
  // https://htor.inf.ethz.ch/publications/img/hoefler-dsde-protocols.pdf

  // We'll grab a tag so we can overlap request sends and receives
  // without confusing one for the other
  auto tag = comm.get_unique_tag();

  // Save off the old send_mode so we can restore it after this
  auto old_send_mode = comm.send_mode();

  // Set the sending to synchronous - this is so that we can know when
  // the sends are complete
  const_cast<Communicator &>(comm).send_mode(Communicator::SYNCHRONOUS);

  // The send requests
  std::list<Request> requests;

  const processor_id_type num_procs = comm.size();

  for (auto & datapair : data)
    {
      // In the case of data partitioned into more processors than we
      // have ranks, we "wrap around"
      processor_id_type dest_pid = datapair.first % num_procs;
      auto & datum = datapair.second;

      // Just act on data if the user requested a send-to-self
      if (dest_pid == comm.rank())
        act_on_data(dest_pid, std::move(datum));
      else
        {
          requests.emplace_back();
          send_functor(dest_pid, datum, requests.back(), tag);
        }
    }

  // In serial we've now acted on all our data.
  if (num_procs == 1)
    return;

  // Whether or not all of the sends are complete
  bool sends_complete = requests.empty();

  // Whether or not the nonblocking barrier has started
  bool started_barrier = false;
  // Request for the nonblocking barrier
  Request barrier_request;

  struct IncomingInfo
  {
    unsigned int src_pid = any_source;
    Request request;
    container_type data;
  };

  // Storage for the incoming requests and data
  // The last entry in this list will _always_ be an invalid entry
  // that is available for use for processing the next incoming
  // request. That is, its size will always be >= 1
  std::list<IncomingInfo> incoming;
  incoming.emplace_back(); // add the first invalid entry for receives

  // Keep looking for receives
  while (true)
    {
      timpi_assert(incoming.size() > 0);

      // Check if there is a message and start receiving it
      auto & current_incoming = incoming.back();
      timpi_assert_equal_to(current_incoming.src_pid, any_source);
      if (possibly_receive_functor(current_incoming.src_pid,
                                   current_incoming.data,
                                   current_incoming.request, tag))
        {
          timpi_assert(current_incoming.src_pid != any_source);

          // Insert another entry so that the next poll has something
          // to fill into if needed
          incoming.emplace_back();
        }

        // Work through the incoming requests and act on them if they're ready
        incoming.remove_if
          ([&act_on_data
#ifndef NDEBUG
            ,&incoming
#endif
           ](IncomingInfo & info)
           {
             // The last entry (marked by an invalid src pid) should be skipped;
             // it needs to remain in the list for potential filling in the next poll
             const bool is_invalid_entry = info.src_pid == any_source;
             timpi_assert_equal_to(is_invalid_entry, &info == &incoming.back());

             if (is_invalid_entry)
               return false;

             // If it's finished - let's act on it
             if (info.request.test())
               {
                 // Do any post-wait work
                 info.request.wait();

                 // Act on the data
                 act_on_data(info.src_pid, std::move(info.data));

                 // This removes it from the list
                 return true;
               }

               // Not finished yet
               return false;
             });

      requests.remove_if
        ([](Request & req)
         {
           if (req.test())
             {
               // Do Post-Wait work
               req.wait();
               return true;
             }

             // Not finished yet
             return false;
         });


      // See if all of the sends are finished
      if (requests.empty())
        sends_complete = true;

      // If they've all completed then we can start the barrier
      if (sends_complete && !started_barrier)
        {
          started_barrier = true;
          comm.nonblocking_barrier(barrier_request);
        }

      // Must fully receive everything before being allowed to move on!
      // We reserve a single value in incoming for filling within the
      // next poll loop, so if incoming is a size of one we have nothing
      // to process
      if (incoming.size() == 1)
        // See if all proessors have finished all sends (i.e. _done_!)
        if (started_barrier)
          if (barrier_request.test())
            break; // Done!
    }

  // Reset the send mode
  const_cast<Communicator &>(comm).send_mode(old_send_mode);
}

template <typename MapToContainers,
          typename SendFunctor,
          typename ReceiveFunctor,
          typename ActionFunctor>
void
push_parallel_alltoall_helper(const Communicator & comm,
                              MapToContainers && data,
                              const SendFunctor & send_functor,
                              const ReceiveFunctor & receive_functor,
                              const ActionFunctor & act_on_data)
{
  typedef typename std::remove_reference<MapToContainers>::type::value_type::second_type
    container_type;

  // This function must be run on all processors at once
  timpi_parallel_only(comm);

  // This function implements a simpler asynchronous protocol than
  // NBX.  Every processor will know exactly how many receives to
  // post.

  processor_id_type num_procs = comm.size();

  // Number of vectors to send to each procesor
  std::vector<std::size_t> will_send_to(num_procs, 0);
  for (auto & datapair : data)
    {
      // In the case of data partitioned into more processors than we
      // have ranks, we "wrap around"
      processor_id_type destid = datapair.first % num_procs;

      // Don't give us empty vectors to send
      timpi_assert_greater(datapair.second.size(), 0);

      will_send_to[destid]++;
    }

  // Tell everyone about where everyone will send to
  comm.alltoall(will_send_to);

  // will_send_to now represents how many vectors we'll receive from
  // each processor; give it a better name.
  auto & will_receive_from = will_send_to;

  processor_id_type n_receives = 0;
  for (processor_id_type proc_id = 0; proc_id < num_procs; proc_id++)
    n_receives += will_receive_from[proc_id];

  // We'll grab a tag so we can overlap request sends and receives
  // without confusing one for the other
  MessageTag tag = comm.get_unique_tag();

  // The send requests
  std::list<Request> requests;

  // Post all of the sends, non-blocking
  for (auto & datapair : data)
    {
      processor_id_type destid = datapair.first % num_procs;
      auto & datum = datapair.second;

      // Just act on data if the user requested a send-to-self
      if (destid == comm.rank())
        {
          act_on_data(destid, std::move(datum));
          n_receives--;
        }
      else
        {
          requests.emplace_back();
          send_functor(destid, datum, requests.back(), tag);
        }
    }

  // In serial we've now acted on all our data.
  if (num_procs == 1)
    return;

  // Post all of the receives.
  for (processor_id_type i = 0; i != n_receives; ++i)
    {
      Status stat(comm.probe(any_source, tag));
      const processor_id_type
        proc_id = cast_int<processor_id_type>(stat.source());

      container_type received_data;
      receive_functor(proc_id, received_data, tag);
      act_on_data(proc_id, std::move(received_data));
    }

  // Wait on all the sends to complete
  for (auto & req : requests)
    req.wait();
}

template <typename MapToContainers,
          typename SendReceiveFunctor,
          typename ActionFunctor>
void
push_parallel_roundrobin_helper(const Communicator & comm,
                                MapToContainers && data,
                                const SendReceiveFunctor & sendreceive_functor,
                                const ActionFunctor & act_on_data)
{
  typedef typename std::remove_reference<MapToContainers>::type::value_type::second_type
    container_type;

  // This function must be run on all processors at once
  timpi_parallel_only(comm);

  // This function implements the simplest protocol possible, fully
  // synchronous.  Every processor talks to every other.  Only use this for
  // debugging, and only when you're desperate.

  unsigned int num_procs = comm.size();

  // Do multiple exchanges if we have an oversized data map
  processor_id_type n_exchanges = 1;
  for (auto & datapair : data)
    {
      const unsigned int destid = datapair.first;
      n_exchanges = std::max(n_exchanges, destid/num_procs+1);

      // Don't give us empty vectors to send
      timpi_assert_greater(datapair.second.size(), 0);
    }

  comm.max(n_exchanges);

  // We'll grab a tag so responses and queries won't be confused when
  // this is used within a pull
  auto tag = comm.get_unique_tag();

  // Do the send_receives, blocking
  for (processor_id_type e=0; e != n_exchanges; ++e)
    for (processor_id_type p=0; p != num_procs; ++p)
      {
        const processor_id_type procup =
          cast_int<processor_id_type>((comm.rank() + p) %
                                      num_procs);
        const processor_id_type procdown =
          cast_int<processor_id_type>((comm.rank() + num_procs - p) %
                                      num_procs);

        container_type empty_container;
        auto data_it = data.find(procup + e*num_procs);
        auto * const data_to_send =
          (data_it == data.end()) ?
          &empty_container : &data_it->second;

        container_type received_data;
        sendreceive_functor(procup, *data_to_send,
                            procdown, received_data, tag);

        // Empty containers aren't *real* data, they're an artifact of
        // doing send_receive with everyone.  Just skip them.
        if (!received_data.empty())
          act_on_data(procdown, std::move(received_data));
      }
}


} // namespace detail



template <typename MapToContainers,
          typename ActionFunctor,
          typename Context>
void push_parallel_packed_range(const Communicator & comm,
                                MapToContainers && data,
                                Context * context,
                                const ActionFunctor & act_on_data)
{
  typedef typename std::remove_reference<MapToContainers>::type::mapped_type container_type;
  typedef typename container_type::value_type nonref_type;
  typename std::remove_const<nonref_type>::type * output_type = nullptr;

  switch (comm.sync_type()) {
  case Communicator::NBX:
    {
      auto send_functor = [&context, &comm](const processor_id_type dest_pid,
                                            const container_type & datum,
                                            Request & send_request,
                                            const MessageTag tag) {
        comm.nonblocking_send_packed_range(dest_pid, context, datum.begin(), datum.end(), send_request, tag);
      };

      auto possibly_receive_functor = [&context, &output_type, &comm](unsigned int & current_src_proc,
                                                                      container_type & current_incoming_data,
                                                                      Request & current_request,
                                                                      const MessageTag tag) {
        return comm.possibly_receive_packed_range(
            current_src_proc,
            context,
            std::inserter(current_incoming_data, current_incoming_data.end()),
            output_type,
            current_request,
            tag);
      };

      detail::push_parallel_nbx_helper
        (comm, data, send_functor, possibly_receive_functor, act_on_data);
    }
    break;
  case Communicator::ALLTOALL_COUNTS:
    {
      auto send_functor = [&context, &comm](const processor_id_type dest_pid,
                                            const container_type & datum,
                                            Request & send_request,
                                            const MessageTag tag) {
        comm.nonblocking_send_packed_range(dest_pid, context, datum.begin(), datum.end(), send_request, tag);
      };

      auto receive_functor = [&context, &output_type, &comm](unsigned int current_src_proc,
                                                             container_type & current_incoming_data,
                                                             const MessageTag tag) {
        bool flag = false;
        Status stat(comm.packed_range_probe<container_type>(current_src_proc, tag, flag));
        timpi_assert(flag);

        Request req;
        comm.nonblocking_receive_packed_range(current_src_proc, context,
          std::inserter(current_incoming_data, current_incoming_data.end()),
          output_type, req, stat, tag);
        req.wait();
      };

      detail::push_parallel_alltoall_helper
        (comm, data, send_functor, receive_functor, act_on_data);
    }
    break;
  case Communicator::SENDRECEIVE:
    {
      auto sendreceive_functor = [&context, &output_type, &comm]
        (const processor_id_type dest_pid,
         const container_type & data_to_send,
         const processor_id_type src_pid,
         container_type & received_data,
         const MessageTag tag) {
        comm.send_receive_packed_range(dest_pid, context,
                                       data_to_send.begin(),
                                       data_to_send.end(), src_pid,
                                       context,
                                       std::inserter(received_data,
                                                     received_data.end()),
                                       output_type, tag, tag);
      };

      detail::push_parallel_roundrobin_helper
        (comm, data, sendreceive_functor, act_on_data);
    }
    break;
  default:
    timpi_error_msg("Invalid sync_type setting " << comm.sync_type());
  }

}


template <typename MapToVectors,
          typename ActionFunctor,
          typename std::enable_if< std::is_base_of<DataType, StandardType<
            typename InnermostType<typename std::remove_const<
              typename std::remove_reference<MapToVectors>::type::mapped_type::value_type
            >::type>::type>>::value, int>::type>
void push_parallel_vector_data(const Communicator & comm,
                               MapToVectors && data,
                               const ActionFunctor & act_on_data)
{
  typedef typename std::remove_reference<MapToVectors>::type::mapped_type container_type;
  typedef typename container_type::value_type nonref_type;
  typedef typename std::remove_const<nonref_type>::type nonconst_nonref_type;

  // We'll construct the StandardType once rather than inside a loop.
  // We can't pass in example data here, because we might have
  // data.empty() on some ranks, so we'll need StandardType to be able
  // to construct the user's data type without an example.
  auto type = build_standard_type(static_cast<nonconst_nonref_type *>(nullptr));

  switch (comm.sync_type()) {
  case Communicator::NBX:
    {
      auto send_functor = [&type, &comm](const processor_id_type dest_pid,
                                         const container_type & datum,
                                         Request & send_request,
                                         const MessageTag tag) {
        comm.send(dest_pid, datum, type, send_request, tag);
      };

      auto possibly_receive_functor = [&type, &comm](unsigned int & current_src_proc,
                                                     container_type & current_incoming_data,
                                                     Request & current_request,
                                                     const MessageTag tag) {
        return comm.possibly_receive(
            current_src_proc, current_incoming_data, type, current_request, tag);
      };

      detail::push_parallel_nbx_helper
        (comm, data, send_functor, possibly_receive_functor, act_on_data);
    }
    break;
  case Communicator::ALLTOALL_COUNTS:
    {
#ifdef TIMPI_HAVE_MPI // We should never hit these functors in serial
      auto send_functor = [&type, &comm](const processor_id_type dest_pid,
                                         const container_type & datum,
                                         Request & send_request,
                                         const MessageTag tag) {
        comm.send(dest_pid, datum, type, send_request, tag);
      };

      auto receive_functor = [&type, &comm](unsigned int current_src_proc,
                                            container_type & current_incoming_data,
                                            const MessageTag tag) {
        comm.receive(current_src_proc, current_incoming_data, type, tag);
      };
#else
      auto send_functor = [](const processor_id_type,
                             const container_type &,
                             Request &,
                             const MessageTag) {
        timpi_error(); // We should never hit these in serial
      };

      auto receive_functor = [](unsigned int,
                                container_type &,
                                const MessageTag) {
        timpi_error();
      };
#endif

      detail::push_parallel_alltoall_helper
        (comm, data, send_functor, receive_functor, act_on_data);
    }
    break;
  case Communicator::SENDRECEIVE:
    {
      auto sendreceive_functor = [&comm](const processor_id_type dest_pid,
                                         const container_type & data_to_send,
                                         const processor_id_type src_pid,
                                         container_type & received_data,
                                         const MessageTag tag) {
        comm.send_receive(dest_pid, data_to_send,
                          src_pid, received_data, tag, tag);
      };

      detail::push_parallel_roundrobin_helper
        (comm, data, sendreceive_functor, act_on_data);
    }
    break;
  default:
    timpi_error_msg("Invalid sync_type setting " << comm.sync_type());
  }
}



template <typename MapToVectors,
          typename ActionFunctor,
          typename std::enable_if<Has_buffer_type<Packing<
            typename InnermostType<typename std::remove_const<
              typename std::remove_reference<MapToVectors>::type::mapped_type::value_type
            >::type>::type>>::value, int>::type>
void push_parallel_vector_data(const Communicator & comm,
                               MapToVectors && data,
                               const ActionFunctor & act_on_data)
{
  void * context = nullptr;
  push_parallel_packed_range(comm, data, context, act_on_data);
}


template <typename datum,
          typename MapToVectors,
          typename GatherFunctor,
          typename ActionFunctor>
void pull_parallel_vector_data(const Communicator & comm,
                               const MapToVectors & queries,
                               GatherFunctor & gather_data,
                               ActionFunctor & act_on_data,
                               const datum *)
{
  typedef typename MapToVectors::mapped_type query_type;

  std::multimap<processor_id_type, std::vector<datum> >
    response_data;

#ifndef NDEBUG
  processor_id_type max_pid = 0;
  for (auto p : queries)
    max_pid = std::max(max_pid, p.first);

  // Our SENDRECEIVE implementation doesn't preserve ordering, but we
  // need ordering preserved for the multimap trick here to work.
  if (comm.sync_type() == Communicator::SENDRECEIVE &&
      max_pid > comm.size())
    timpi_not_implemented();
#endif

  auto gather_functor =
    [&gather_data, &response_data]
    (processor_id_type pid, query_type query)
    {
      auto new_data_it =
        response_data.emplace(pid, std::vector<datum>());
      gather_data(pid, query, new_data_it->second);
      timpi_assert_equal_to(query.size(), new_data_it->second.size());
    };

  push_parallel_vector_data (comm, queries, gather_functor);

  std::map<processor_id_type, unsigned int> responses_acted_on;

  const processor_id_type num_procs = comm.size();

  auto action_functor =
    [&act_on_data, &queries, &responses_acted_on,
#ifndef NDEBUG
     max_pid,
#endif
     num_procs
    ]
    (processor_id_type pid, const std::vector<datum> & data)
    {
      // We rely on responses coming in the same order as queries
      const unsigned int nth_query = responses_acted_on[pid]++;

      auto q_pid_its = queries.equal_range(pid);
      auto query_it = q_pid_its.first;

      // In an oversized pull we might not have any queries addressed
      // to the *base* pid, but only to pid+N*num_procs for some N>1
      // timpi_assert(query_it != q_pid_its.second);
      while (query_it == q_pid_its.second)
        {
          pid += num_procs;
          q_pid_its = queries.equal_range(pid);
          timpi_assert_less_equal(pid, max_pid);
          query_it = q_pid_its.first;
        }

      for (unsigned int i=0; i != nth_query; ++i)
        {
          query_it++;
          if (query_it == q_pid_its.second)
            {
              do
                {
                  pid += num_procs;
                  q_pid_its = queries.equal_range(pid);
                  timpi_assert_less_equal(pid, max_pid);
                } while (q_pid_its.first == q_pid_its.second);
              query_it = q_pid_its.first;
            }
        }

      act_on_data(pid, query_it->second, data);
    };

  push_parallel_vector_data (comm, response_data, action_functor);
}




template <typename datum,
          typename A,
          typename MapToVectors,
          typename GatherFunctor,
          typename ActionFunctor>
void pull_parallel_vector_data(const Communicator & comm,
                               const MapToVectors & queries,
                               GatherFunctor & gather_data,
                               ActionFunctor & act_on_data,
                               const std::vector<datum,A> *)
{
  typedef typename MapToVectors::mapped_type query_type;

  // First index: order of creation, irrelevant
  std::vector<std::vector<std::vector<datum,A>>> response_data;
  std::vector<Request> response_requests;

  // We'll grab a tag so we can overlap request sends and receives
  // without confusing one for the other
  MessageTag tag = comm.get_unique_tag();

  auto gather_functor =
    [&comm, &gather_data, &act_on_data,
     &response_data, &response_requests, &tag]
    (processor_id_type pid, query_type query)
    {
      std::vector<std::vector<datum,A>> response;
      gather_data(pid, query, response);
      timpi_assert_equal_to(query.size(),
                              response.size());

      // Just act on data if the user requested a send-to-self
      if (pid == comm.rank())
        {
          act_on_data(pid, query, response);
        }
      else
        {
          Request sendreq;
          comm.send(pid, response, sendreq, tag);
          response_requests.push_back(sendreq);
          response_data.push_back(std::move(response));
        }
    };

  push_parallel_vector_data (comm, queries, gather_functor);

  // Every outgoing query should now have an incoming response.
  //
  // Post all of the receives.
  //
  // Use blocking API here since we can't use the pre-sized
  // non-blocking APIs with this data type.
  //
  // FIXME - implement Derek's API from #1684, switch to that!
  std::vector<Request> receive_requests;
  std::vector<processor_id_type> receive_procids;
  for (std::size_t i = 0,
       n_queries = queries.size() - queries.count(comm.rank());
       i != n_queries; ++i)
    {
      Status stat(comm.probe(any_source, tag));
      const processor_id_type
        proc_id = cast_int<processor_id_type>(stat.source());

      std::vector<std::vector<datum,A>> received_data;
      comm.receive(proc_id, received_data, tag);

      timpi_assert(queries.count(proc_id));
      auto & querydata = queries.at(proc_id);
      timpi_assert_equal_to(querydata.size(), received_data.size());
      act_on_data(proc_id, querydata, received_data);
    }

  wait(response_requests);
}

} // namespace TIMPI

#endif // TIMPI_PARALLEL_SYNC_H
