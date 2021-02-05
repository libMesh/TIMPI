#include <timpi/parallel_sync.h>
#include <timpi/timpi.h>

#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

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

using namespace TIMPI;

Communicator *TestCommWorld;

  void testNullAllGather()
  {
    std::vector<processor_id_type> vals;

    std::vector<std::string> send(1);
    if (TestCommWorld->rank() == 0)
      send[0].assign("Hello");
    else
      send[0].assign("Goodbye");

    TestCommWorld->allgather_packed_range
      ((void *)(NULL), send.begin(), send.end(),
       null_output_iterator<std::string>());
  }


  void testNullSendReceive()
  {
    std::vector<processor_id_type> vals;

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

    void * context = nullptr;
    TIMPI::push_parallel_packed_range(*TestCommWorld, data, context, collect_data);

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

int main(int argc, const char * const * argv)
{
  TIMPI::TIMPIInit init(argc, argv);
  TestCommWorld = &init.comm();

  testNullAllGather();
  testNullSendReceive();
  testContainerAllGather();
  testContainerSendReceive();
  testPushPacked();
  testPushPackedOversized();

  return 0;
}
