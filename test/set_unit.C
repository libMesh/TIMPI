#include <timpi/timpi.h>

#include <set>
#include <unordered_set>

#define TIMPI_UNIT_ASSERT(expr) \
  if (!(expr)) \
    timpi_error();

using namespace TIMPI;

Communicator *TestCommWorld;


  template <class Set>
  void testUnion()
  {
    Set data;

    const int N = TestCommWorld->size();

    data.insert(TestCommWorld->rank());
    data.insert(2*N);
    data.insert(3*N + TestCommWorld->rank());

    TestCommWorld->set_union(data);

    TIMPI_UNIT_ASSERT( data.size() == std::size_t(2*N+1) );
    TIMPI_UNIT_ASSERT( data.count(2*N) == std::size_t(1) );
    for (int p=0; p<N; ++p)
      {
        TIMPI_UNIT_ASSERT( data.count(p) == std::size_t(1) );
        TIMPI_UNIT_ASSERT( data.count(3*N+p) == std::size_t(1) );
      }
  }


int main(int argc, const char * const * argv)
{
  TIMPI::TIMPIInit init(argc, argv);
  TestCommWorld = &init.comm();

  testUnion<std::set<int>>();

  // TODO
  // testUnion<std::unordered_set<int>>();

  return 0;
}
