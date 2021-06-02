#include <timpi/timpi.h>

#include <set>
#include <unordered_set>

#define TIMPI_UNIT_ASSERT(expr) \
  if (!(expr)) \
    timpi_error();

using namespace TIMPI;

Communicator *TestCommWorld;

void inserter(std::set<int> & s, int i)
{ s.insert(i); }

void inserter(std::unordered_set<int> & s, int i)
{ s.insert(i); }

void inserter(std::map<int, int> & m, int i)
{ m.insert(std::make_pair(i,2*i+3)); }

void inserter(std::unordered_map<int, int> & m, int i)
{ m.insert(std::make_pair(i,2*i+3)); }


void tester(const std::set<int> & s, int i)
{ TIMPI_UNIT_ASSERT( s.count(i) == std::size_t(1) ); }

void tester(const std::unordered_set<int> & s, int i)
{ TIMPI_UNIT_ASSERT( s.count(i) == std::size_t(1) ); }

void tester(const std::map<int, int> & m, int i)
{
  TIMPI_UNIT_ASSERT( m.count(i) == std::size_t(1) );
  TIMPI_UNIT_ASSERT( m.at(i) == 2*i+3 );
}

void tester(const std::unordered_map<int, int> & m, int i)
{
  TIMPI_UNIT_ASSERT( m.count(i) == std::size_t(1) );
  TIMPI_UNIT_ASSERT( m.at(i) == 2*i+3 );
}


  template <class Set>
  void testUnion()
  {
    Set data;

    const int N = TestCommWorld->size();

    inserter(data, TestCommWorld->rank());
    inserter(data, 2*N);
    inserter(data, 3*N + TestCommWorld->rank());

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

  return 0;
}
