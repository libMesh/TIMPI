#include <timpi/timpi.h>

#include <iterator>
#include <vector>
#include <set>

#define TIMPI_UNIT_ASSERT(expr) \
  if (!(expr)) \
    timpi_error()

namespace TIMPI
{
template <typename T>
class StandardType<std::set<T>> : public DataType
{
public:
  static const bool is_fixed_type = false;

private:
  StandardType(const std::set<T> * example = nullptr);
};
}

namespace libMesh {
namespace Parallel {
template <typename T>
class Packing<std::set<T>> {
public:

  static const unsigned int size_bytes = 4;

  typedef T buffer_type;

  static unsigned int
  get_set_len (typename std::vector<T>::const_iterator in)
  {
    unsigned int set_len = reinterpret_cast<const unsigned char &>(in[size_bytes-1]);
    for (signed int i=size_bytes-2; i >= 0; --i)
      {
        set_len *= 256;
        set_len += reinterpret_cast<const unsigned char &>(in[i]);
      }
    return set_len;
  }


  static unsigned int
  packed_size (typename std::vector<T>::const_iterator in)
  {
    return get_set_len(in) + size_bytes;
  }

  static unsigned int packable_size
  (const std::set<T> & s,
   const void *)
  {
    return s.size() + size_bytes;
  }


  template <typename Iter>
  static void pack (const std::set<T> & b, Iter data_out,
                    const void *)
  {
    unsigned int set_len = b.size();
    for (unsigned int i=0; i != size_bytes; ++i)
      {
        *data_out++ = (set_len % 256);
        set_len /= 256;
      }
    std::copy(b.begin(), b.end(), data_out);
  }

  static std::set<T>
  unpack (typename std::vector<T>::const_iterator in, void *)
  {
    unsigned int set_len = get_set_len(in);

    return std::set<T>(in + size_bytes, in + size_bytes + set_len);
  }

};


} // namespace Parallel

} // namespace libMesh

using namespace TIMPI;

Communicator *TestCommWorld;

  void testContainerAllGather()
  {
    std::vector<std::set<unsigned int>> vals;
    const unsigned int my_rank = TestCommWorld->rank();

    std::vector<int> data_vec(my_rank + 1);
    std::iota(data_vec.begin(), data_vec.end(), 0);
    TestCommWorld->allgather(std::set<unsigned int>(data_vec.begin(), data_vec.end()), vals);

    const std::size_t comm_size = TestCommWorld->size();
    const std::size_t vec_size = vals.size();
    TIMPI_UNIT_ASSERT(comm_size == vec_size);

    for (std::size_t i = 0; i < vec_size; ++i)
    {
      const auto & current_set = vals[i];
      unsigned int value = 0;
      for (auto number : current_set)
        TIMPI_UNIT_ASSERT(number == value++);
    }
  }

  void testPairContainerAllGather()
  {
    std::vector<std::pair<std::set<unsigned int>, unsigned int>> vals;
    const unsigned int my_rank = TestCommWorld->rank();

    std::vector<int> data_vec(my_rank + 1);
    std::iota(data_vec.begin(), data_vec.end(), 0);
    TestCommWorld->allgather(std::make_pair(
                               std::set<unsigned int>(data_vec.begin(), data_vec.end()),
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


int main(int argc, const char * const * argv)
{
  TIMPI::TIMPIInit init(argc, argv);
  TestCommWorld = &init.comm();

  testContainerAllGather();
  testContainerBroadcast();
  testPairContainerAllGather();

  return 0;
}
