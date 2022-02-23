#include <timpi/timpi.h>
// timpi.h doesn't pull in parallel_sync
#include <timpi/parallel_sync.h>

#include <algorithm>
#include <array>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility> // pair
#include <vector>
#include <unistd.h>

#define TIMPI_UNIT_ASSERT(expr) \
  if (!(expr)) \
    timpi_error()

namespace TIMPI
{
template <typename T>
class StandardType<std::set<T>> : public NotADataType
{
public:
  StandardType(const std::set<T> *)
    {
    }
};
}

using namespace TIMPI;

Communicator *TestCommWorld;


template <typename Container>
Container createContainer(std::size_t size)
{
  std::vector<typename Container::value_type> temp(size);
  std::iota(temp.begin(), temp.end(), 0);
  return Container(temp.begin(), temp.end());
}


template <typename T>
std::set<T> createSet(std::size_t size)
{ return createContainer<std::set<T>>(size); }


template <typename Container>
Container createMapContainer(std::size_t size)
{
  Container c;

  for (std::size_t i = 0; i != size; ++i)
    c.insert(std::make_pair(i*10, i*50));

  return c;
}



  template <typename Container>
  void testContainerAllGather()
  {
    std::vector<Container> vals;
    const unsigned int my_rank = TestCommWorld->rank();

    auto my_val = createContainer<Container>(my_rank + 1);

    TestCommWorld->allgather(my_val, vals);

    const std::size_t comm_size = TestCommWorld->size();
    const std::size_t vec_size = vals.size();
    TIMPI_UNIT_ASSERT(comm_size == vec_size);

    for (std::size_t i = 0; i < vec_size; ++i)
    {
      const auto & current_container = vals[i];
      TIMPI_UNIT_ASSERT(current_container.size() == i+1);
      for (std::size_t n = 0; n != i+1; ++n)
        {
          auto it = std::find(current_container.begin(),
                              current_container.end(), n);
          TIMPI_UNIT_ASSERT(it != current_container.end());
        }
    }
  }

  template <typename Container>
  void testMapContainerAllGather()
  {
    std::vector<Container> vals;
    const unsigned int my_rank = TestCommWorld->rank();

    auto my_val = createMapContainer<Container>(my_rank + 1);

    TestCommWorld->allgather(my_val, vals);

    const std::size_t comm_size = TestCommWorld->size();
    const std::size_t vec_size = vals.size();
    TIMPI_UNIT_ASSERT(comm_size == vec_size);

    for (std::size_t i = 0; i < vec_size; ++i)
    {
      const auto & current_container = vals[i];
      TIMPI_UNIT_ASSERT(current_container.size() == i+1);
      for (std::size_t n = 0; n != i+1; ++n)
        {
          auto it = current_container.find(n*10);
          TIMPI_UNIT_ASSERT(it != current_container.end());
          TIMPI_UNIT_ASSERT(it->second == n*50);
        }
    }
  }

  void testPackedSetUnion()
  {
    std::set<std::vector<std::tuple<int,int>>> data;
    const int N = TestCommWorld->size();

    auto set_inserter = [&data](int i)
    {
      std::vector<std::tuple<int,int>> datum(1);
      std::get<0>(datum[0]) = i;
      std::get<1>(datum[0]) = 2*i;
      data.insert(datum);
    };

    auto set_tester = [&data](int i)
    {
      std::vector<std::tuple<int,int>> datum(1);
      std::get<0>(datum[0]) = i;
      std::get<1>(datum[0]) = 2*i;
      TIMPI_UNIT_ASSERT(data.count(datum) == std::size_t(1));
    };

    set_inserter(TestCommWorld->rank());
    set_inserter(2*N);
    set_inserter(3*N + TestCommWorld->rank());

    TestCommWorld->set_union(data);

    TIMPI_UNIT_ASSERT( data.size() == std::size_t(2*N+1) );
    set_tester(2*N);
    for (int p=0; p<N; ++p)
      {
        set_tester(p);
        set_tester(3*N+p);
      }
  }

  void testVectorOfContainersAllGather()
  {
    std::vector<std::set<unsigned int>> vals(1);
    const unsigned int my_rank = TestCommWorld->rank();
    vals[0] = createSet<unsigned int>(my_rank + 1);

    TestCommWorld->allgather(vals);

    const std::size_t comm_size = TestCommWorld->size();
    const std::size_t vec_size = vals.size();
    TIMPI_UNIT_ASSERT(comm_size == vec_size);

    for (std::size_t i = 0; i < vec_size; ++i)
    {
      const auto & current_set = vals[i];
      TIMPI_UNIT_ASSERT(current_set.size() == i+1);
      unsigned int value = 0;
      for (auto number : current_set)
        TIMPI_UNIT_ASSERT(number == value++);
    }
  }

  void testArrayContainerAllGather()
  {
    std::vector<std::array<std::set<unsigned int>, 2>> vals;
    const unsigned int my_rank = TestCommWorld->rank();

    std::array<std::set<unsigned int>, 2> vals_out
      {{createSet<unsigned int>(my_rank + 1),
        createSet<unsigned int>(my_rank + 10)}};

    TestCommWorld->allgather(vals_out, vals);

    const std::size_t comm_size = TestCommWorld->size();
    const std::size_t vec_size = vals.size();
    TIMPI_UNIT_ASSERT(comm_size == vec_size);

    for (std::size_t i = 0; i < vec_size; ++i)
    {
      const auto & first_set = vals[i][0];
      TIMPI_UNIT_ASSERT(first_set.size() == i+1);
      unsigned int value = 0;
      for (auto number : first_set)
        TIMPI_UNIT_ASSERT(number == value++);
      const auto & second_set = vals[i][1];
      TIMPI_UNIT_ASSERT(second_set.size() == i+10);
      value = 0;
      for (auto number : second_set)
        TIMPI_UNIT_ASSERT(number == value++);
    }
  }

  void testTupleContainerAllGather()
  {
    std::vector<std::tuple<std::set<unsigned int>, unsigned int, unsigned int>> vals;
    const unsigned int my_rank = TestCommWorld->rank();

    TestCommWorld->allgather(std::make_tuple(
                               createSet<unsigned int>(my_rank + 1),
                               my_rank, 2*my_rank), vals);

    const std::size_t comm_size = TestCommWorld->size();
    const std::size_t vec_size = vals.size();
    TIMPI_UNIT_ASSERT(comm_size == vec_size);

    for (std::size_t i = 0; i < vec_size; ++i)
    {
      const auto & current_set = std::get<0>(vals[i]);
      unsigned int value = 0;
      for (auto number : current_set)
        TIMPI_UNIT_ASSERT(number == value++);
      TIMPI_UNIT_ASSERT(std::get<1>(vals[i]) == i);
      TIMPI_UNIT_ASSERT(std::get<2>(vals[i]) == 2*i);
    }
  }

  void testPairContainerAllGather()
  {
    std::vector<std::pair<std::set<unsigned int>, unsigned int>> vals;
    const unsigned int my_rank = TestCommWorld->rank();

    TestCommWorld->allgather(std::make_pair(
                               createSet<unsigned int>(my_rank + 1),
                               my_rank), vals);

    const std::size_t comm_size = TestCommWorld->size();
    const std::size_t vec_size = vals.size();
    TIMPI_UNIT_ASSERT(comm_size == vec_size);

    for (std::size_t i = 0; i < vec_size; ++i)
    {
      const auto & current_set = vals[i].first;
      unsigned int value = 0;
      for (auto number : current_set)
        TIMPI_UNIT_ASSERT(number == value++);
      TIMPI_UNIT_ASSERT(vals[i].second == i);
    }
  }

  void testContainerBroadcast()
  {
    std::set<unsigned int> val;
    const unsigned int my_rank = TestCommWorld->rank();

    if (my_rank == 0)
      val.insert(0);

    TestCommWorld->broadcast(val);

    TIMPI_UNIT_ASSERT(val.size() == 1);
    TIMPI_UNIT_ASSERT(*val.begin() == 0);
  }

void testVectorOfContainersBroadcast()
  {
    std::vector<std::set<unsigned int>> vals;
    const unsigned int my_rank = TestCommWorld->rank();
    const std::size_t comm_size = TestCommWorld->size();

    if (my_rank == 0)
    {
      vals.resize(comm_size + 1);
      unsigned int counter = 1;
      for (auto & val : vals)
      {
        for (unsigned int number = 0; number < counter; ++number)
          val.insert(number);
        ++counter;
      }
    }
    TestCommWorld->broadcast(vals);

    const std::size_t vec_size = vals.size();
    TIMPI_UNIT_ASSERT((comm_size + 1) == vec_size);

    std::size_t counter = 1;
    for (const auto & current_set : vals)
    {
      TIMPI_UNIT_ASSERT(current_set.size() == counter);
      unsigned int number = 0;
      for (auto elem : current_set)
        TIMPI_UNIT_ASSERT(elem == number++);
      ++counter;
    }
  }

  // Data to send/recieve with each processor rank.  For this test, processor p
  // will send to destination d a set with d+1 elements numbered from 0 to d, in
  // a vector with sqrt(c)+1 copies, iff c := |p-d| is a square number.
  void fill_data
  (std::map<processor_id_type, std::vector<std::set<unsigned int>>> & data,
     int M)
  {
    const int rank = TestCommWorld->rank();
    for (int d=0; d != M; ++d)
      {
        const int diffsize = std::abs(d-rank);
        const int diffsqrt = std::sqrt(diffsize);
        if (diffsqrt*diffsqrt == diffsize)
          for (int i=-1; i != diffsqrt; ++i)
            data[d].push_back(createSet<unsigned int>(d+1));
      }
  }

  void testPush()
  {
    const int size = TestCommWorld->size(),
              rank = TestCommWorld->rank();

    std::map<processor_id_type, std::vector<std::set<unsigned int>>> data, received_data;

    fill_data(data, size);

    auto collect_data =
      [&received_data]
      (processor_id_type pid,
       const typename std::vector<std::set<unsigned int>> & vecset_received)
      {
        auto & vec = received_data[pid];
        vec.insert(vec.end(), vecset_received.begin(), vecset_received.end());
      };

    push_parallel_vector_data(*TestCommWorld, data, collect_data);

    // We only need to check ourselves to see what we were sent
    int p = rank;
    for (int srcp=0; srcp != size; ++srcp)
    {
      auto map_it = received_data.find(srcp);

      const int diffsize = std::abs(srcp-p);
      const int diffsqrt = std::sqrt(diffsize);
      if (diffsqrt*diffsqrt != diffsize)
      {
        // We shouldn't have been sent anything from srcp!
        TIMPI_UNIT_ASSERT(map_it == received_data.end() || map_it->second.empty());
        continue;
      }

      TIMPI_UNIT_ASSERT(map_it != received_data.end());
      const std::vector<std::set<unsigned int>> & datum = map_it->second;
      TIMPI_UNIT_ASSERT(datum.size() == static_cast<std::size_t>(diffsqrt+1));

      for (const auto & set : datum)
      {
        TIMPI_UNIT_ASSERT(set.size() == static_cast<std::size_t>((p+1)));

        unsigned int comparator = 0;
        for (const auto element : set)
          TIMPI_UNIT_ASSERT(element == comparator++);
      }
    }
  }

  void testPull()
  {
    const int size = TestCommWorld->size();

    std::map<processor_id_type, std::vector<std::set<unsigned int>> > data, received_data;

    fill_data(data, size);

    auto compose_replies =
      []
      (processor_id_type /* pid */,
       const std::vector<std::set<unsigned int>> & query,
       std::vector<std::set<unsigned int>> & response)
      {
        const std::size_t query_size = query.size();
        response.resize(query_size);
        for (unsigned int i=0; i != query_size; ++i)
        {
          const auto & query_set = query[i];
          for (const unsigned int elem : query_set)
            response[i].insert(elem*elem);
        }
      };


    auto collect_replies =
      [&received_data]
      (processor_id_type pid,
       const std::vector<std::set<unsigned int>> & query,
       const std::vector<std::set<unsigned int>> & response)
      {
        const std::size_t query_size = query.size();
        TIMPI_UNIT_ASSERT(query_size == response.size());
        for (unsigned int i=0; i != query_size; ++i)
          {
            TIMPI_UNIT_ASSERT(query[i].size() == response[i].size());

            auto query_set_it = query[i].begin(), response_set_it = response[i].begin();

            for (; query_set_it != query[i].end(); ++query_set_it, ++response_set_it)
            {
              const auto query_elem = *query_set_it, response_elem = *response_set_it;
              TIMPI_UNIT_ASSERT(query_elem * query_elem == response_elem);
            }
          }
        received_data[pid] = response;
      };

    // Do the pull
    std::set<unsigned int> * ex = nullptr;
    TIMPI::pull_parallel_vector_data
      (*TestCommWorld, data, compose_replies, collect_replies, ex);

    // Test the received results, for each query we sent.
    for (int p=0; p != size; ++p)
      {
        TIMPI_UNIT_ASSERT(data[p].size() == received_data[p].size());
        for (std::size_t i = 0; i != data[p].size(); ++i)
        {
          TIMPI_UNIT_ASSERT(data[p][i].size() == received_data[p][i].size());

          auto data_set_it = data[p][i].begin(), received_set_it = received_data[p][i].begin();

          for (; data_set_it != data[p][i].end(); ++data_set_it, ++received_set_it)
          {
            const auto data_elem = *data_set_it, received_elem = *received_set_it;
            TIMPI_UNIT_ASSERT(data_elem * data_elem == received_elem);
          }
        }
      }
  }

int main(int argc, const char * const * argv)
{
  TIMPI::TIMPIInit init(argc, argv);
  TestCommWorld = &init.comm();

  testContainerAllGather<std::list<unsigned int>>();
  testContainerAllGather<std::set<unsigned int>>();
  testContainerAllGather<std::unordered_set<unsigned int>>();
  testContainerAllGather<std::multiset<unsigned int>>();
  testContainerAllGather<std::unordered_multiset<unsigned int>>();
  testMapContainerAllGather<std::map<unsigned int, unsigned int>>();
  testMapContainerAllGather<std::unordered_map<unsigned int, unsigned int>>();
  testMapContainerAllGather<std::multimap<unsigned int, unsigned int>>();
  testMapContainerAllGather<std::unordered_multimap<unsigned int, unsigned int>>();
  testPackedSetUnion();
  testVectorOfContainersAllGather();
  testContainerBroadcast();
  testVectorOfContainersBroadcast();
  testPairContainerAllGather();
  testTupleContainerAllGather();
  testArrayContainerAllGather();

  testPush();
  testPull();

  return 0;
}
