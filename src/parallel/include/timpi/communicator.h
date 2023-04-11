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


#ifndef TIMPI_COMMUNICATOR_H
#define TIMPI_COMMUNICATOR_H

// TIMPI includes
#include "timpi/standard_type.h"
#include "timpi/packing.h"
#include "timpi/message_tag.h"
#include "timpi/timpi_config.h"
#include "timpi/request.h"
#include "timpi/status.h"

// C++ includes
#include <map>
#include <memory> // shared_ptr
#include <string>
#include <vector>
#include <type_traits>

// C++ includes needed for parallel_communicator_specializations
//
// These could be forward declarations if only that wasn't illegal
#include <complex> // for specializations
#include <set>
#include <unordered_map>

namespace TIMPI
{

using libMesh::Parallel::Packing;
using libMesh::Parallel::Has_buffer_type;

// Define processor id storage type.
#if TIMPI_PROCESSOR_ID_BYTES == 1
typedef uint8_t processor_id_type;
#elif TIMPI_PROCESSOR_ID_BYTES == 2
typedef uint16_t processor_id_type;
#elif TIMPI_PROCESSOR_ID_BYTES == 4
typedef uint32_t processor_id_type; // default
#elif TIMPI_PROCESSOR_ID_BYTES == 8
typedef uint64_t processor_id_type;
#else
// We should not get here: it's not currently possible to set any
// other size.
DIE A HORRIBLE DEATH HERE...
#endif


#ifdef TIMPI_HAVE_MPI

//-------------------------------------------------------------------
/**
 * Communicator object for talking with subsets of processors
 */
typedef MPI_Comm communicator;

/**
 * Info object used by some MPI-3 methods
 */
typedef MPI_Info info;

/**
 * Processor id meaning "Accept from any source"
 */
const unsigned int any_source =
  static_cast<unsigned int>(MPI_ANY_SOURCE);

#else

// These shouldn't actually be needed, but must be
// unique types for function overloading to work
// properly.
typedef int communicator; // Must match petsc-nompi definition

typedef int info;

const unsigned int any_source=0;

#endif // TIMPI_HAVE_MPI

//-------------------------------------------------------------------
/**
 * Encapsulates the MPI_Comm object.  Allows the size of the group
 * and this process's position in the group to be determined.
 *
 * Methods of this object are the preferred way to perform
 * distributed-memory parallel operations.
 */
class Communicator
{
  // Basic operations:
public:

  /**
   * Default Constructor.
   */
  Communicator ();

  /*
   * Constructor from MPI_Comm
   */
  explicit Communicator (const communicator & comm);

  /*
   * Don't use copy construction or assignment, just copy by reference
   * or pointer - it's too hard to keep a common used_tag_values if
   * each communicator is shared by more than one Communicator
   */
  Communicator (const Communicator &) = delete;
  Communicator & operator= (const Communicator &) = delete;

  /*
   * Move constructor and assignment operator
   */
  Communicator (Communicator &&) = default;
  Communicator & operator= (Communicator &&) = default;

  /*
   * NON-VIRTUAL destructor
   */
  ~Communicator ();

  /*
   * Create a new communicator between some subset of \p this,
   * based on specified "color"
   */
  void split(int color, int key, Communicator & target) const;

  /*
   * Create a new communicator between some subset of \p this,
   * based on specified split "type" (e.g. MPI_COMM_TYPE_SHARED)
   */
  void split_by_type(int split_type, int key, info i, Communicator & target) const;

  /*
   * Make \p this a new duplicate of the Communicator \p comm -
   * sharing the same processes but with a new communication context
   * and without sharing unique MessageTag assignment.
   */
  void duplicate(const Communicator & comm);

  /*
   * Make \p this a new duplicate of the MPI communicator \p comm -
   * sharing the same processes but with a new communication context.
   */
  void duplicate(const communicator & comm);

  communicator & get() { return _communicator; }

  const communicator & get() const { return _communicator; }

  /**
   * Get a tag that is unique to this Communicator.  A requested tag
   * value may be provided.  If no request is made then an automatic
   * unique tag value will be generated; such usage of
   * get_unique_tag() must be done on every processor in a consistent
   * order.
   *
   * \note If people are also using magic numbers or copying
   * raw communicators around then we can't guarantee the tag is
   * unique to this MPI_Comm.
   *
   * \note Leaving \p tagvalue unspecified is recommended in most
   * cases.  Manually selecting tag values is dangerous, as tag values may be
   * freed and reselected earlier than expected in asynchronous
   * communication algorithms.
   */
  MessageTag get_unique_tag(int tagvalue = MessageTag::invalid_tag) const;

  /**
   * Reference an already-acquired tag, so that we know it will
   * be dereferenced multiple times before we can re-release it.
   */
  void reference_unique_tag(int tagvalue) const;

  /**
   * Dereference an already-acquired tag, and see if we can
   * re-release it.
   */
  void dereference_unique_tag(int tagvalue) const;

  /**
   * Free and reset this communicator
   */
  void clear();

  Communicator & operator= (const communicator & comm);

  processor_id_type rank() const { return _rank; }

  processor_id_type size() const { return _size; }

  /**
   * Whether to use default or synchronous sends?
   */
  enum SendMode { DEFAULT=0, SYNCHRONOUS };

  /**
   * What algorithm to use for parallel synchronization?
   */
  enum SyncType { NBX, ALLTOALL_COUNTS, SENDRECEIVE };


private:

  /**
   * Utility function for setting our member variables from an MPI
   * communicator
   */
  void assign(const communicator & comm);

  communicator  _communicator;
  processor_id_type _rank, _size;
  SendMode _send_mode;
  SyncType _sync_type;

  // mutable used_tag_values and tag_queue - not thread-safe, but then
  // TIMPI:: isn't thread-safe in general.
  mutable std::map<int, unsigned int> used_tag_values;
  mutable int _next_tag;

  int _max_tag;

  // Keep track of duplicate/split operations so we know when to free
  bool _I_duped_it;

  /**
   * Private implementation function called by the map-based sum()
   * specializations. This is_fixed_type variant saves a communication
   * by broadcasting pairs
   */
  template <typename Map,
            typename std::enable_if<std::is_base_of<DataType, StandardType<typename Map::key_type>>::value &&
                                    std::is_base_of<DataType, StandardType<typename Map::mapped_type>>::value,
                                    int>::type = 0>
  void map_sum(Map & data) const;

  /**
   * Private implementation function called by the map-based sum()
   * specializations. This !is_fixed_type variant calls allgather
   * twice: once for the keys and once for the values.
   */
  template <typename Map,
            typename std::enable_if<!(std::is_base_of<DataType, StandardType<typename Map::key_type>>::value &&
                                      std::is_base_of<DataType, StandardType<typename Map::mapped_type>>::value),
                                    int>::type = 0>
  void map_sum(Map & data) const;

  /**
   * Private implementation function called by the map-based broadcast()
   * specializations. This is_fixed_type variant saves a communication by broadcasting pairs
   */
  template <typename Map,
            typename std::enable_if<std::is_base_of<DataType, StandardType<typename Map::key_type>>::value &&
                                    std::is_base_of<DataType, StandardType<typename Map::mapped_type>>::value,
                                    int>::type = 0>
  void map_broadcast(Map & data,
                     const unsigned int root_id,
                     const bool identical_sizes) const;

  /**
   * Private implementation function called by the map-based broadcast()
   * specializations. This !is_fixed_type variant makes two broadcasts, which is slower
   * but gives more control over reach broadcast (e.g. we may need to specialize for either
   * key_type or mapped_type)
   */
  template <typename Map,
            typename std::enable_if<!(std::is_base_of<DataType, StandardType<typename Map::key_type>>::value &&
                                      std::is_base_of<DataType, StandardType<typename Map::mapped_type>>::value),
                                    int>::type = 0>
  void map_broadcast(Map & data,
                     const unsigned int root_id,
                     const bool identical_sizes) const;

  /**
   * Private implementation function called by the map-based max()
   * specializations.  This is_fixed_type variant saves a
   * communication by broadcasting pairs
   */
  template <typename Map,
            typename std::enable_if<std::is_base_of<DataType, StandardType<typename Map::key_type>>::value &&
                                    std::is_base_of<DataType, StandardType<typename Map::mapped_type>>::value,
                                    int>::type = 0>
  void map_max(Map & data) const;

  /**
   * Private implementation function called by the map-based max()
   * specializations.  This !is_fixed_type variant calls allgather
   * twice: once for the keys and once for the values.
   */
  template <typename Map,
            typename std::enable_if<!(std::is_base_of<DataType, StandardType<typename Map::key_type>>::value &&
                                      std::is_base_of<DataType, StandardType<typename Map::mapped_type>>::value),
                                    int>::type = 0>
  void map_max(Map & data) const;

  // Utility function for determining size for buffering of
  // vector<vector<T>> into vector<char> via MPI_Pack*
  template <typename T, typename A1, typename A2>
  int packed_size_of(const std::vector<std::vector<T,A1>,A2> & buf,
                     const DataType & type) const;

  // Communication operations:
public:

  /**
   * Explicitly sets the \p SendMode type used for send operations.
   */
  void send_mode (const SendMode sm) { _send_mode = sm; }

  /**
   * Gets the user-requested SendMode.
   */
  SendMode send_mode() const { return _send_mode; }

  /**
   * Explicitly sets the \p SyncType used for sync operations.
   */
  void sync_type (const SyncType st) { _sync_type = st; }

  /**
   * Sets the sync type used for sync operations via a string.
   *
   * Useful for changing the sync type via a CLI arg or parameter.
   */
  void sync_type (const std::string & st);

  /**
   * Gets the user-requested SyncType.
   */
  SyncType sync_type() const { return _sync_type; }

  /**
   * Pause execution until all processors reach a certain point.
   */
  void barrier () const;

  /**
   * Start a barrier that doesn't block
   */
  void nonblocking_barrier (Request & req) const;

  /**
   * Verify that a local variable has the same value on all processors.
   * Containers must have the same value in every entry.
   */
  template <typename T>
  inline
  bool verify(const T & r) const;

  /**
   * Verify that a local pointer points to the same value on all
   * processors where it is not nullptr.
   * Containers must have the same value in every entry.
   */
  template <typename T>
  inline
  bool semiverify(const T * r) const;

  /**
   * Non-blocking minimum of the local value \p r into \p o
   * with the request \p req.
   */
  template <typename T>
  inline
  void min(const T & r, T & o, Request & req) const;

  /**
   * Take a local variable and replace it with the minimum of it's values
   * on all processors.  Containers are replaced element-wise.
   */
  template <typename T>
  inline
  void min(T & r) const;

  /**
   * Take a local variable and replace it with the minimum of it's values
   * on all processors, returning the minimum rank of a processor
   * which originally held the minimum value.
   */
  template <typename T>
  inline
  void minloc(T & r,
              unsigned int & min_id) const;

  /**
   * Take a vector of local variables and replace each entry with the minimum
   * of it's values on all processors.  Set each \p min_id entry to
   * the minimum rank where a corresponding minimum was found.
   */
  template <typename T, typename A1, typename A2>
  inline
  void minloc(std::vector<T,A1> & r,
              std::vector<unsigned int,A2> & min_id) const;

  /**
   * Non-blocking maximum of the local value \p r into \p o
   * with the request \p req.
   */
  template <typename T>
  inline
  void max(const T & r, T & o, Request & req) const;

  /**
   * Take a local variable and replace it with the maximum of it's values
   * on all processors.  Containers are replaced element-wise.
   */
  template <typename T>
  inline
  void max(T & r) const;

  /**
   * Take a local variable and replace it with the maximum of it's values
   * on all processors, returning the minimum rank of a processor
   * which originally held the maximum value.
   */
  template <typename T>
  inline
  void maxloc(T & r,
              unsigned int & max_id) const;

  /**
   * Take a vector of local variables and replace each entry with the maximum
   * of it's values on all processors.  Set each \p min_id entry to
   * the minimum rank where a corresponding maximum was found.
   */
  template <typename T, typename A1, typename A2>
  inline
  void maxloc(std::vector<T,A1> & r,
              std::vector<unsigned int,A2> & max_id) const;

  /**
   * Take a local variable and replace it with the sum of it's values
   * on all processors.  Containers are replaced element-wise.
   */
  template <typename T>
  inline
  void sum(T & r) const;

  /**
   * Non-blocking sum of the local value \p r into \p o
   * with the request \p req.
   */
  template <typename T>
  inline
  void sum(const T & r, T & o, Request & req) const;

  /**
   * Take a container (set, map, unordered_set, multimap, etc) of
   * local variables on each processor, and collect their union over
   * all processors, replacing the original on processor 0.
   *
   * If the \p data is a map or unordered_map and entries exist on
   * different processors with the same key and different values, then
   * the value with the lowest processor id takes precedence.
   */
  template <typename T>
  inline
  void set_union(T & data, const unsigned int root_id) const;

  /**
   * Take a container of local variables on each processor, and
   * replace it with their union over all processors, replacing the
   * original on all processors.
   */
  template <typename T>
  inline
  void set_union(T & data) const;

  /**
   * Blocking message probe.  Allows information about a message to be
   * examined before the message is actually received.
   */
  status probe (const unsigned int src_processor_id,
                const MessageTag & tag=any_tag) const;

  /**
   * Non-Blocking message probe for a packed range message.
   * Allows information about a message to be
   * examined before the message is actually received.
   *
   * Template type must match the object type that will be in
   * the packed range
   *
   * \param src_processor_id The processor the message is expected from or TIMPI::any_source
   * \param tag The message tag or TIMPI::any_tag
   * \param flag Output.  True if a message exists.  False otherwise.
   */
  template <typename T>
  inline
  Status packed_range_probe (const unsigned int src_processor_id,
                             const MessageTag & tag,
                             bool & flag) const;

  /**
   * Blocking-send to one processor with data-defined type.
   */
  template <typename T>
  inline
  void send (const unsigned int dest_processor_id,
             const T & buf,
             const MessageTag & tag=no_tag) const;

  /**
   * Nonblocking-send to one processor with data-defined type.
   */
  template <typename T>
  inline
  void send (const unsigned int dest_processor_id,
             const T & buf,
             Request & req,
             const MessageTag & tag=no_tag) const;

  /**
   * Blocking-send to one processor with user-defined type.
   *
   * If \p T is a container, container-of-containers, etc., then
   * \p type should be the DataType of the underlying fixed-size
   * entries in the container(s).
   */
  template <typename T>
  inline
  void send (const unsigned int dest_processor_id,
             const T & buf,
             const DataType & type,
             const MessageTag & tag=no_tag) const;

  /**
   * Nonblocking-send to one processor with user-defined type.
   *
   * If \p T is a container, container-of-containers, etc., then
   * \p type should be the DataType of the underlying fixed-size
   * entries in the container(s).
   */
  template <typename T>
  inline
  void send (const unsigned int dest_processor_id,
             const T & buf,
             const DataType & type,
             Request & req,
             const MessageTag & tag=no_tag) const;

  /**
   * Nonblocking-send to one processor with user-defined packable type.
   * \p Packing<T> must be defined for \p T
   */
  template <typename T>
  inline
  void send (const unsigned int dest_processor_id,
             const T & buf,
             const NotADataType & type,
             Request & req,
             const MessageTag & tag=no_tag) const;

  /**
   * Blocking-receive from one processor with data-defined type.
   */
  template <typename T>
  inline
  Status receive (const unsigned int dest_processor_id,
                  T & buf,
                  const MessageTag & tag=any_tag) const;

  /**
   * Nonblocking-receive from one processor with data-defined type.
   */
  template <typename T>
  inline
  void receive (const unsigned int dest_processor_id,
                T & buf,
                Request & req,
                const MessageTag & tag=any_tag) const;

  /**
   * Blocking-receive from one processor with user-defined type.
   *
   * If \p T is a container, container-of-containers, etc., then
   * \p type should be the DataType of the underlying fixed-size
   * entries in the container(s).
   */
  template <typename T>
  inline
  Status receive (const unsigned int dest_processor_id,
                  T & buf,
                  const DataType & type,
                  const MessageTag & tag=any_tag) const;

  /**
   * Nonblocking-receive from one processor with user-defined type.
   *
   * If \p T is a container, container-of-containers, etc., then
   * \p type should be the DataType of the underlying fixed-size
   * entries in the container(s).
   */
  template <typename T>
  inline
  void receive (const unsigned int dest_processor_id,
                T & buf,
                const DataType & type,
                Request & req,
                const MessageTag & tag=any_tag) const;

  /**
   * Nonblocking-receive from one processor with user-defined type.
   *
   * Checks to see if a message can be received from the
   * src_processor_id .  If so, it starts a non-blocking
   * receive using the passed in request and returns true
   *
   * Otherwise - if there is no message to receive it returns false
   *
   * Note: The buf does NOT need to be properly sized before this call
   * this will resize the buffer automatically
   *
   * @param src_processor_id The pid to receive from or "any".
   * will be set to the actual src being received from
   * @param buf The buffer to receive into
   * @param req The request to use
   * @param tag The tag to use
   */
  template <typename T, typename A,
            typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type = 0>
  inline
  bool possibly_receive (unsigned int & src_processor_id,
                         std::vector<T,A> & buf,
                         Request & req,
                         const MessageTag & tag) const;

  /**
   * dispatches to \p possibly_receive_packed_range
   * @param src_processor_id The pid to receive from or "any".
   * will be set to the actual src being received from
   * @param buf The buffer to receive into
   * @param req The request to use
   * @param tag The tag to use
   */
  template <typename T, typename A,
            typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type = 0>
  inline
  bool possibly_receive (unsigned int & src_processor_id,
                         std::vector<T,A> & buf,
                         Request & req,
                         const MessageTag & tag) const;


  /**
   * Nonblocking-receive from one processor with user-defined type.
   *
   * As above, but with manually-specified data type.
   *
   * @param src_processor_id The pid to receive from or "any".
   * will be set to the actual src being received from
   * @param buf The buffer to receive into
   * @param type The intrinsic datatype to receive
   * @param req The request to use
   * @param tag The tag to use

   */
  template <typename T, typename A, typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type = 0>
  inline
  bool possibly_receive (unsigned int & src_processor_id,
                         std::vector<T,A> & buf,
                         const DataType & type,
                         Request & req,
                         const MessageTag & tag) const;

  /**
   * Nonblocking-receive from one processor with user-defined type. Dispatches to \p
   * possibly_receive_packed_range
   *
   * @param src_processor_id The pid to receive from or "any".
   * will be set to the actual src being received from
   * @param buf The buffer to receive into
   * @param type The packable type to receive
   * @param req The request to use
   * @param tag The tag to use

   */
  template <typename T, typename A,
            typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type = 0>
  inline
  bool possibly_receive (unsigned int & src_processor_id,
                         std::vector<T,A> & buf,
                         const NotADataType & type,
                         Request & req,
                         const MessageTag & tag) const;

 /**
  * Nonblocking packed range receive from one processor with
  * user-defined type.
  *
  * Checks to see if a message can be received from the
  * src_processor_id .  If so, it starts a nonblocking
  * packed range receive using the passed in request and
  * returns true
  *
  * Otherwise - if there is no message to receive it returns false
  *
  * void Parallel::unpack(const T *, OutputIter data, const Context *)
  * is used to unserialize type T
  *
  * @param src_processor_id The pid to receive from or "any".
  * will be set to the actual src being receieved from
  * @param context Context pointer that will be passed into
  * the unpack functions
  * @param out The output iterator
  * @param output_type The intrinsic datatype to receive
  * @param req The request to use
  * @param tag The tag to use
  */
  template <typename Context, typename OutputIter, typename T>
  bool possibly_receive_packed_range (unsigned int & src_processor_id,
                                      Context * context,
                                      OutputIter out,
                                      const T * output_type,
                                      Request & req,
                                      const MessageTag & tag) const;

  /**
   * Blocking-send range-of-pointers to one processor.  This
   * function does not send the raw pointers, but rather constructs
   * new objects at the other end whose contents match the objects
   * pointed to by the sender.
   *
   * void TIMPI::pack(const T *, vector<int> & data, const Context *)
   * is used to serialize type T onto the end of a data vector.
   *
   * unsigned int TIMPI::packable_size(const T *, const Context *) is
   * used to allow data vectors to reserve memory, and for additional
   * error checking
   *
   * The approximate maximum size (in *entries*; number of bytes will
   * likely be 4x or 8x larger) to use in a single data vector buffer
   * can be specified for performance or memory usage reasons; if the
   * range cannot be packed into a single buffer of this size then
   * multiple buffers and messages will be used.
   */
  template <typename Context, typename Iter>
  inline
  void send_packed_range (const unsigned int dest_processor_id,
                          const Context * context,
                          Iter range_begin,
                          const Iter range_end,
                          const MessageTag & tag=no_tag,
                          std::size_t approx_buffer_size = 1000000) const;

  /**
   * Nonblocking-send range-of-pointers to one processor.  This
   * function does not send the raw pointers, but rather constructs
   * new objects at the other end whose contents match the objects
   * pointed to by the sender.
   *
   * void TIMPI::pack(const T *, vector<int> & data, const Context *)
   * is used to serialize type T onto the end of a data vector.
   *
   * unsigned int TIMPI::packable_size(const T *, const Context *) is
   * used to allow data vectors to reserve memory, and for additional
   * error checking
   *
   * The approximate maximum size (in *entries*; number of bytes will
   * likely be 4x or 8x larger) to use in a single data vector buffer
   * can be specified for performance or memory usage reasons; if the
   * range cannot be packed into a single buffer of this size then
   * multiple buffers and messages will be used.
   */
  template <typename Context, typename Iter>
  inline
  void send_packed_range (const unsigned int dest_processor_id,
                          const Context * context,
                          Iter range_begin,
                          const Iter range_end,
                          Request & req,
                          const MessageTag & tag=no_tag,
                          std::size_t approx_buffer_size = 1000000) const;

  /**
   * Similar to the above Nonblocking send_packed_range with a few important differences:
   *
   * 1. The total size of the packed buffer MUST be less than std::numeric_limits<int>::max()
   * 2. Only _one_ message is generated
   * 3. On the receiving end the message should be tested for using Communicator::packed_range_probe()
   * 4. The message must be received by Communicator::nonblocking_receive_packed_range()
   */
  template <typename Context, typename Iter>
  inline
  void nonblocking_send_packed_range (const unsigned int dest_processor_id,
                                      const Context * context,
                                      Iter range_begin,
                                      const Iter range_end,
                                      Request & req,
                                      const MessageTag & tag=no_tag) const;


  /**
   * Similar to the above Nonblocking send_packed_range with a few important differences:
   *
   * 1. The total size of the packed buffer MUST be less than std::numeric_limits<int>::max()
   * 2. Only _one_ message is generated
   * 3. On the receiving end the message should be tested for using Communicator::packed_range_probe()
   * 4. The message must be received by Communicator::nonblocking_receive_packed_range()
   */
  template <typename Context, typename Iter>
  inline
  void nonblocking_send_packed_range (const unsigned int dest_processor_id,
                                      const Context * context,
                                      Iter range_begin,
                                      const Iter range_end,
                                      Request & req,
                                      std::shared_ptr<std::vector<typename TIMPI::Packing<typename std::iterator_traits<Iter>::value_type>::buffer_type>> & buffer,
                                      const MessageTag & tag=no_tag) const;

  /**
   * Blocking-receive range-of-pointers from one processor.  This
   * function does not receive raw pointers, but rather constructs new
   * objects whose contents match the objects pointed to by the
   * sender.
   *
   * The objects will be of type
   * T = iterator_traits<OutputIter>::value_type.
   *
   * Using std::back_inserter as the output iterator allows receive to
   * fill any container type.  Using some null_output_iterator
   * allows the receive to be dealt with solely by TIMPI::unpack(),
   * for objects whose unpack() is written so as to not leak memory
   * when used in this fashion.
   *
   * A future version of this method should be created to preallocate
   * memory when receiving vectors...
   *
   * void TIMPI::unpack(vector<int>::iterator in, T ** out, Context *)
   * is used to unserialize type T, typically into a new
   * heap-allocated object whose pointer is returned as *out.
   *
   * unsigned int TIMPI::packed_size(const T *,
   *                                    vector<int>::const_iterator)
   * is used to advance to the beginning of the next object's data.
   */
  template <typename Context, typename OutputIter, typename T>
  inline
  void receive_packed_range (const unsigned int dest_processor_id,
                             Context * context,
                             OutputIter out,
                             const T * output_type, // used only to infer T
                             const MessageTag & tag=any_tag) const;

  /**
   * Non-Blocking-receive range-of-pointers from one processor.
   *
   * This is meant to receive messages from nonblocking_send_packed_range
   *
   * Similar in design to the above receive_packed_range.  However,
   * this version requires a Request and a Status.
   *
   * The Status must be a positively tested Status for a message of this
   * type (i.e. a message _does_ exist).  It should most likely be generated by
   * Communicator::packed_range_probe.
   */
  template <typename Context, typename OutputIter, typename T>
  inline
  void nonblocking_receive_packed_range (const unsigned int src_processor_id,
                                         Context * context,
                                         OutputIter out,
                                         const T * output_type,
                                         Request & req,
                                         Status & stat,
                                         const MessageTag & tag=any_tag) const;

  /**
   * Non-Blocking-receive range-of-pointers from one processor.
   *
   * This is meant to receive messages from nonblocking_send_packed_range
   *
   * Similar in design to the above receive_packed_range.  However,
   * this version requires a Request and a Status.
   *
   * The Status must be a positively tested Status for a message of this
   * type (i.e. a message _does_ exist).  It should most likely be generated by
   * Communicator::packed_range_probe.
   */
  template <typename Context, typename OutputIter, typename T>
  inline
  void nonblocking_receive_packed_range (const unsigned int src_processor_id,
                                         Context * context,
                                         OutputIter out,
                                         const T * output_type,
                                         Request & req,
                                         Status & stat,
                                         std::shared_ptr<std::vector<typename TIMPI::Packing<T>::buffer_type>> & buffer,
                                         const MessageTag & tag=any_tag
                                         ) const;

  /**
   * Send data \p send to one processor while simultaneously receiving
   * other data \p recv from a (potentially different) processor.
   *
   * This overload is defined for fixed-size data; other overloads
   * exist for many other categories.
   */
  template <typename T1, typename T2,
            typename std::enable_if<std::is_base_of<DataType, StandardType<T1>>::value &&
                                    std::is_base_of<DataType, StandardType<T2>>::value,
                                    int>::type = 0>
  inline
  void send_receive(const unsigned int dest_processor_id,
                    const T1 & send_data,
                    const unsigned int source_processor_id,
                    T2 & recv_data,
                    const MessageTag & send_tag = no_tag,
                    const MessageTag & recv_tag = any_tag) const;

  /**
   * Send a range-of-pointers to one processor while simultaneously receiving
   * another range from a (potentially different) processor.  This
   * function does not send or receive raw pointers, but rather constructs
   * new objects at each receiver whose contents match the objects
   * pointed to by the sender.
   *
   * The objects being sent will be of type
   * T1 = iterator_traits<RangeIter>::value_type, and the objects
   * being received will be of type
   * T2 = iterator_traits<OutputIter>::value_type
   *
   * void TIMPI::pack(const T1*, vector<int> & data, const Context1*)
   * is used to serialize type T1 onto the end of a data vector.
   *
   * Using std::back_inserter as the output iterator allows
   * send_receive to fill any container type.  Using some
   * null_output_iterator allows the receive to be dealt with
   * solely by TIMPI::unpack(), for objects whose unpack() is
   * written so as to not leak memory when used in this fashion.
   *
   * A future version of this method should be created to preallocate
   * memory when receiving vectors...
   *
   * void TIMPI::unpack(vector<int>::iterator in, T2** out, Context *)
   * is used to unserialize type T2, typically into a new
   * heap-allocated object whose pointer is returned as *out.
   *
   * unsigned int TIMPI::packable_size(const T1*, const Context1*)
   * is used to allow data vectors to reserve memory, and for
   * additional error checking.
   *
   * unsigned int TIMPI::packed_size(const T2*,
   *                                    vector<int>::const_iterator)
   * is used to advance to the beginning of the next object's data.
   */
  template <typename Context1, typename RangeIter, typename Context2,
            typename OutputIter, typename T>
  inline
  void send_receive_packed_range(const unsigned int dest_processor_id,
                                 const Context1 * context1,
                                 RangeIter send_begin,
                                 const RangeIter send_end,
                                 const unsigned int source_processor_id,
                                 Context2 * context2,
                                 OutputIter out,
                                 const T * output_type, // used only to infer T
                                 const MessageTag & send_tag = no_tag,
                                 const MessageTag & recv_tag = any_tag,
                                 std::size_t approx_buffer_size = 1000000) const;

  /**
   * Send data \p send to one processor while simultaneously receiving
   * other data \p recv from a (potentially different) processor, using
   * a user-specified MPI Dataype.
   */
  template <typename T1, typename T2>
  inline
  void send_receive(const unsigned int dest_processor_id,
                    const T1 & send_data,
                    const DataType & type1,
                    const unsigned int source_processor_id,
                    T2 & recv_data,
                    const DataType & type2,
                    const MessageTag & send_tag = no_tag,
                    const MessageTag & recv_tag = any_tag) const;

  /**
   * Take a vector of length comm.size(), and on processor root_id fill in
   * recv[processor_id] = the value of send on processor processor_id
   */
  template <typename T, typename A>
  inline void gather(const unsigned int root_id,
                     const T & send_data,
                     std::vector<T,A> & recv) const;

  /**
   * The gather overload for string types has an optional
   * identical_buffer_sizes optimization for when all strings are the
   * same length.
   */
  template <typename T, typename A>
  inline void gather(const unsigned int root_id,
                     const std::basic_string<T> & send_data,
                     std::vector<std::basic_string<T>,A> & recv_data,
                     const bool identical_buffer_sizes=false) const;

  /**
   * Take a vector of local variables and expand it on processor root_id
   * to include values from all processors
   *
   * This handles the case where the lengths of the vectors may vary.
   * Specifically, this function transforms this:
   * \verbatim
   * Processor 0: [ ... N_0 ]
   * Processor 1: [ ....... N_1 ]
   * ...
   * Processor M: [ .. N_M]
   * \endverbatim
   *
   * into this:
   *
   * \verbatim
   * [ [ ... N_0 ] [ ....... N_1 ] ... [ .. N_M] ]
   * \endverbatim
   *
   * on processor root_id. This function is collective and therefore
   * must be called by all processors in the Communicator.
   *
   * If the type T is a standard (fixed-size) type then we use a
   * standard MPI_Gatherv; if it is a packable variable-size type then
   * we dispatch to gather_packed_range.
   */
  template <typename T, typename A,
            typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type = 0>
  inline void gather(const unsigned int root_id,
                     std::vector<T,A> & r) const;

  template <typename T, typename A,
            typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type = 0>
  inline void gather(const unsigned int root_id,
                     std::vector<T,A> & r) const;

  /**
   * Take a vector of length \p this->size(), and fill in
   * \p recv[processor_id] = the value of \p send on that processor. This
   * overload works on fixed size types
   */
  template <typename T, typename A, typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value,
                                                            int>::type = 0>
  inline void allgather(const T & send_data,
                        std::vector<T,A> & recv_data) const;

  /**
   * Take a vector of length \p this->size(), and fill in
   * \p recv[processor_id] = the value of \p send on that processor. This
   * overload works on potentially dynamically sized types, and dispatches
   * to \p allgather_packed_range
   */
  template <typename T, typename A, typename std::enable_if<Has_buffer_type<Packing<T>>::value,
                                                            int>::type = 0>
  inline void allgather(const T & send_data,
                        std::vector<T,A> & recv_data) const;

  /**
   * The allgather overload for string types has an optional
   * identical_buffer_sizes optimization for when all strings are the
   * same length.
   */
  template <typename T, typename A>
  inline void allgather(const std::basic_string<T> & send_data,
                        std::vector<std::basic_string<T>,A> & recv_data,
                        const bool identical_buffer_sizes=false) const;

  /**
   * Take a vector of fixed size local variables and expand it to include
   * values from all processors. By default, each processor is
   * allowed to have its own unique input buffer length. If
   * it is known that all processors have the same input sizes
   * additional communication can be avoided.
   *
   * Specifically, this function transforms this:
   * \verbatim
   * Processor 0: [ ... N_0 ]
   * Processor 1: [ ....... N_1 ]
   * ...
   * Processor M: [ .. N_M]
   * \endverbatim
   *
   * into this:
   *
   * \verbatim
   * [ [ ... N_0 ] [ ....... N_1 ] ... [ .. N_M] ]
   * \endverbatim
   *
   * on each processor. This function is collective and therefore
   * must be called by all processors in the Communicator.
   */
  template <typename T, typename A,
            typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type = 0>
  inline void allgather(std::vector<T,A> & r,
                        const bool identical_buffer_sizes = false) const;

  /**
   * Take a vector of fixed size local variables and collect similar
   * vectors from all processors. By default, each processor is
   * allowed to have its own unique input buffer length. If
   * it is known that all processors have the same input sizes
   * additional communication can be avoided.
   */
  template <typename T, typename A1, typename A2,
            typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type = 0>
  inline void allgather(const std::vector<T,A1> & send_data,
                        std::vector<std::vector<T,A1>, A2> & recv_data,
                        const bool identical_buffer_sizes = false) const;

  /**
   * Take a vector of dynamic-size local variables and collect similar
   * vectors from all processors.
   */
  template <typename T, typename A1, typename A2,
            typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type = 0>
  inline void allgather(const std::vector<T,A1> & send_data,
                        std::vector<std::vector<T,A1>, A2> & recv_data,
                        const bool identical_buffer_sizes = false) const;

  /**
   * Take a vector of possibly dynamically sized local variables and expand it
   * to include values from all processors. By default, each processor is
   * allowed to have its own unique input buffer length. If it is known that all
   * processors have the same input sizes additional communication can be
   * avoided.
   *
   * Specifically, this function transforms this:
   * \verbatim
   * Processor 0: [ ... N_0 ]
   * Processor 1: [ ....... N_1 ]
   * ...
   * Processor M: [ .. N_M]
   * \endverbatim
   *
   * into this:
   *
   * \verbatim
   * [ [ ... N_0 ] [ ....... N_1 ] ... [ .. N_M] ]
   * \endverbatim
   *
   * on each processor. This function is collective and therefore
   * must be called by all processors in the Communicator.
   */
  template <typename T, typename A,
            typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type = 0>
  inline void allgather(std::vector<T,A> & r,
                        const bool identical_buffer_sizes = false) const;

  /**
   * AllGather overload for vectors of string types
   */
  template <typename T, typename A>
  inline void allgather(std::vector<std::basic_string<T>,A> & r,
                        const bool identical_buffer_sizes = false) const;

  //-------------------------------------------------------------------
  /**
   * Take a vector of local variables and scatter the ith item to the ith
   * processor in the communicator. The result is saved into recv.
   */
  template <typename T, typename A>
  inline void scatter(const std::vector<T,A> & data,
                      T & recv,
                      const unsigned int root_id=0) const;

  /**
   * Take a vector of local variables and scatter the ith equal-sized chunk
   * to the ith processor in the communicator. The data size must be a
   * multiple of the communicator size. The result is saved into recv buffer.
   * The recv buffer does not have to be sized prior to this operation.
   */
  template <typename T, typename A>
  inline void scatter(const std::vector<T,A> & data,
                      std::vector<T,A> & recv,
                      const unsigned int root_id=0) const;

  /**
   * Take a vector of local variables and scatter the ith variable-sized chunk
   * to the ith processor in the communicator. The counts vector should contain
   * the number of items for each processor. The result is saved into recv buffer.
   * The recv buffer does not have to be sized prior to this operation.
   */
  template <typename T, typename A1, typename A2>
  inline void scatter(const std::vector<T,A1> & data,
                      const std::vector<int,A2> counts,
                      std::vector<T,A1> & recv,
                      const unsigned int root_id=0) const;

  /**
   * Take a vector of vectors and scatter the ith inner vector
   * to the ith processor in the communicator. The result is saved into recv buffer.
   * The recv buffer does not have to be sized prior to this operation.
   */
  template <typename T, typename A1, typename A2>
  inline void scatter(const std::vector<std::vector<T,A1>,A2> & data,
                      std::vector<T,A1> & recv,
                      const unsigned int root_id=0,
                      const bool identical_buffer_sizes=false) const;

  //-------------------------------------------------------------------
  /**
   * Take a range of local variables, combine it with ranges from all
   * processors, and write the output to the output iterator on rank root.
   *
   * The approximate maximum size (in *entries*; number of bytes will
   * likely be 4x or 8x larger) to use in a single data vector buffer
   * to send can be specified for performance or memory usage reasons;
   * if the range cannot be packed into a single buffer of this size
   * then multiple buffers and messages will be used.
   *
   * Note that the received data vector sizes will be the *sum* of the
   * sent vector sizes; a smaller-than-default size may be useful for
   * users on many processors, in cases where all-to-one communication
   * cannot be avoided entirely.
   */
  template <typename Context, typename Iter, typename OutputIter>
  inline void gather_packed_range (const unsigned int root_id,
                                   Context * context,
                                   Iter range_begin,
                                   const Iter range_end,
                                   OutputIter out,
                                   std::size_t approx_buffer_size = 1000000) const;

  /**
   * Take a range of local variables, combine it with ranges from all
   * processors, and write the output to the output iterator.
   *
   * The approximate maximum size (in *entries*; number of bytes will
   * likely be 4x or 8x larger) to use in a single data vector buffer
   * to send can be specified for performance or memory usage reasons;
   * if the range cannot be packed into a single buffer of this size
   * then multiple buffers and messages will be used.
   *
   * Note that the received data vector sizes will be the *sum* of the
   * sent vector sizes; a smaller-than-default size may be useful for
   * users on many processors, in cases where all-to-one communication
   * cannot be avoided entirely.
   */
  template <typename Context, typename Iter, typename OutputIter>
  inline void allgather_packed_range (Context * context,
                                      Iter range_begin,
                                      const Iter range_end,
                                      OutputIter out,
                                      std::size_t approx_buffer_size = 1000000) const;

  /**
   * Effectively transposes the input vector across all processors.
   * The jth entry on processor i is replaced with the ith entry
   * from processor j.
   */
  template <typename T, typename A>
  inline void alltoall(std::vector<T,A> & r) const;

  /**
   * Take a local value and broadcast it to all processors.
   * Optionally takes the \p root_id processor, which specifies
   * the processor initiating the broadcast.
   *
   * If \p data is a container, it will be resized on target
   * processors.  When using pre-sized target containers, specify
   * \p identical_sizes=true on all processors for an optimization.
   *
   * Fixed variant
   */
  template <typename T
#ifdef TIMPI_HAVE_MPI
            ,
            typename std::enable_if<std::is_base_of<DataType, StandardType<T>>::value, int>::type = 0
#endif
            >
  inline void broadcast(T & data, const unsigned int root_id=0,
                        const bool identical_sizes=false) const;

#ifdef TIMPI_HAVE_MPI
  /**
   * Take a possibly dynamically-sized local value and broadcast it to all
   * processors.  Optionally takes the \p root_id processor, which specifies the
   * processor initiating the broadcast.
   *
   * If \p data is a container, it will be resized on target
   * processors.  When using pre-sized target containers, specify
   * \p identical_sizes=true on all processors for an optimization.
   *
   * Dynamic variant
   */
  template <typename T,
            typename std::enable_if<Has_buffer_type<Packing<T>>::value, int>::type = 0>
  inline void broadcast(T & data, const unsigned int root_id=0,
                        const bool identical_sizes=false) const;
#endif

  /**
   * Blocking-broadcast range-of-pointers to one processor.  This
   * function does not send the raw pointers, but rather constructs
   * new objects at the other end whose contents match the objects
   * pointed to by the sender.
   *
   * void TIMPI::pack(const T *, vector<int> & data, const Context *)
   * is used to serialize type T onto the end of a data vector.
   *
   * unsigned int TIMPI::packable_size(const T *, const Context *) is
   * used to allow data vectors to reserve memory, and for additional
   * error checking
   *
   * unsigned int TIMPI::packed_size(const T *,
   *                                    vector<int>::const_iterator)
   * is used to advance to the beginning of the next object's data.
   *
   * The approximate maximum size (in *entries*; number of bytes will
   * likely be 4x or 8x larger) to use in a single data vector buffer
   * can be specified for performance or memory usage reasons; if the
   * range cannot be packed into a single buffer of this size then
   * multiple buffers and messages will be used.
   */
  template <typename Context, typename OutputContext, typename Iter, typename OutputIter>
  inline void broadcast_packed_range (const Context * context1,
                                      Iter range_begin,
                                      const Iter range_end,
                                      OutputContext * context2,
                                      OutputIter out,
                                      const unsigned int root_id = 0,
                                      std::size_t approx_buffer_size = 1000000) const;

  /**
   * C++ doesn't let us partially specialize functions (we're really
   * just doing operator overloading on them), so more-specialized
   * versions of the functions above need to be redeclared.  We'll
   * do so in a separate file so that users don't have to look at
   * the redundancy.
   */
#include "timpi/parallel_communicator_specializations"

}; // class Communicator


} // namespace TIMPI


// Backwards compatibility for libMesh forward declarations
namespace libMesh {
namespace Parallel {
class Communicator : public TIMPI::Communicator {
public:
  Communicator () = default;
  explicit Communicator (const TIMPI::communicator & comm) : TIMPI::Communicator(comm) {}
  Communicator (const Communicator &) = delete;
  Communicator & operator= (const Communicator &) = delete;
  Communicator (Communicator &&) = default;
  Communicator & operator= (Communicator &&) = default;
  ~Communicator () = default;
};
}
}

#endif // TIMPI_COMMUNICATOR_H
