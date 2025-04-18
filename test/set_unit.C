#include <timpi/timpi.h>

#include <bitset>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>

#define TIMPI_UNIT_ASSERT(expr) \
  if (!(expr)) \
    timpi_error();

using namespace TIMPI;

Communicator *TestCommWorld;

void my_inserter(std::set<int> & s, int i)
{ s.insert(i); }

void my_inserter(std::multiset<int> & s, int i)
{
  s.insert(-1);
  s.insert(3*i+1);
  s.insert(3*i+2);
}

void my_inserter(std::unordered_multiset<int> & s, int i)
{
  s.insert(-1);
  s.insert(3*i+1);
  s.insert(3*i+2);
}

void my_inserter(std::set<std::vector<int>> & s, int i)
{ s.insert(std::vector<int>(i,i)); }

void my_inserter(std::unordered_set<int> & s, int i)
{ s.insert(i); }

void my_inserter(std::map<int, int> & m, int i)
{ m.insert(std::make_pair(i,2*i+3)); }

void my_inserter(std::multimap<int, int> & m, int i)
{
  m.insert(std::make_pair(-1,-1));
  m.insert(std::make_pair(i,3*i+1));
  m.insert(std::make_pair(i,3*i+2));
}

void my_inserter(std::unordered_multimap<int, int> & m, int i)
{
  m.insert(std::make_pair(-1,-1));
  m.insert(std::make_pair(i,3*i+1));
  m.insert(std::make_pair(i,3*i+2));
}

void my_inserter(std::map<int, std::vector<int>> & m, int i)
{ m.insert(std::make_pair(i,std::vector<int>(i,2*i+3))); }

void my_inserter(std::unordered_map<int, int> & m, int i)
{ m.insert(std::make_pair(i,2*i+3)); }

void my_inserter(std::unordered_map<int, std::vector<int>> & m, int i)
{ m.insert(std::make_pair(i,std::vector<int>(i,2*i+3))); }


void tester(const std::set<int> & s, int i)
{ TIMPI_UNIT_ASSERT( s.count(i) == std::size_t(1) ); }

void tester(const std::multiset<int> & s, int i)
{
  TIMPI_UNIT_ASSERT( s.count(-1) * 3 == s.size() );
  TIMPI_UNIT_ASSERT( s.count(3*i+1) == std::size_t(1) );
  TIMPI_UNIT_ASSERT( s.count(3*i+2) == std::size_t(1) );
}

void tester(const std::unordered_multiset<int> & s, int i)
{
  TIMPI_UNIT_ASSERT( s.count(-1) * 3 == s.size() );
  TIMPI_UNIT_ASSERT( s.count(3*i+1) == std::size_t(1) );
  TIMPI_UNIT_ASSERT( s.count(3*i+2) == std::size_t(1) );
}

void tester(const std::set<std::vector<int>> & s, int i)
{ TIMPI_UNIT_ASSERT( s.count(std::vector<int>(i,i)) == std::size_t(1) ); }

void tester(const std::unordered_set<int> & s, int i)
{ TIMPI_UNIT_ASSERT( s.count(i) == std::size_t(1) ); }

void tester(const std::map<int, int> & m, int i)
{
  TIMPI_UNIT_ASSERT( m.count(i) == std::size_t(1) );
  TIMPI_UNIT_ASSERT( m.at(i) == 2*i+3 );
}

void tester(const std::multimap<int, int> & m, int i)
{
  TIMPI_UNIT_ASSERT( m.count(-1) * 3 == m.size() );

  std::bitset<2> found;
  auto pr = m.equal_range(i);
  for (auto it = pr.first; it != pr.second; ++it)
    {
      auto val = it->second;
      TIMPI_UNIT_ASSERT( val > 3*i && val < 3*i+3 );
      TIMPI_UNIT_ASSERT( !found[val-3*i-1] );
      found[val-3*i-1] = true;
    }
  TIMPI_UNIT_ASSERT( found.count() == 2 );
}

void tester(const std::unordered_multimap<int, int> & m, int i)
{
  TIMPI_UNIT_ASSERT( m.count(-1) * 3 == m.size() );

  std::bitset<2> found;
  const auto pr = m.equal_range(i);
  for (auto it = pr.first; it != pr.second; ++it)
    {
      auto val = it->second;
      TIMPI_UNIT_ASSERT( val > 3*i && val < 3*i+3 );
      TIMPI_UNIT_ASSERT( !found[val-3*i-1] );
      found[val-3*i-1] = true;
    }
  TIMPI_UNIT_ASSERT( found.count() == 2 );
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
  void testBigUnion(int n_multi = 1)
  {
    Set data;

    const int N = TestCommWorld->size();

    my_inserter(data, 150*N + TestCommWorld->rank());

    TestCommWorld->set_union(data);

    // The real assertions here are the internal ones in that
    // set_union
    TIMPI_UNIT_ASSERT( data.size() == n_multi * std::size_t(N) );
    for (int p=0; p<N; ++p)
      {
        tester(data, 150*N + p);
      }
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


  void testMapSet()
  {
    std::map<unsigned int, std::set<unsigned short>> mapset;

    // Values on all procs
    mapset[0].insert(20201);

    // Insert extra values on procs 0 and 2
    switch (TestCommWorld->rank())
      {
      case 0:
      case 2:
        mapset[0].insert(60201);
        break;
      default:
        break;
      }

    TestCommWorld->set_union(mapset);

    // Check results on all procs. In a broken 4-processor run what we
    // saw was:
    // key = 0, surface_ids = 20201 60201
    // key = 0, surface_ids = 20201
    // key = 0, surface_ids = 20201 60201
    // key = 0, surface_ids = 20201
    // whereas what we expect to see is that all procs have the
    // same surface_ids, the ones from pid 0.
    TIMPI_UNIT_ASSERT( mapset.size() == 1 );
    const std::set<unsigned short> goodset {20201, 60201};
    for (const auto & pr : mapset)
    {
      const auto & key = pr.first;
      const auto & boundary_id_set = pr.second;
      TIMPI_UNIT_ASSERT( key == 0 );
      TIMPI_UNIT_ASSERT( boundary_id_set == goodset );
    }
  }


  void testMapMap()
  {
    std::map<unsigned int, std::map<unsigned short, double>> mapmap;

    // Values on all procs
    mapmap[0].emplace(20201, 0.8);

    // Insert extra values on procs 0 and 2
    switch (TestCommWorld->rank())
      {
      case 0:
      case 2:
        mapmap[0].emplace(60201, 1.);
        break;
      default:
        break;
      }

    TestCommWorld->set_union(mapmap);

    TIMPI_UNIT_ASSERT( mapmap.size() == 1 );
    const std::map<unsigned short, double> goodmap {{20201, 0.8},
                                                    {60201, 1}};
    for (const auto & pr : mapmap)
    {
      const auto & key = pr.first;
      const auto & boundary_id_map = pr.second;
      TIMPI_UNIT_ASSERT( key == 0 );
      TIMPI_UNIT_ASSERT( boundary_id_map == goodmap );
    }
  }


int main(int argc, const char * const * argv)
{
  TIMPI::TIMPIInit init(argc, argv);
  TestCommWorld = &init.comm();

  testBigUnion<std::set<int>>();
  testBigUnion<std::multiset<int>>(3);
  testBigUnion<std::unordered_multiset<int>>(3);
  testBigUnion<std::unordered_set<int>>();
  testBigUnion<std::map<int, int>>();
  testBigUnion<std::multimap<int, int>>(3);
  testBigUnion<std::unordered_multimap<int, int>>(3);
  testBigUnion<std::unordered_map<int, int>>();

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

  testMapSet();
  testMapMap();

  return 0;
}
