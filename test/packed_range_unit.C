#include <timpi/parallel_sync.h>
#include <timpi/timpi.h>
#include <timpi/packing.h>

#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <memory>

#define TIMPI_UNIT_ASSERT(expr) \
  if (!(expr)) \
    timpi_error()

std::string stringy_number(int number)
{
  std::string digit_strings [10] = {"zero", "one", "two",
      "three", "four", "five", "six", "seven", "eight", "nine"};

  std::string returnval = "done";
  while (number)
    {
      returnval = digit_strings[number%10]+" "+returnval;
      number = number/10;
    };

  return returnval;
}


std::vector<char> fake_stringy_number(int number)
{
  const std::string returnval = stringy_number(number);
  return std::vector<char>(returnval.begin(), returnval.end());
}

template <typename T>
struct null_output_iterator
  : std::iterator<std::output_iterator_tag, T>
{
  template <typename T2>
  void operator=(const T2&) {}

  null_output_iterator & operator++() {
    return *this;
  }

  null_output_iterator operator++(int) {
    return null_output_iterator(*this);
  }

  // We don't return a reference-to-T here because we don't want to
  // construct one or have any of its methods called.
  null_output_iterator & operator*() { return *this; }
};


template <int i, typename T>
inline
auto my_get(T & container) -> decltype(std::get<i>(container))
{
  return std::get<i>(container);
}


template <int i, typename T>
inline
auto my_get(T & container) -> decltype(*container.begin())
{
  auto fwd_it = container.begin();
  for (int n_it = 0; n_it < i; ++n_it)
    ++fwd_it;
  return *fwd_it;
}


// Need to disambiguate - array has get() and begin()...
template <int i, typename T, std::size_t N>
inline
auto my_get(std::array<T, N> & container) -> decltype(std::get<i>(container))
{
  return std::get<i>(container);
}


template <typename T>
inline void my_resize(T & container, std::size_t size) { container.resize(size); }

template <typename T, typename U>
inline void my_resize(std::pair<T,U> &, std::size_t) {}

template <typename ... Types>
inline void my_resize(std::tuple<Types...> &, std::size_t) {}

template <typename T, std::size_t N>
inline void my_resize(std::array<T,N> &, std::size_t) {}


#if __cplusplus > 201402L
namespace libMesh
{
namespace Parallel
{
template <>
class Packing<std::unique_ptr<int>>
{
public:
  typedef int buffer_type;

  static unsigned int packed_size(typename std::vector<int>::const_iterator) { return 1; }
  static unsigned int packable_size(const std::unique_ptr<int> &, const void *) { return 1; }

  template <typename Iter>
  static void pack(const std::unique_ptr<int> & object, Iter data_out, const void *) { data_out = *object; }

  template <typename BufferIter>
  static std::unique_ptr<int> unpack(BufferIter in, const void *)
    {
      return std::make_unique<int>(*in++);
    }
};
}
}
#endif

using namespace TIMPI;

Communicator *TestCommWorld;

  void testNullAllGather()
  {
    std::vector<std::string> send(1);
    if (TestCommWorld->rank() == 0)
      send[0].assign("Hello");
    else
      send[0].assign("Goodbye");

    TestCommWorld->allgather_packed_range
      ((void *)(NULL), send.begin(), send.end(),
       null_output_iterator<std::string>());
  }


  // Make sure we don't have problems with strings of length above 256
  // inside pairs, like we used to.
  template <typename PairAtLeast>
  void testGettableStringAllGather()
  {
    std::vector<PairAtLeast> sendv(2);
    my_resize(sendv[0], 2);
    my_resize(sendv[1], 2);

    my_get<0>(sendv[0]).assign("Hello");
    auto & s0 = my_get<1>(sendv[0]);
    s0.assign("Is it me you're looking for?\n");
    for (int i=0; i != 6; ++i)
      s0 = s0+s0;
    timpi_assert_greater(s0.size(), 256);

    my_get<0>(sendv[1]).assign("Goodbye");
    auto & s1 = my_get<1>(sendv[1]);
    s1.assign("to you!  Guess it's better to say, goodbye\n");
    for (int i=0; i != 6; ++i)
      s1 = s1+s1;
    timpi_assert_greater(s1.size(), 256);

    std::vector<PairAtLeast> send(1);
    my_resize(send[0], 2);

    if (TestCommWorld->rank() == 0)
      send[0] = sendv[0];
    else
      send[0] = sendv[1];

    std::vector<PairAtLeast> recv;

    TestCommWorld->allgather_packed_range
      ((void *)(NULL), send.begin(), send.end(),
       std::back_inserter(recv));

    const std::size_t comm_size = TestCommWorld->size();
    const std::size_t vec_size = recv.size();
    TIMPI_UNIT_ASSERT(comm_size == vec_size);

    TIMPI_UNIT_ASSERT(sendv[0] == recv[0]);
    for (std::size_t i=1; i < vec_size; ++i)
      TIMPI_UNIT_ASSERT(sendv[1] == recv[i]);
  }


  void testPairStringAllGather()
  {
    testGettableStringAllGather<std::pair<std::string, std::string>>();
  }


  void testArrayStringAllGather()
  {
    testGettableStringAllGather<std::array<std::string, 4>>();
  }


  void testListStringAllGather()
  {
    testGettableStringAllGather<std::list<std::string>>();
  }


  void testVectorStringAllGather()
  {
    testGettableStringAllGather<std::vector<std::string>>();
  }


  // Make sure we don't have problems with strings of length above 256
  // inside other containers either.  Also test mixing strings with
  // other types.
  void testTupleStringAllGather()
  {
    static_assert(Has_buffer_type<Packing<std::string>>::value);
    static_assert(libMesh::Parallel::TupleHasPacking<std::string>::value);
    static_assert(Has_buffer_type<Packing<std::tuple<std::string>>>::value);
    static_assert(libMesh::Parallel::TupleHasPacking<std::string, std::string>::value);
    static_assert(Has_buffer_type<Packing<std::tuple<std::string, std::string>>>::value);
    static_assert(libMesh::Parallel::TupleHasPacking<std::string, std::string, int>::value);
    static_assert(Has_buffer_type<Packing<std::tuple<std::string, std::string, int>>>::value);

    std::vector<std::tuple<std::string, std::string, int>> sendv(2);

    auto & s0 = std::get<1>(sendv[0]);
    std::get<0>(sendv[0]).assign("Hello");
    s0.assign("Is it me you're looking for?\n");
    for (int i=0; i != 6; ++i)
      s0 = s0+s0;
    timpi_assert_greater(s0.size(), 256);
    std::get<2>(sendv[0]) = 257;

    auto & s1 = std::get<1>(sendv[1]);
    std::get<0>(sendv[1]).assign("Goodbye");
    s1.assign("to you!  Guess it's better to say, goodbye\n");
    for (int i=0; i != 6; ++i)
      s1 = s1+s1;
    timpi_assert_greater(s1.size(), 256);
    std::get<2>(sendv[1]) = 258;

    std::vector<std::tuple<std::string, std::string, int>> send(1);
    if (TestCommWorld->rank() == 0)
      send[0] = sendv[0];
    else
      send[0] = sendv[1];

    std::vector<std::tuple<std::string, std::string, int>> recv;

    TestCommWorld->allgather_packed_range
      ((void *)(NULL), send.begin(), send.end(),
       std::back_inserter(recv));

    const std::size_t comm_size = TestCommWorld->size();
    const std::size_t vec_size = recv.size();
    TIMPI_UNIT_ASSERT(comm_size == vec_size);

    TIMPI_UNIT_ASSERT(sendv[0] == recv[0]);
    for (std::size_t i=1; i < vec_size; ++i)
      TIMPI_UNIT_ASSERT(sendv[1] == recv[i]);
  }



  // We should be able to nest containers in other containers in the
  // first containers etc.
  void testNestingAllGather()
  {
    typedef std::tuple<unsigned int, std::vector<std::tuple<char,int,std::size_t>>, unsigned int> send_type;

    static_assert(Has_buffer_type<Packing<std::vector<std::tuple<char,int,std::size_t>>>>::value);
    static_assert(Has_buffer_type<Packing<send_type>>::value);

    std::vector<send_type> sendv(2);

    std::get<0>(sendv[0]) = 100;
    std::get<1>(sendv[0]) = {{'a', -4, 1000},{'b', -5, 2000}};
    std::get<2>(sendv[0]) = 3000;

    std::get<0>(sendv[1]) = 200;
    std::get<1>(sendv[1]) = {{'c', -6, 4000},{'d', -7, 5000}};
    std::get<2>(sendv[1]) = 6000;

    std::vector<send_type> send(1);
    if (TestCommWorld->rank() == 0)
      send[0] = sendv[0];
    else
      send[0] = sendv[1];

    std::vector<send_type> recv;

    TestCommWorld->allgather_packed_range
      ((void *)(NULL), send.begin(), send.end(),
       std::back_inserter(recv));

    const std::size_t comm_size = TestCommWorld->size();
    const std::size_t vec_size = recv.size();
    TIMPI_UNIT_ASSERT(comm_size == vec_size);

    TIMPI_UNIT_ASSERT(sendv[0] == recv[0]);
    for (std::size_t i=1; i < vec_size; ++i)
      TIMPI_UNIT_ASSERT(sendv[1] == recv[i]);
  }



  void testNullSendReceive()
  {
    std::vector<std::string> send(1);
    const unsigned int my_rank = TestCommWorld->rank();
    const unsigned int dest_rank =
      (my_rank + 1) % TestCommWorld->size();
    const unsigned int source_rank =
      (my_rank + TestCommWorld->size() - 1) % TestCommWorld->size();

    {
      std::ostringstream os;
      os << my_rank;
      send[0] = os.str();
    }

    TestCommWorld->send_receive_packed_range
      (dest_rank, (void *)(NULL), send.begin(), send.end(),
       source_rank, (void *)(NULL),
       null_output_iterator<std::string>(),
       (std::string*)NULL);
  }

  void testContainerAllGather()
  {
    // This method uses a specialized allgather method that is only defined
    // when we have MPI
#ifdef TIMPI_HAVE_MPI
    std::vector<std::string> vals;
    const unsigned int my_rank = TestCommWorld->rank();
    TestCommWorld->allgather(std::string(my_rank+1, '0' + my_rank), vals);

    const std::size_t comm_size = TestCommWorld->size();
    const std::size_t vec_size = vals.size();
    TIMPI_UNIT_ASSERT(comm_size == vec_size);

    for (std::size_t i = 0; i < vec_size; ++i)
      TIMPI_UNIT_ASSERT(vals[i] == std::string(i + 1, '0' + i));
#endif
  }

  void testContainerSendReceive()
  {
    std::vector<processor_id_type> vals;

    std::vector<std::string> send(1), recv;

    const unsigned int my_rank = TestCommWorld->rank();
    const unsigned int dest_rank =
      (my_rank + 1) % TestCommWorld->size();
    const unsigned int source_rank =
      (my_rank + TestCommWorld->size() - 1) % TestCommWorld->size();

    {
      std::ostringstream os;
      os << my_rank;
      send[0] = os.str();
    }

    TestCommWorld->send_receive_packed_range
      (dest_rank, (void *)(NULL), send.begin(), send.end(),
       source_rank, (void *)(NULL),
       std::back_inserter(recv),
       (std::string*)NULL);

    TIMPI_UNIT_ASSERT(recv.size() == std::size_t(1));

    std::string check;
    {
      std::ostringstream os;
      os << source_rank;
      check = os.str();
    }

    TIMPI_UNIT_ASSERT(recv[0] == check);
  }

  void testPushPackedImpl(int M)
  {
    const int size = TestCommWorld->size(),
              rank = TestCommWorld->rank();

    std::map<processor_id_type, std::multiset<std::string>>
      data, received_data;

    auto stringy_number = [] (int number)
      {
        std::string digit_strings [10] = {"zero", "one", "two",
            "three", "four", "five", "six", "seven", "eight", "nine"};

        std::string returnval = "done";
        while (number)
          {
            returnval = digit_strings[number%10]+" "+returnval;
            number = number/10;
          };

        return returnval;
      };

    for (int d=0; d != M; ++d)
      {
        int diffsize = std::abs(d-rank);
        int diffsqrt = std::sqrt(diffsize);
        if (diffsqrt*diffsqrt == diffsize)
          for (int i=-1; i != diffsqrt; ++i)
            data[d].insert(stringy_number(d));
      }

    auto collect_data =
      [&received_data]
      (processor_id_type pid,
       const typename std::multiset<std::string> & multiset_received)
      {
        auto & received = received_data[pid];
        received.insert(multiset_received.begin(), multiset_received.end());
      };

    // Ensure that no const_cast perfidy in parallel_sync.h messes up
    // our original data
    std::map<processor_id_type, std::multiset<std::string>> preserved_data {data};

    // Do the push
    void * context = nullptr;
    TIMPI::push_parallel_packed_range(*TestCommWorld, data, context, collect_data);

    // Test the sent data, which shouldn't have changed
    TIMPI_UNIT_ASSERT(preserved_data.size() == data.size());
    for (const auto & pair: preserved_data)
      {
        const auto &pd_ms = pair.second;
        const auto &d_ms = data[pair.first];
        TIMPI_UNIT_ASSERT(pd_ms.size() == d_ms.size());
        for (auto entry : pd_ms)
          TIMPI_UNIT_ASSERT(pd_ms.count(entry) == d_ms.count(entry));
      }

    // Test the received results, for each processor id p we're in
    // charge of.
    std::vector<std::size_t> checked_sizes(size, 0);
    for (int p=rank; p < M; p += size)
      for (int srcp=0; srcp != size; ++srcp)
        {
          int diffsize = std::abs(srcp-p);
          int diffsqrt = std::sqrt(diffsize);
          if (diffsqrt*diffsqrt != diffsize)
            {
              if (received_data.count(srcp))
                {
                  const std::multiset<std::string> & datum = received_data[srcp];
                  TIMPI_UNIT_ASSERT
                    (std::count(datum.begin(), datum.end(),
                                stringy_number(p)) == std::ptrdiff_t(0));
                }
              continue;
            }

          TIMPI_UNIT_ASSERT(received_data.count(srcp) == std::size_t(1));
          const std::multiset<std::string> & datum = received_data[srcp];
          TIMPI_UNIT_ASSERT
            (std::count(datum.begin(), datum.end(), stringy_number(p)) ==
             std::ptrdiff_t(diffsqrt+1));
          checked_sizes[srcp] += diffsqrt+1;
        }

    for (int srcp=0; srcp != size; ++srcp)
      TIMPI_UNIT_ASSERT(checked_sizes[srcp] == received_data[srcp].size());

  }

  void testPushPacked()
  {
    testPushPackedImpl(TestCommWorld->size());
  }

  void testPushPackedOversized()
  {
    testPushPackedImpl((TestCommWorld->size() + 4) * 2);
  }

  template <typename FillFunctor, typename PushFunctor>
  void testPushPackedNestedImpl(FillFunctor fill_functor, PushFunctor push_functor)
  {
    const int size = TestCommWorld->size(),
              rank = TestCommWorld->rank();

    typedef decltype(fill_functor(0)) fill_type;

    typedef std::vector<fill_type> vec_type;
    std::map<processor_id_type, vec_type> data, received_data;

    for (int d=0; d != size; ++d)
      {
        int diffsize = std::abs(d-rank);
        int diffsqrt = std::sqrt(diffsize);
        if (diffsqrt*diffsqrt == diffsize)
          for (int i=-1; i != diffsqrt; ++i)
            data[d].push_back(fill_functor(d));
      }

    auto collect_data =
      [&received_data]
      (processor_id_type pid,
       const vec_type & vec_received)
      {
        auto & received = received_data[pid];
        received = vec_received;
      };

    // Ensure that no const_cast perfidy in parallel_sync.h messes up
    // our original data
    std::map<processor_id_type, vec_type> preserved_data {data};

    // Do the push
    push_functor(data, collect_data);

    // Test the sent data, which shouldn't have changed
    TIMPI_UNIT_ASSERT(preserved_data == data);

    // Test the received results, for each processor id p we're in
    // charge of.
    std::vector<std::size_t> checked_sizes(size, 0);
    for (int srcp=0; srcp != size; ++srcp)
      {
        int diffsize = std::abs(srcp-rank);
        int diffsqrt = std::sqrt(diffsize);
        if (diffsqrt*diffsqrt != diffsize)
          {
            TIMPI_UNIT_ASSERT(!received_data.count(srcp));
            continue;
          }

        TIMPI_UNIT_ASSERT(received_data.count(srcp));
        const auto & rec = received_data[srcp];
        TIMPI_UNIT_ASSERT(rec.size() == std::size_t(diffsqrt+1));

        for (auto & tup : rec)
          TIMPI_UNIT_ASSERT(tup == fill_functor(rank));
      }
  }


  void testPushPackedNested()
  {
    // Do the push explicitly with a packed_range function
    auto explicitly_packed_push = [](auto & data, auto & collect_data) {
      void * context = nullptr;
      TIMPI::push_parallel_packed_range(*TestCommWorld, data, context, collect_data);
    };

    typedef std::tuple<unsigned int, std::vector<char>, unsigned int,
            unsigned int, unsigned int, unsigned int> tuple_type;
    auto fill_tuple= [] (int n) {
      return tuple_type(n, fake_stringy_number(n), n, n, n, n);
    };

    testPushPackedNestedImpl(fill_tuple, explicitly_packed_push);
  }


  void testPushPackedDispatch()
  {
    // Do the push implicitly with an auto-dispatching function
    auto implicitly_packed_push = [](auto & data, auto & collect_data) {
      TIMPI::push_parallel_vector_data(*TestCommWorld, data, collect_data);
    };

    typedef std::tuple<unsigned int, std::vector<char>, unsigned int,
            unsigned int, unsigned int, unsigned int> tuple_type;
    auto fill_tuple= [] (int n) {
      return tuple_type(n, fake_stringy_number(n), n, n, n, n);
    };

    testPushPackedNestedImpl(fill_tuple, implicitly_packed_push);
  }


  void testPushPackedOneTuple()
  {
    typedef std::tuple<std::unordered_map<unsigned int, std::string>> tuple_type;
    static_assert(Has_buffer_type<Packing<std::string>>::value);
    static_assert(TIMPI::StandardType<std::pair<unsigned int, unsigned int>>::is_fixed_type);
    static_assert(TIMPI::StandardType<std::pair<const unsigned int, unsigned int>>::is_fixed_type);
    static_assert(Has_buffer_type<Packing<std::unordered_map<unsigned int, unsigned int>>>::value);
    static_assert(Has_buffer_type<Packing<std::pair<std::string, std::string>>>::value);
    static_assert(Has_buffer_type<Packing<std::pair<const std::string, std::string>>>::value);
    static_assert(Has_buffer_type<Packing<std::unordered_map<std::string, std::string>>>::value);
    static_assert(Has_buffer_type<Packing<std::pair<unsigned int, std::string>>>::value);
    static_assert(Has_buffer_type<Packing<std::pair<const unsigned int, std::string>>>::value);
    static_assert(Has_buffer_type<Packing<std::unordered_map<unsigned int, std::string>>>::value);
    static_assert(Has_buffer_type<Packing<tuple_type>>::value);

    auto fill_tuple = [] (int n)
      {
        tuple_type returnval;
        (std::get<0>(returnval))[n] = stringy_number(n);
        (std::get<0>(returnval))[n+1] = stringy_number(n+1);
        return returnval;
      };

    auto implicitly_packed_push = [](auto & data, auto & collect_data) {
      TIMPI::push_parallel_vector_data(*TestCommWorld, data, collect_data);
    };

    testPushPackedNestedImpl(fill_tuple, implicitly_packed_push);
  }


// A failure case I caught downstream
  void testPushPackedFailureCase()
  {
    typedef std::tuple<std::size_t, int, std::unordered_map<unsigned int, std::string>> tuple_type;

    auto fill_tuple = [] (int n)
      {
        tuple_type returnval;
        std::get<0>(returnval) = n;
        std::get<1>(returnval) = n;
        (std::get<2>(returnval))[n] = stringy_number(n);
        (std::get<2>(returnval))[n+1] = stringy_number(n+1);
        return returnval;
      };

    auto implicitly_packed_push = [](auto & data, auto & collect_data) {
      TIMPI::push_parallel_vector_data(*TestCommWorld, data, collect_data);
    };

    testPushPackedNestedImpl(fill_tuple, implicitly_packed_push);
  }


#if __cplusplus > 201402L
  void testPushPackedImplMove(int M)
  {
    const int size = TestCommWorld->size(),
              rank = TestCommWorld->rank();

    std::map<processor_id_type, std::vector<std::unique_ptr<int>>>
      data, received_data;

    for (int d=0; d != M; ++d)
      {
        int diffsize = std::abs(d-rank);
        int diffsqrt = std::sqrt(diffsize);
        if (diffsqrt*diffsqrt == diffsize)
          for (int i=-1; i != diffsqrt; ++i)
            data[d].emplace_back(std::make_unique<int>(d));
      }

    auto collect_data =
      [&received_data]
      (processor_id_type pid,
       std::vector<std::unique_ptr<int>> && vector_received)
      {
        auto & received = received_data[pid];
        for (auto & val : vector_received)
          received.emplace_back(std::move(val));
      };

    void * context = nullptr;
    TIMPI::push_parallel_packed_range(*TestCommWorld, std::move(data), context, collect_data);

    // Test the received results, for each processor id p we're in
    // charge of.
    std::vector<std::size_t> checked_sizes(size, 0);
    for (int p=rank; p < M; p += size)
      for (int srcp=0; srcp != size; ++srcp)
        {
          int diffsize = std::abs(srcp-p);
          int diffsqrt = std::sqrt(diffsize);
          if (diffsqrt*diffsqrt != diffsize)
            {
              if (received_data.count(srcp))
                {
                  std::size_t count = 0;
                  for (const auto & val : received_data[srcp])
                    if (*val == p)
                      ++count;

                  TIMPI_UNIT_ASSERT(count == (std::size_t)std::ptrdiff_t(0));
                }
              continue;
            }

          TIMPI_UNIT_ASSERT(received_data.count(srcp) == std::size_t(1));

          std::size_t count = 0;
          for (const auto & val : received_data[srcp])
            if (*val == p)
              ++count;

          TIMPI_UNIT_ASSERT(count == (std::size_t)std::ptrdiff_t(diffsqrt+1));
          checked_sizes[srcp] += diffsqrt+1;
        }

    for (int srcp=0; srcp != size; ++srcp)
      TIMPI_UNIT_ASSERT(checked_sizes[srcp] == received_data[srcp].size());
  }

  void testPushPackedMove()
  {
    testPushPackedImpl(TestCommWorld->size());
  }

  void testPushPackedMoveOversized()
  {
    testPushPackedImpl((TestCommWorld->size() + 4) * 2);
  }
#endif

int main(int argc, const char * const * argv)
{
  TIMPI::TIMPIInit init(argc, argv);
  TestCommWorld = &init.comm();

  testNullAllGather();
  testPairStringAllGather();
  testArrayStringAllGather();
  testTupleStringAllGather();
  testNestingAllGather();
  testNullSendReceive();
  testContainerAllGather();
  testContainerSendReceive();
  testPushPacked();
  testPushPackedOversized();
  testPushPackedNested();
  testPushPackedDispatch();
  testPushPackedOneTuple();
  testPushPackedFailureCase();
#if __cplusplus > 201402L
  testPushPackedMove();
  testPushPackedMoveOversized();
#endif

  return 0;
}
