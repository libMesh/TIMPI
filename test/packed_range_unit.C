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
  void testPairStringAllGather()
  {
    std::vector<std::pair<std::string, std::string>> sendv(2);

    sendv[0].first.assign("Hello");
    auto & s0 = sendv[0].second;
    s0.assign("Is it me you're looking for?\n");
    for (int i=0; i != 6; ++i)
      s0 = s0+s0;
    timpi_assert_greater(s0.size(), 256);

    sendv[1].first.assign("Goodbye");
    auto & s1 = sendv[1].second;
    s1.assign("to you!  Guess it's better to say, goodbye\n");
    for (int i=0; i != 6; ++i)
      s1 = s1+s1;
    timpi_assert_greater(s1.size(), 256);

    std::vector<std::pair<std::string, std::string>> send(1);
    if (TestCommWorld->rank() == 0)
      send[0] = sendv[0];
    else
      send[0] = sendv[1];

    std::vector<std::pair<std::string, std::string>> recv;

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


  // Make sure we don't have problems with strings of length above 256
  // inside other containers either
  void testTupleStringAllGather()
  {
    std::vector<std::tuple<std::string, std::string, std::string>> sendv(2);

    auto & s0 = std::get<1>(sendv[0]);
    std::get<0>(sendv[0]).assign("Hello");
    s0.assign("Is it me you're looking for?\n");
    for (int i=0; i != 6; ++i)
      s0 = s0+s0;
    timpi_assert_greater(s0.size(), 256);
    std::get<2>(sendv[0]).assign("I can see it in your eyes.\n");

    auto & s1 = std::get<1>(sendv[1]);
    std::get<0>(sendv[1]).assign("Goodbye");
    s1.assign("to you!  Guess it's better to say, goodbye\n");
    for (int i=0; i != 6; ++i)
      s1 = s1+s1;
    timpi_assert_greater(s1.size(), 256);
    std::get<2>(sendv[1]).assign("'Cause baby it's over now.\n");

    std::vector<std::tuple<std::string, std::string, std::string>> send(1);
    if (TestCommWorld->rank() == 0)
      send[0] = sendv[0];
    else
      send[0] = sendv[1];

    std::vector<std::tuple<std::string, std::string, std::string>> recv;

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
       std::vector<std::unique_ptr<int>> & vector_received)
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
  testTupleStringAllGather();
  testNullSendReceive();
  testContainerAllGather();
  testContainerSendReceive();
  testPushPacked();
  testPushPackedOversized();
#if __cplusplus > 201402L
  testPushPackedMove();
  testPushPackedMoveOversized();
#endif

  return 0;
}
