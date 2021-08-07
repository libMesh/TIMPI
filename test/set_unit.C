#include <timpi/timpi.h>

#include <set>
#include <unordered_set>

#define TIMPI_UNIT_ASSERT(expr) \
  if (!(expr)) \
    timpi_error();

using namespace TIMPI;

Communicator *TestCommWorld;

void my_inserter(std::set<int> & s, int i)
{ s.insert(i); }

void my_inserter(std::set<std::vector<int>> & s, int i)
{ s.insert(std::vector<int>(i,i)); }

void my_inserter(std::unordered_set<int> & s, int i)
{ s.insert(i); }

void my_inserter(std::map<int, int> & m, int i)
{ m.insert(std::make_pair(i,2*i+3)); }

void my_inserter(std::map<int, std::vector<int>> & m, int i)
{ m.insert(std::make_pair(i,std::vector<int>(i,2*i+3))); }

void my_inserter(std::unordered_map<int, int> & m, int i)
{ m.insert(std::make_pair(i,2*i+3)); }

void my_inserter(std::unordered_map<int, std::vector<int>> & m, int i)
{ m.insert(std::make_pair(i,std::vector<int>(i,2*i+3))); }


void tester(const std::set<int> & s, int i)
{ TIMPI_UNIT_ASSERT( s.count(i) == std::size_t(1) ); }

void tester(const std::set<std::vector<int>> & s, int i)
{ TIMPI_UNIT_ASSERT( s.count(std::vector<int>(i,i)) == std::size_t(1) ); }

void tester(const std::unordered_set<int> & s, int i)
{ TIMPI_UNIT_ASSERT( s.count(i) == std::size_t(1) ); }

void tester(const std::map<int, int> & m, int i)
{
  TIMPI_UNIT_ASSERT( m.count(i) == std::size_t(1) );
  TIMPI_UNIT_ASSERT( m.at(i) == 2*i+3 );
}

void tester(const std::map<int, std::vector<int>> & m, int i)
{
  TIMPI_UNIT_ASSERT( m.count(i) == std::size_t(1) );
  TIMPI_UNIT_ASSERT( m.at(i).size() == std::size_t(i) );
  for (auto val : m.at(i))
    TIMPI_UNIT_ASSERT( val == 2*i+3 );
}

void tester(const std::unordered_map<int, int> & m, int i)
{
  TIMPI_UNIT_ASSERT( m.count(i) == std::size_t(1) );
  TIMPI_UNIT_ASSERT( m.at(i) == 2*i+3 );
}

void tester(const std::unordered_map<int, std::vector<int>> & m, int i)
{
  TIMPI_UNIT_ASSERT( m.count(i) == std::size_t(1) );
  TIMPI_UNIT_ASSERT( m.at(i).size() == std::size_t(i) );
  for (auto val : m.at(i))
    TIMPI_UNIT_ASSERT( val == 2*i+3 );
}



  template <class Set>
  void testUnion()
  {
    Set data;

    const int N = TestCommWorld->size();

    my_inserter(data, TestCommWorld->rank());
    my_inserter(data, 2*N);
    my_inserter(data, 3*N + TestCommWorld->rank());

    TestCommWorld->set_union(data);

    TIMPI_UNIT_ASSERT( data.size() == std::size_t(2*N+1) );
    tester(data, 2*N);
    for (int p=0; p<N; ++p)
      {
        tester(data, p);
        tester(data, 3*N+p);
      }
  }


int main(int argc, const char * const * argv)
{
  TIMPI::TIMPIInit init(argc, argv);
  TestCommWorld = &init.comm();

  testUnion<std::set<int>>();
  testUnion<std::unordered_set<int>>();
  testUnion<std::map<int, int>>();
  testUnion<std::unordered_map<int, int>>();

  // TODO: allgather(vector<vector>)
  // testUnion<std::set<std::vector<int>>>();

  // No std::hash<vector<non-bool>>
  // testUnion<std::unordered_set<std::vector<int>>>();

  testUnion<std::map<int, std::vector<int>>>();
  testUnion<std::unordered_map<int, std::vector<int>>>();

  return 0;
}
